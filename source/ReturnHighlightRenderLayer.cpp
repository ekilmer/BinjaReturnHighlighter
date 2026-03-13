#include "BinjaReturnHighlighter/ReturnHighlightRenderLayer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
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
		"description": "Color used to highlight return statements. Choose a preset or enter a custom hex color (e.g. #FF5500).",
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
		catch (...)
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

	BNHighlightColor ResolveHighlightColor(Logger& logger)
	{
		static BNHighlightColor lastValid = MakeStandardHighlight(BlueHighlightColor);

		const std::string colorSetting = Settings::Instance()->Get<std::string>("returnHighlighter.highlightColor");

		if (auto color = MapColorName(colorSetting))
		{
			lastValid = MakeStandardHighlight(*color);
		}
		else if (auto rgb = ParseHexColor(colorSetting))
		{
			lastValid = MakeCustomHighlight(rgb->r, rgb->g, rgb->b);
		}
		else
		{
			logger.LogWarn(  // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
				"unrecognized color '%s', using last valid color", colorSetting.c_str());
		}

		return lastValid;
	}

	bool LineContainsKeywordToken(DisassemblyTextLine& line)
	{
		return std::ranges::any_of(line.tokens, [](const auto& token) { return token.type == KeywordToken; });
	}

	bool LlilInstructionIsReturn(const LowLevelILInstruction& instruction)
	{
		const auto operation = instruction.operation;
		return operation == LLIL_RET || operation == LLIL_TAILCALL;
	}

	bool MlilInstructionIsReturn(const MediumLevelILInstruction& instruction)
	{
		const auto operation = instruction.operation;
		return operation == MLIL_RET || operation == MLIL_TAILCALL;
	}

	bool HlilInstructionIsReturn(const HighLevelILInstruction& instruction)
	{
		const auto operation = instruction.operation;
		return operation == HLIL_RET || operation == HLIL_TAILCALL;
	}
}  // namespace

void ReturnHighlightRenderLayer::ApplyToLowLevelILBlock(
	const Ref<BasicBlock> block, std::vector<DisassemblyTextLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor(*m_logger);
	Ref<LowLevelILFunction> const llilFunc = block->GetLowLevelILFunction();
	for (auto& line : lines)
	{
		if (LowLevelILInstruction const instr = llilFunc->GetInstruction(line.instrIndex);
			LlilInstructionIsReturn(instr) && LineContainsKeywordToken(line))
		{
			line.highlight = highlight;
		}
	}
}

void ReturnHighlightRenderLayer::ApplyToMediumLevelILBlock(
	const Ref<BasicBlock> block, std::vector<DisassemblyTextLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor(*m_logger);
	Ref<MediumLevelILFunction> const mlilFunc = block->GetMediumLevelILFunction();
	for (auto& line : lines)
	{
		if (MediumLevelILInstruction const instr = mlilFunc->GetInstruction(line.instrIndex);
			MlilInstructionIsReturn(instr) && LineContainsKeywordToken(line))
		{
			line.highlight = highlight;
		}
	}
}

void ReturnHighlightRenderLayer::ApplyToHighLevelILBlock(
	Ref<BasicBlock> const block, std::vector<DisassemblyTextLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor(*m_logger);
	Ref<HighLevelILFunction> const hlilFunc = block->GetHighLevelILFunction();
	for (auto& line : lines)
	{
		if (HighLevelILInstruction const instr = hlilFunc->GetInstruction(line.instrIndex);
			HlilInstructionIsReturn(instr) && LineContainsKeywordToken(line))
		{
			line.highlight = highlight;
		}
	}
}

void ReturnHighlightRenderLayer::ApplyToHighLevelILBody(
	const Ref<Function> function, std::vector<LinearDisassemblyLine>& lines)
{
	const BNHighlightColor highlight = ResolveHighlightColor(*m_logger);
	Ref<HighLevelILFunction> const hlilFunc = function->GetHighLevelIL();
	for (auto& linearLine : lines)
	{
		DisassemblyTextLine& line = linearLine.contents;

		if (HighLevelILInstruction const instr = hlilFunc->GetInstruction(line.instrIndex);
			HlilInstructionIsReturn(instr) && LineContainsKeywordToken(line))
		{
			line.highlight = highlight;
		}
	}
}
