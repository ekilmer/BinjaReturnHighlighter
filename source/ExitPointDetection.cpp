#include "BinjaReturnHighlighter/ExitPointDetection.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include <binaryninjaapi.h>
#include <binaryninjacore.h>

using namespace BinaryNinja;

namespace {
	bool IsNoreturnCallDest(uint64_t destAddr, const Ref<Function>& caller)
	{
		auto view = caller->GetView();
		return std::ranges::any_of(view->GetAnalysisFunctionsForAddress(destAddr), [](const Ref<Function>& target) {
			return !target->CanReturn().GetValue();
		});
	}

	template <auto RetOp, auto TailcallOp, auto NoretOp, auto ConstPtrOp, auto... CallOps>
	bool InstructionIsExitPoint(const auto& instruction)
	{
		const auto operation = instruction.operation;
		if (operation == RetOp || operation == TailcallOp || operation == NoretOp)
		{
			return true;
		}
		if (((operation == CallOps) || ...))
		{
			auto dest = instruction.GetDestExpr();
			if (dest.operation == ConstPtrOp)
			{
				if (IsNoreturnCallDest(static_cast<uint64_t>(dest.GetConstant()), instruction.function->GetFunction()))
				{
					return true;
				}
			}
		}
		return false;
	}
}  // namespace

bool LlilInstructionIsExitPoint(const LowLevelILInstruction& instruction)
{
	return InstructionIsExitPoint<LLIL_RET, LLIL_TAILCALL, LLIL_NORET, LLIL_CONST_PTR, LLIL_CALL>(instruction);
}

bool MlilInstructionIsExitPoint(const MediumLevelILInstruction& instruction)
{
	return InstructionIsExitPoint<MLIL_RET, MLIL_TAILCALL, MLIL_NORET, MLIL_CONST_PTR, MLIL_CALL, MLIL_CALL_UNTYPED>(
		instruction);
}

bool HlilInstructionIsExitPoint(const HighLevelILInstruction& instruction)
{
	return InstructionIsExitPoint<HLIL_RET, HLIL_TAILCALL, HLIL_NORET, HLIL_CONST_PTR, HLIL_CALL>(instruction);
}

std::unordered_set<uint64_t> FindExitPointAddresses(const Ref<Function>& func, uint64_t blockStart, uint64_t blockEnd)
{
	std::unordered_set<uint64_t> exitAddrs;
	auto llil = func->GetLowLevelIL();
	if (!llil)
	{
		return exitAddrs;
	}
	for (const auto& llilBlock : llil->GetBasicBlocks())
	{
		const size_t llilStart = llilBlock->GetStart();
		const size_t llilEnd = llilBlock->GetEnd();
		if (llilStart >= llilEnd)
		{
			continue;
		}
		const uint64_t firstAddr = llil->GetInstruction(llilStart).address;
		const uint64_t lastAddr = llil->GetInstruction(llilEnd - 1).address;
		if (lastAddr < blockStart || firstAddr >= blockEnd)
		{
			continue;
		}
		for (size_t i = llilStart; i < llilEnd; i++)
		{
			auto instr = llil->GetInstruction(i);
			if (instr.address >= blockStart && instr.address < blockEnd && LlilInstructionIsExitPoint(instr))
			{
				exitAddrs.insert(instr.address);
			}
		}
	}
	return exitAddrs;
}
