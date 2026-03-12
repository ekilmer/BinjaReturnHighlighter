#include <BinjaReturnHighlighter/ReturnHighlightRenderLayer.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <binaryninjaapi.h>
#include <binaryninjacore.h>

#include <gtest/gtest.h>

using namespace BinaryNinja;

namespace {

	constexpr uint8_t KFullAlpha = 255;

	// Addresses from the test binary (test/data/simple, compiled from test/data/simple.c).
	// _add: 0x100000328, ret at 0x100000344
	// _main: 0x100000348, ret at 0x10000036c
	constexpr uint64_t KAddRetAddr = 0x100000344;
	constexpr uint64_t KAddNonRetAddr = 0x10000032c;
	constexpr uint64_t KMainRetAddr = 0x10000036c;
	constexpr uint64_t KMainNonRetAddr = 0x100000354;

	// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
	BinaryView* GlobalBv = nullptr;
	// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

	Ref<Function> FindFunctionByName(const std::string& name)
	{
		for (const auto& func : GlobalBv->GetAnalysisFunctionList())
		{
			const Ref<Symbol> sym = func->GetSymbol();
			if (sym != nullptr && (sym->GetFullName() == name || sym->GetShortName() == name))
			{
				return func;
			}
		}
		return nullptr;
	}

	bool IsHighlighted(const DisassemblyTextLine& line)
	{
		return line.highlight.style == StandardHighlightColor && line.highlight.color == BlueHighlightColor
			&& line.highlight.alpha == KFullAlpha;
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

	class BinaryNinjaEnvironment : public ::testing::Environment
	{
	public:
		void SetUp() override
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

			const std::string testBinary = TEST_DATA_DIR "simple";
			const Ref<BinaryView> binaryView = BinaryNinja::Load(testBinary);
			ASSERT_NE(binaryView, nullptr) << "Failed to load test binary: " << testBinary;
			ASSERT_NE(binaryView->GetTypeName(), "Raw") << "Binary loaded as Raw view";
			binaryView->UpdateAnalysisAndWait();
			GlobalBv = binaryView.GetPtr();
			GlobalBv->AddRef();
		}

		void TearDown() override
		{
			if (GlobalBv != nullptr)
			{
				GlobalBv->GetFile()->Close();
				GlobalBv->Release();
				GlobalBv = nullptr;
			}
			BNShutdown();
		}
	};

}  // namespace

// NOLINTBEGIN(misc-use-internal-linkage)
class ReturnHighlightTest : public ::testing::Test
{
protected:
	ReturnHighlightRenderLayer m_Layer;

	static Ref<Function> FindFunction(const std::string& name)
	{
		Ref<Function> func = FindFunctionByName(name);
		if (func == nullptr)
		{
			func = FindFunctionByName("_" + name);
		}
		return func;
	}
};
// NOLINTEND(misc-use-internal-linkage)

TEST_F(ReturnHighlightTest, LLILReturnHighlighted)
{
	const Ref<Function> func = FindFunction("add");
	ASSERT_NE(func, nullptr) << "Could not find 'add' function";

	const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
	ASSERT_NE(llil, nullptr);

	std::vector<DisassemblyTextLine> allLines;
	for (const auto& block : llil->GetBasicBlocks())
	{
		DisassemblySettings settings;
		std::vector<DisassemblyTextLine> lines = block->GetDisassemblyText(&settings);
		m_Layer.ApplyToLowLevelILBlock(block, lines);
		allLines.insert(allLines.end(), lines.begin(), lines.end());
	}

	const auto* retLine = FindLineByAddr(allLines, KAddRetAddr);
	ASSERT_NE(retLine, nullptr) << "Could not find return line at 0x100000344";
	EXPECT_TRUE(IsHighlighted(*retLine)) << "Return line at 0x100000344 should be highlighted";

	const auto* nonRetLine = FindLineByAddr(allLines, KAddNonRetAddr);
	ASSERT_NE(nonRetLine, nullptr) << "Could not find non-return line at 0x10000032c";
	EXPECT_FALSE(IsHighlighted(*nonRetLine)) << "Non-return line at 0x10000032c should not be highlighted";
}

