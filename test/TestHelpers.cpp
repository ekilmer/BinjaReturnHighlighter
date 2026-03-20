#include "TestHelpers.hpp"

#include <BinjaReturnHighlighter/ReturnHighlightRenderLayer.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <binaryninjaapi.h>
#include <binaryninjacore.h>

#include <gtest/gtest.h>

using namespace BinaryNinja;

namespace TestHelpers {

	Ref<BinaryView>& GlobalBv()
	{
		static Ref<BinaryView> instance;
		return instance;
	}

	Ref<Function> FindFunctionByName(const std::string& name)
	{
		for (const auto& func : GlobalBv()->GetAnalysisFunctionList())
		{
			const Ref<Symbol> sym = func->GetSymbol();
			if (sym != nullptr && (sym->GetFullName() == name || sym->GetShortName() == name))
			{
				return func;
			}
		}
		return nullptr;
	}

	const DisassemblyTextLine* FindLineByAddr(const std::vector<DisassemblyTextLine>& lines, uint64_t addr)
	{
		const auto iter = std::ranges::find_if(lines, [addr](const DisassemblyTextLine& line) {
			return line.addr == addr;
		});
		if (iter == lines.end())
		{
			return nullptr;
		}
		return &(*iter);
	}

	bool AnyLineHighlightedAtAddr(const std::vector<DisassemblyTextLine>& lines, uint64_t addr)
	{
		return std::ranges::any_of(lines, [addr](const DisassemblyTextLine& line) {
			return line.addr == addr && IsHighlighted(line);
		});
	}

	std::vector<LinearDisassemblyLine> BuildLinearLines(const Ref<Function>& func, const Ref<HighLevelILFunction>& hlil)
	{
		std::vector<LinearDisassemblyLine> linearLines;
		for (const auto& block : hlil->GetBasicBlocks())
		{
			DisassemblySettings settings;
			const std::vector<DisassemblyTextLine> blockLines = block->GetDisassemblyText(&settings);
			for (const auto& line : blockLines)
			{
				LinearDisassemblyLine linearLine;
				linearLine.type = CodeDisassemblyLineType;
				linearLine.function = func;
				linearLine.contents = line;
				linearLines.push_back(linearLine);
			}
		}
		return linearLines;
	}

	void BinaryNinjaEnvironment::SetUp()
	{
		const std::string installDir = BN_INSTALL_DIR;
#ifdef __APPLE__
		const std::string pluginDir = installDir + "/Contents/MacOS/plugins";
#elif defined(_WIN32)
		const std::string pluginDir = installDir + "/plugins";
#else
		const std::string pluginDir = installDir + "/plugins";
#endif
		SetBundledPluginDirectory(pluginDir);
		InitPlugins();
		RegisterReturnHighlighterSettings();
		Settings::Instance()->LoadSettingsFile("", SettingsUserScope);

		const std::string testBinary = TEST_DATA_DIR "simple";
		const Ref<BinaryView> binaryView = BinaryNinja::Load(testBinary);
		ASSERT_NE(binaryView, nullptr) << "Failed to load test binary: " << testBinary;
		ASSERT_NE(binaryView->GetTypeName(), "Raw") << "Binary loaded as Raw view";
		binaryView->UpdateAnalysisAndWait();
		GlobalBv() = binaryView;
	}

	void BinaryNinjaEnvironment::TearDown()
	{
		if (GlobalBv() != nullptr)
		{
			GlobalBv()->GetFile()->Close();
			GlobalBv() = nullptr;
		}
		BNShutdown();
	}

}  // namespace TestHelpers
