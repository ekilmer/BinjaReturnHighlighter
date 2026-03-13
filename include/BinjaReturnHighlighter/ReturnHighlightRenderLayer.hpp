#ifndef RETURNHIGHLIGHTRENDERLAYER_HPP
#define RETURNHIGHLIGHTRENDERLAYER_HPP

#include <vector>

#include <binaryninjaapi.h>

void RegisterReturnHighlighterSettings();

class ReturnHighlightRenderLayer final : public BinaryNinja::RenderLayer
{
	BinaryNinja::Ref<BinaryNinja::Logger> m_logger;

public:
	static constexpr const char* LoggerName = "ReturnHighlighter";

	ReturnHighlightRenderLayer() :
		RenderLayer("Highlight Return Statements"), m_logger(BinaryNinja::LogRegistry::CreateLogger(LoggerName))
	{}
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