TEST_F(ReturnHighlightTest, MLILReturnHighlighted)
{
	const Ref<Function> func = FindFunction("add");
	ASSERT_NE(func, nullptr) << "Could not find 'add' function";

	const Ref<MediumLevelILFunction> mlil = func->GetMediumLevelIL();
	ASSERT_NE(mlil, nullptr);

	std::vector<DisassemblyTextLine> allLines;
	for (const auto& block : mlil->GetBasicBlocks())
	{
		DisassemblySettings settings;
		std::vector<DisassemblyTextLine> lines = block->GetDisassemblyText(&settings);
		m_Layer.ApplyToMediumLevelILBlock(block, lines);
		allLines.insert(allLines.end(), lines.begin(), lines.end());
	}

	const auto* retLine = FindLineByAddr(allLines, KAddRetAddr);
	ASSERT_NE(retLine, nullptr) << "Could not find return line at 0x100000344";
	EXPECT_TRUE(IsHighlighted(*retLine)) << "Return line at 0x100000344 should be highlighted";

	const auto* nonRetLine = FindLineByAddr(allLines, KAddNonRetAddr);
	ASSERT_NE(nonRetLine, nullptr) << "Could not find non-return line at 0x10000032c";
	EXPECT_FALSE(IsHighlighted(*nonRetLine)) << "Non-return line at 0x10000032c should not be highlighted";
}

TEST_F(ReturnHighlightTest, HLILBlockReturnHighlighted)
{
	const Ref<Function> func = FindFunction("main");
	ASSERT_NE(func, nullptr) << "Could not find 'main' function";

	const Ref<HighLevelILFunction> hlil = func->GetHighLevelIL();
	ASSERT_NE(hlil, nullptr);

	std::vector<DisassemblyTextLine> allLines;
	for (const auto& block : hlil->GetBasicBlocks())
	{
		DisassemblySettings settings;
		std::vector<DisassemblyTextLine> lines = block->GetDisassemblyText(&settings);
		m_Layer.ApplyToHighLevelILBlock(block, lines);
		allLines.insert(allLines.end(), lines.begin(), lines.end());
	}

	const auto* retLine = FindLineByAddr(allLines, KMainRetAddr);
	ASSERT_NE(retLine, nullptr) << "Could not find return line at 0x10000036c";
	EXPECT_TRUE(IsHighlighted(*retLine)) << "Return line at 0x10000036c should be highlighted";

	const auto* nonRetLine = FindLineByAddr(allLines, KMainNonRetAddr);
	ASSERT_NE(nonRetLine, nullptr) << "Could not find non-return line at 0x100000354";
	EXPECT_FALSE(IsHighlighted(*nonRetLine)) << "Non-return line at 0x100000354 should not be highlighted";
}

TEST_F(ReturnHighlightTest, HLILBodyReturnHighlighted)
{
	const Ref<Function> func = FindFunction("main");
	ASSERT_NE(func, nullptr) << "Could not find 'main' function";

	const Ref<HighLevelILFunction> hlil = func->GetHighLevelIL();
	ASSERT_NE(hlil, nullptr);

	std::vector<LinearDisassemblyLine> linearLines = BuildLinearLines(func, hlil);
	m_Layer.ApplyToHighLevelILBody(func, linearLines);

	const auto retLine = std::ranges::find_if(linearLines, [](const LinearDisassemblyLine& line) {
		return line.contents.addr == KMainRetAddr;
	});
	ASSERT_NE(retLine, linearLines.end()) << "Could not find return line at 0x10000036c";
	EXPECT_TRUE(IsHighlighted(retLine->contents)) << "Return line at 0x10000036c should be highlighted";

	const auto nonRetLine = std::ranges::find_if(linearLines, [](const LinearDisassemblyLine& line) {
		return line.contents.addr == KMainNonRetAddr;
	});
	ASSERT_NE(nonRetLine, linearLines.end()) << "Could not find non-return line at 0x100000354";
	EXPECT_FALSE(IsHighlighted(nonRetLine->contents)) << "Non-return line at 0x100000354 should not be highlighted";
}

auto main(int argc, char** argv) -> int
{
	::testing::InitGoogleTest(&argc, argv);
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
	::testing::AddGlobalTestEnvironment(new BinaryNinjaEnvironment());
	return RUN_ALL_TESTS();
}
