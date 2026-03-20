#ifndef EXITPOINTDETECTION_HPP
#define EXITPOINTDETECTION_HPP

#include <highlevelilinstruction.h>
#include <lowlevelilinstruction.h>
#include <mediumlevelilinstruction.h>

bool LlilInstructionIsExitPoint(const BinaryNinja::LowLevelILInstruction& instruction);
bool MlilInstructionIsExitPoint(const BinaryNinja::MediumLevelILInstruction& instruction);
bool HlilInstructionIsExitPoint(const BinaryNinja::HighLevelILInstruction& instruction);

#endif  // EXITPOINTDETECTION_HPP
