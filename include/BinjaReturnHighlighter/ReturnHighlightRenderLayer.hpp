#ifndef RETURNHIGHLIGHTRENDERLAYER_HPP
#define RETURNHIGHLIGHTRENDERLAYER_HPP

#include <vector>

#include <binaryninjaapi.h>

class ReturnHighlightRenderLayer final : public BinaryNinja::RenderLayer
{
public:
	ReturnHighlightRenderLayer() : RenderLayer("Highlight Return Statements") {}
	void ApplyToLowLevelILBlock(
		BinaryNinja::Ref<BinaryNinja::BasicBlock> block, std::vector<BinaryNinja::DisassemblyTextLine>& lines) override;
	void ApplyToMediumLevelILBlock(
		BinaryNinja::Ref<BinaryNinja::BasicBlock> block, std::vector<BinaryNinja::DisassemblyTextLine>& lines) override;
	void ApplyToHighLevelILBlock(
		BinaryNinja::Ref<BinaryNinja::BasicBlock> block, std::vector<BinaryNinja::DisassemblyTextLine>& lines) override;
	void ApplyToHighLevelILBody(BinaryNinja::Ref<BinaryNinja::Function> function,
		std::vector<BinaryNinja::LinearDisassemblyLine>& lines) override;
};

#endif  // RETURNHIGHLIGHTRENDERLAYER_HPP
