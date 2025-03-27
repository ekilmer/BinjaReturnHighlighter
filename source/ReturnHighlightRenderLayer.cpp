#include "BinjaReturnHighlighter/ReturnHighlightRenderLayer.hpp"

#include <algorithm>
#include <vector>

#include <binaryninjaapi.h>
#include <binaryninjacore.h>
#include <highlevelilinstruction.h>
#include <lowlevelilinstruction.h>
#include <mediumlevelilinstruction.h>

using namespace BinaryNinja;

namespace {
	constexpr int AlphaSolid = 255;

	void HighLightReturnLine(DisassemblyTextLine& line)
	{
		line.highlight.style = StandardHighlightColor;
		line.highlight.color = BlueHighlightColor;
		line.highlight.mixColor = NoHighlightColor;
		line.highlight.mix = 0;
		line.highlight.r = 0;
		line.highlight.g = 0;
		line.highlight.b = 0;
		line.highlight.alpha = AlphaSolid;
	}

	bool LineContainsKeywordToken(DisassemblyTextLine& line)
	{
		return std::any_of(line.tokens.begin(), line.tokens.end(), [](const auto& token) {
			return token.type == KeywordToken;
		});
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
	Ref<LowLevelILFunction> const llilFunc = block->GetLowLevelILFunction();
	for (auto& line : lines)
	{
		if (LowLevelILInstruction const instr = llilFunc->GetInstruction(line.instrIndex);
			LlilInstructionIsReturn(instr) && LineContainsKeywordToken(line))
		{
			HighLightReturnLine(line);
		}
	}
}

void ReturnHighlightRenderLayer::ApplyToMediumLevelILBlock(
	const Ref<BasicBlock> block, std::vector<DisassemblyTextLine>& lines)
{
	Ref<MediumLevelILFunction> const mlilFunc = block->GetMediumLevelILFunction();
	for (auto& line : lines)
	{
		if (MediumLevelILInstruction const instr = mlilFunc->GetInstruction(line.instrIndex);
			MlilInstructionIsReturn(instr) && LineContainsKeywordToken(line))
		{
			HighLightReturnLine(line);
		}
	}
}

void ReturnHighlightRenderLayer::ApplyToHighLevelILBlock(
	Ref<BasicBlock> const block, std::vector<DisassemblyTextLine>& lines)
{
	Ref<HighLevelILFunction> const hlilFunc = block->GetHighLevelILFunction();
	for (auto& line : lines)
	{
		if (HighLevelILInstruction const instr = hlilFunc->GetInstruction(line.instrIndex);
			HlilInstructionIsReturn(instr) && LineContainsKeywordToken(line))
		{
			HighLightReturnLine(line);
		}
	}
}

void ReturnHighlightRenderLayer::ApplyToHighLevelILBody(
	const Ref<Function> function, std::vector<LinearDisassemblyLine>& lines)
{
	Ref<HighLevelILFunction> const hlilFunc = function->GetHighLevelIL();
	for (auto& linearLine : lines)
	{
		DisassemblyTextLine& line = linearLine.contents;

		// Need to check for a keyword token. Not sure if there's a more efficient way
		if (HighLevelILInstruction const instr = hlilFunc->GetInstruction(line.instrIndex);
			HlilInstructionIsReturn(instr) && LineContainsKeywordToken(line))
		{
			HighLightReturnLine(line);
		}
	}
}
