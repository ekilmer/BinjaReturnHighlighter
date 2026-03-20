#pragma once

#include <cstdint>
#include <unordered_set>

#include <binaryninjaapi.h>
#include <highlevelilinstruction.h>
#include <lowlevelilinstruction.h>
#include <mediumlevelilinstruction.h>

bool LlilInstructionIsExitPoint(const BinaryNinja::LowLevelILInstruction& instruction);
bool MlilInstructionIsExitPoint(const BinaryNinja::MediumLevelILInstruction& instruction);
bool HlilInstructionIsExitPoint(const BinaryNinja::HighLevelILInstruction& instruction);

std::unordered_set<uint64_t> FindExitPointAddresses(
	const BinaryNinja::Ref<BinaryNinja::Function>& func, uint64_t blockStart, uint64_t blockEnd);
