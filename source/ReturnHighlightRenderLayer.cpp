#include "BinjaReturnHighlighter/ReturnHighlightRenderLayer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <binaryninjaapi.h>
#include <binaryninjacore.h>

#include "BinjaReturnHighlighter/ColorDefs.hpp"
#include "BinjaReturnHighlighter/ExitPointDetection.hpp"

using namespace BinaryNinja;

void RegisterReturnHighlighterSettings()
{
	auto settings = Settings::Instance();
	settings->RegisterGroup("returnHighlighter", "Return Highlighter");
	settings->RegisterSetting("returnHighlighter.highlightColor",
		R"~({
		"title": "Highlight Color",
		"type": "string",
		"default": "blue",
		"description": "Color used to highlight exit points (return statements and noreturn calls). Choose a preset or enter a custom hex color (e.g. #FF5500).",
		"enum": ["blue", "green", "cyan", "red", "magenta", "yellow", "orange", "white", "black"],
		"enumDescriptions": ["Blue", "Green", "Cyan", "Red", "Magenta", "Yellow", "Orange", "White", "Black"],
		"ignore": ["SettingsProjectScope", "SettingsResourceScope"]
		})~");
}

namespace {
	constexpr int AlphaSolid = 255;

	struct RgbColor
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

	constexpr size_t HexColorLen = 6;
	constexpr uint32_t RedShift = 16;
	constexpr uint32_t GreenShift = 8;
	constexpr uint32_t ByteMask = 0xFF;
	constexpr int HexBase = 16;

	std::optional<RgbColor> ParseHexColor(const std::string& str)
	{
		std::string hex = str;
		if (!hex.empty() && hex.front() == '#')
		{
			hex = hex.substr(1);
		}
		if (hex.size() != HexColorLen)
		{
			return std::nullopt;
		}
		uint32_t val = 0;
		try
		{
			val = static_cast<uint32_t>(std::stoul(hex, nullptr, HexBase));
		}
		catch (const std::invalid_argument&)
		{
			return std::nullopt;
		}
		catch (const std::out_of_range&)
		{
			return std::nullopt;
		}
		return RgbColor {.r = static_cast<uint8_t>((val >> RedShift) & ByteMask),
			.g = static_cast<uint8_t>((val >> GreenShift) & ByteMask),
			.b = static_cast<uint8_t>(val & ByteMask)};
	}

	std::optional<BNHighlightStandardColor> MapColorName(const std::string& name)
	{
		if (const auto* def = FindColorByName(name))
		{
			return def->bnColor;
		}
		return std::nullopt;
	}

	BNHighlightColor MakeStandardHighlight(BNHighlightStandardColor color)
	{
		return {.style = StandardHighlightColor,
			.color = color,
			.mixColor = NoHighlightColor,
			.mix = 0,
			.r = 0,
			.g = 0,
			.b = 0,
			.alpha = AlphaSolid};
	}

	BNHighlightColor MakeCustomHighlight(uint8_t red, uint8_t green, uint8_t blue)
	{
		return {.style = CustomHighlightColor,
			.color = NoHighlightColor,
			.mixColor = NoHighlightColor,
			.mix = 0,
			.r = red,
			.g = green,
			.b = blue,
			.alpha = AlphaSolid};
	}

	bool LineContainsKeywordToken(const DisassemblyTextLine& line)
	{
		return std::ranges::any_of(line.tokens, [](const auto& token) { return token.type == KeywordToken; });
	}

	bool LineContainsCallTarget(const DisassemblyTextLine& line)
	{
		return std::ranges::any_of(line.tokens, [](const auto& token) {
			return token.type == CodeRelativeAddressToken || token.type == CodeSymbolToken || token.type == ImportToken
				|| token.type == IndirectImportToken;
		});
	}

	DisassemblyTextLine& GetDisasmLine(DisassemblyTextLine& line)
	{
		return line;
	}
	DisassemblyTextLine& GetDisasmLine(LinearDisassemblyLine& line)
	{
		return line.contents;
	}

	template <typename ILFuncT, typename LineT, typename IsExitPointFn>
	void HighlightExitPointLines(
		BNHighlightColor highlight, const Ref<ILFuncT>& ilFunc, std::vector<LineT>& lines, IsExitPointFn isExitPoint)
	{
		for (auto& line : lines)
		{
			DisassemblyTextLine& disasmLine = GetDisasmLine(line);
			if (isExitPoint(ilFunc->GetInstruction(disasmLine.instrIndex))
				&& (LineContainsKeywordToken(disasmLine) || LineContainsCallTarget(disasmLine)))
			{
				disasmLine.highlight = highlight;
			}
		}
	}
}  // namespace

BNHighlightColor ReturnHighlightRenderLayer::ResolveHighlightColor() const
{
	const std::string colorSetting = Settings::Instance()->Get<std::string>("returnHighlighter.highlightColor");

	if (colorSetting == m_cachedColorSetting)
	{
		return m_cachedHighlight;
	}

	if (auto color = MapColorName(colorSetting))
	{
		m_cachedHighlight = MakeStandardHighlight(*color);
		m_cachedColorSetting = colorSetting;
	}
	else if (auto rgb = ParseHexColor(colorSetting))
	{
		m_cachedHighlight = MakeCustomHighlight(rgb->r, rgb->g, rgb->b);
		m_cachedColorSetting = colorSetting;
	}
	else
	{
		m_logger->LogWarnF("unrecognized color '{}', using last valid color", colorSetting);
	}

	return m_cachedHighlight;
}

void ReturnHighlightRenderLayer::ApplyToDisassemblyBlock(Ref<BasicBlock> block, std::vector<DisassemblyTextLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor();
	auto func = block->GetFunction();
	if (!func)
	{
		return;
	}

	const auto exitAddrs = FindExitPointAddresses(func, block->GetStart(), block->GetEnd());
	for (auto& line : lines)
	{
		if (exitAddrs.contains(line.addr))
		{
			line.highlight = highlight;
		}
	}
}

void ReturnHighlightRenderLayer::ApplyToLowLevelILBlock(
	const Ref<BasicBlock> block, std::vector<DisassemblyTextLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor();
	HighlightExitPointLines(highlight, block->GetLowLevelILFunction(), lines, LlilInstructionIsExitPoint);
}

void ReturnHighlightRenderLayer::ApplyToMediumLevelILBlock(
	const Ref<BasicBlock> block, std::vector<DisassemblyTextLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor();
	HighlightExitPointLines(highlight, block->GetMediumLevelILFunction(), lines, MlilInstructionIsExitPoint);
}

void ReturnHighlightRenderLayer::ApplyToHighLevelILBlock(
	Ref<BasicBlock> const block, std::vector<DisassemblyTextLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor();
	HighlightExitPointLines(highlight, block->GetHighLevelILFunction(), lines, HlilInstructionIsExitPoint);
}

void ReturnHighlightRenderLayer::ApplyToHighLevelILBody(
	const Ref<Function> function, std::vector<LinearDisassemblyLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor();
	HighlightExitPointLines(highlight, function->GetHighLevelIL(), lines, HlilInstructionIsExitPoint);
}
