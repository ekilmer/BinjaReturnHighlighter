#include "BinjaReturnHighlighter/ReturnHighlightRenderLayer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <binaryninjaapi.h>
#include <binaryninjacore.h>

#include "BinjaReturnHighlighter/ColorDefs.hpp"
#include <highlevelilinstruction.h>
#include <lowlevelilinstruction.h>
#include <mediumlevelilinstruction.h>

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
		if (auto def = FindColorByName(name))
		{
			return (*def)->bnColor;
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

	bool IsNoreturnCallDest(uint64_t destAddr, const Ref<Function>& caller)
	{
		auto view = caller->GetView();
		return std::ranges::any_of(view->GetAnalysisFunctionsForAddress(destAddr), [](const Ref<Function>& target) {
			return !target->CanReturn().GetValue();
		});
	}

	bool LlilInstructionIsExitPoint(const LowLevelILInstruction& instruction)
	{
		const auto operation = instruction.operation;
		if (operation == LLIL_RET || operation == LLIL_TAILCALL || operation == LLIL_NORET)
		{
			return true;
		}
		if (operation == LLIL_CALL)
		{
			auto dest = instruction.GetDestExpr();
			if (dest.operation == LLIL_CONST_PTR)
			{
				if (IsNoreturnCallDest(static_cast<uint64_t>(dest.GetConstant()), instruction.function->GetFunction()))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool MlilInstructionIsExitPoint(const MediumLevelILInstruction& instruction)
	{
		const auto operation = instruction.operation;
		if (operation == MLIL_RET || operation == MLIL_TAILCALL || operation == MLIL_NORET)
		{
			return true;
		}
		if (operation == MLIL_CALL || operation == MLIL_CALL_UNTYPED)
		{
			auto dest = instruction.GetDestExpr();
			if (dest.operation == MLIL_CONST_PTR)
			{
				if (IsNoreturnCallDest(static_cast<uint64_t>(dest.GetConstant()), instruction.function->GetFunction()))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool HlilInstructionIsExitPoint(const HighLevelILInstruction& instruction)
	{
		const auto operation = instruction.operation;
		if (operation == HLIL_RET || operation == HLIL_TAILCALL || operation == HLIL_NORET)
		{
			return true;
		}
		if (operation == HLIL_CALL)
		{
			auto dest = instruction.GetDestExpr();
			if (dest.operation == HLIL_CONST_PTR)
			{
				if (IsNoreturnCallDest(static_cast<uint64_t>(dest.GetConstant()), instruction.function->GetFunction()))
				{
					return true;
				}
			}
		}
		return false;
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
	auto llil = func->GetLowLevelIL();
	if (!llil)
	{
		return;
	}

	const uint64_t blockStart = block->GetStart();
	const uint64_t blockEnd = block->GetEnd();
	std::set<uint64_t> exitAddrs;
	for (size_t i = 0; i < llil->GetInstructionCount(); i++)
	{
		auto instr = llil->GetInstruction(i);
		if (instr.address >= blockStart && instr.address < blockEnd && LlilInstructionIsExitPoint(instr))
		{
			exitAddrs.insert(instr.address);
		}
	}

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
