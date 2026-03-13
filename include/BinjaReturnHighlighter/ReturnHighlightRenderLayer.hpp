#ifndef RETURNHIGHLIGHTRENDERLAYER_HPP
#define RETURNHIGHLIGHTRENDERLAYER_HPP

#include <string>
#include <vector>

#include <binaryninjaapi.h>
#include <binaryninjacore.h>

void RegisterReturnHighlighterSettings();

class ReturnHighlightRenderLayer final : public BinaryNinja::RenderLayer
{
	BinaryNinja::Ref<BinaryNinja::Logger> m_logger;
	mutable std::string m_cachedColorSetting;
	mutable BNHighlightColor m_cachedHighlight;

	BNHighlightColor ResolveHighlightColor() const;

public:
	static constexpr const char* LoggerName = "ReturnHighlighter";

	ReturnHighlightRenderLayer() :
		RenderLayer("Highlight Return Statements"), m_logger(BinaryNinja::LogRegistry::CreateLogger(LoggerName)),
		m_cachedHighlight {}
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
