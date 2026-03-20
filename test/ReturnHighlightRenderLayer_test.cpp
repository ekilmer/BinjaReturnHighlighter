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
using namespace TestHelpers;

namespace {

	// Addresses from the test binary (test/data/simple, compiled from test/data/simple.c).
	// _add: 0x100000410, ret at 0x10000042c
	// _die: 0x100000430, call exit at 0x100000444
	// _main: 0x100000448, ret at 0x10000046c
	constexpr uint64_t AddRetAddr = 0x10000042c;
	constexpr uint64_t AddNonRetAddr = 0x100000414;
	constexpr uint64_t DieCallExitAddr = 0x100000444;
	constexpr uint64_t DieNonExitAddr = 0x100000434;
	constexpr uint64_t MainRetAddr = 0x10000046c;
	constexpr uint64_t MainNonRetAddr = 0x100000454;

	class ReturnHighlightTest : public ::testing::Test
	{
	protected:
		void TearDown() override { Settings::Instance()->Reset("returnHighlighter.highlightColor"); }

		static Ref<Function> FindFunction(const std::string& name)
		{
			Ref<Function> func = FindFunctionByName(name);
			if (func == nullptr)
			{
				func = FindFunctionByName("_" + name);
			}
			return func;
		}

		std::vector<DisassemblyTextLine> CollectDisassemblyLines(const Ref<Function>& func)
		{
			std::vector<DisassemblyTextLine> allLines;
			for (const auto& block : func->GetBasicBlocks())
			{
				DisassemblySettings settings;
				std::vector<DisassemblyTextLine> lines = block->GetDisassemblyText(&settings);
				m_layer.ApplyToDisassemblyBlock(block, lines);
				allLines.insert(allLines.end(), lines.begin(), lines.end());
			}
			return allLines;
		}

		template <typename ILFuncT, typename ApplyFn>
		std::vector<DisassemblyTextLine> CollectILLines(const Ref<ILFuncT>& ilFunc, ApplyFn apply)
		{
			std::vector<DisassemblyTextLine> allLines;
			for (const auto& block : ilFunc->GetBasicBlocks())
			{
				DisassemblySettings settings;
				std::vector<DisassemblyTextLine> lines = block->GetDisassemblyText(&settings);
				apply(block, lines);
				allLines.insert(allLines.end(), lines.begin(), lines.end());
			}
			return allLines;
		}

		std::vector<DisassemblyTextLine> CollectLLILLines(const Ref<LowLevelILFunction>& llil)
		{
			return CollectILLines(llil, [this](const Ref<BasicBlock>& block, std::vector<DisassemblyTextLine>& lines) {
				m_layer.ApplyToLowLevelILBlock(block, lines);
			});
		}

		std::vector<DisassemblyTextLine> CollectMLILLines(const Ref<MediumLevelILFunction>& mlil)
		{
			return CollectILLines(mlil, [this](const Ref<BasicBlock>& block, std::vector<DisassemblyTextLine>& lines) {
				m_layer.ApplyToMediumLevelILBlock(block, lines);
			});
		}

		std::vector<DisassemblyTextLine> CollectHLILLines(const Ref<HighLevelILFunction>& hlil)
		{
			return CollectILLines(hlil, [this](const Ref<BasicBlock>& block, std::vector<DisassemblyTextLine>& lines) {
				m_layer.ApplyToHighLevelILBlock(block, lines);
			});
		}

		void ApplyToHLILBody(const Ref<Function>& func, std::vector<LinearDisassemblyLine>& lines)
		{
			m_layer.ApplyToHighLevelILBody(func, lines);
		}

	private:
		ReturnHighlightRenderLayer m_layer;
	};

}  // namespace

TEST_F(ReturnHighlightTest, LLILReturnHighlighted)
{
	const Ref<Function> func = FindFunction("add");
	ASSERT_NE(func, nullptr) << "Could not find 'add' function";

	const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
	ASSERT_NE(llil, nullptr);

	const std::vector<DisassemblyTextLine> allLines = CollectLLILLines(llil);

	const auto* retLine = FindLineByAddr(allLines, AddRetAddr);
	ASSERT_NE(retLine, nullptr) << "Could not find return line at 0x10000042c";
	EXPECT_TRUE(IsHighlighted(*retLine)) << "Return line at 0x10000042c should be highlighted";

	const auto* nonRetLine = FindLineByAddr(allLines, AddNonRetAddr);
	ASSERT_NE(nonRetLine, nullptr) << "Could not find non-return line at 0x100000414";
	EXPECT_FALSE(IsHighlighted(*nonRetLine)) << "Non-return line at 0x100000414 should not be highlighted";
}

TEST_F(ReturnHighlightTest, MLILReturnHighlighted)
{
	const Ref<Function> func = FindFunction("add");
	ASSERT_NE(func, nullptr) << "Could not find 'add' function";

	const Ref<MediumLevelILFunction> mlil = func->GetMediumLevelIL();
	ASSERT_NE(mlil, nullptr);

	const std::vector<DisassemblyTextLine> allLines = CollectMLILLines(mlil);

	const auto* retLine = FindLineByAddr(allLines, AddRetAddr);
	ASSERT_NE(retLine, nullptr) << "Could not find return line at 0x10000042c";
	EXPECT_TRUE(IsHighlighted(*retLine)) << "Return line at 0x10000042c should be highlighted";

	const auto* nonRetLine = FindLineByAddr(allLines, AddNonRetAddr);
	ASSERT_NE(nonRetLine, nullptr) << "Could not find non-return line at 0x100000414";
	EXPECT_FALSE(IsHighlighted(*nonRetLine)) << "Non-return line at 0x100000414 should not be highlighted";
}

TEST_F(ReturnHighlightTest, HLILBlockReturnHighlighted)
{
	const Ref<Function> func = FindFunction("main");
	ASSERT_NE(func, nullptr) << "Could not find 'main' function";

	const Ref<HighLevelILFunction> hlil = func->GetHighLevelIL();
	ASSERT_NE(hlil, nullptr);

	const std::vector<DisassemblyTextLine> allLines = CollectHLILLines(hlil);

	const auto* retLine = FindLineByAddr(allLines, MainRetAddr);
	ASSERT_NE(retLine, nullptr) << "Could not find return line at 0x10000046c";
	EXPECT_TRUE(IsHighlighted(*retLine)) << "Return line at 0x10000046c should be highlighted";

	const auto* nonRetLine = FindLineByAddr(allLines, MainNonRetAddr);
	ASSERT_NE(nonRetLine, nullptr) << "Could not find non-return line at 0x100000454";
	EXPECT_FALSE(IsHighlighted(*nonRetLine)) << "Non-return line at 0x100000454 should not be highlighted";
}

TEST_F(ReturnHighlightTest, HLILBodyReturnHighlighted)
{
	const Ref<Function> func = FindFunction("main");
	ASSERT_NE(func, nullptr) << "Could not find 'main' function";

	const Ref<HighLevelILFunction> hlil = func->GetHighLevelIL();
	ASSERT_NE(hlil, nullptr);

	std::vector<LinearDisassemblyLine> linearLines = BuildLinearLines(func, hlil);
	ApplyToHLILBody(func, linearLines);

	const auto retLine = std::ranges::find_if(linearLines, [](const LinearDisassemblyLine& line) {
		return line.contents.addr == MainRetAddr;
	});
	ASSERT_NE(retLine, linearLines.end()) << "Could not find return line at 0x10000046c";
	EXPECT_TRUE(IsHighlighted(retLine->contents)) << "Return line at 0x10000046c should be highlighted";

	const auto nonRetLine = std::ranges::find_if(linearLines, [](const LinearDisassemblyLine& line) {
		return line.contents.addr == MainNonRetAddr;
	});
	ASSERT_NE(nonRetLine, linearLines.end()) << "Could not find non-return line at 0x100000454";
	EXPECT_FALSE(IsHighlighted(nonRetLine->contents)) << "Non-return line at 0x100000454 should not be highlighted";
}

TEST_F(ReturnHighlightTest, DisassemblyReturnHighlighted)
{
	const Ref<Function> func = FindFunction("add");
	ASSERT_NE(func, nullptr) << "Could not find 'add' function";

	const std::vector<DisassemblyTextLine> allLines = CollectDisassemblyLines(func);

	const auto* retLine = FindLineByAddr(allLines, AddRetAddr);
	ASSERT_NE(retLine, nullptr) << "Could not find return line at 0x10000042c";
	EXPECT_TRUE(IsHighlighted(*retLine)) << "Disassembly return at 0x10000042c should be highlighted";

	const auto* nonRetLine = FindLineByAddr(allLines, AddNonRetAddr);
	ASSERT_NE(nonRetLine, nullptr) << "Could not find non-return line at 0x100000414";
	EXPECT_FALSE(IsHighlighted(*nonRetLine)) << "Non-return line at 0x100000414 should not be highlighted";
}

TEST_F(ReturnHighlightTest, LLILNoreturnCallHighlighted)
{
	const Ref<Function> func = FindFunction("die");
	ASSERT_NE(func, nullptr) << "Could not find 'die' function";

	const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
	ASSERT_NE(llil, nullptr);

	const std::vector<DisassemblyTextLine> allLines = CollectLLILLines(llil);

	EXPECT_TRUE(AnyLineHighlightedAtAddr(allLines, DieCallExitAddr))
		<< "LLIL exit point at 0x100000444 should be highlighted";

	EXPECT_FALSE(AnyLineHighlightedAtAddr(allLines, DieNonExitAddr))
		<< "Non-exit line at 0x100000434 should not be highlighted";
}

TEST_F(ReturnHighlightTest, MLILNoreturnCallHighlighted)
{
	const Ref<Function> func = FindFunction("die");
	ASSERT_NE(func, nullptr) << "Could not find 'die' function";

	const Ref<MediumLevelILFunction> mlil = func->GetMediumLevelIL();
	ASSERT_NE(mlil, nullptr);

	const std::vector<DisassemblyTextLine> allLines = CollectMLILLines(mlil);

	EXPECT_TRUE(AnyLineHighlightedAtAddr(allLines, DieCallExitAddr))
		<< "MLIL exit point at 0x100000444 should be highlighted";
}

TEST_F(ReturnHighlightTest, HLILNoreturnCallHighlighted)
{
	const Ref<Function> func = FindFunction("die");
	ASSERT_NE(func, nullptr) << "Could not find 'die' function";

	const Ref<HighLevelILFunction> hlil = func->GetHighLevelIL();
	ASSERT_NE(hlil, nullptr);

	const std::vector<DisassemblyTextLine> allLines = CollectHLILLines(hlil);

	EXPECT_TRUE(AnyLineHighlightedAtAddr(allLines, DieCallExitAddr))
		<< "HLIL exit point at 0x100000444 should be highlighted";
}

TEST_F(ReturnHighlightTest, DisassemblyNoreturnCallHighlighted)
{
	const Ref<Function> func = FindFunction("die");
	ASSERT_NE(func, nullptr) << "Could not find 'die' function";

	const std::vector<DisassemblyTextLine> allLines = CollectDisassemblyLines(func);

	EXPECT_TRUE(AnyLineHighlightedAtAddr(allLines, DieCallExitAddr))
		<< "Disassembly exit point at 0x100000444 should be highlighted";

	EXPECT_FALSE(AnyLineHighlightedAtAddr(allLines, DieNonExitAddr))
		<< "Non-exit line at 0x100000434 should not be highlighted";
}

TEST_F(ReturnHighlightTest, SettingChangesHighlightColor)
{
	Settings::Instance()->Set("returnHighlighter.highlightColor", "red");

	const Ref<Function> func = FindFunction("add");
	ASSERT_NE(func, nullptr);

	const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
	ASSERT_NE(llil, nullptr);

	const std::vector<DisassemblyTextLine> allLines = CollectLLILLines(llil);

	const auto* retLine = FindLineByAddr(allLines, AddRetAddr);
	ASSERT_NE(retLine, nullptr);
	EXPECT_TRUE(IsHighlightedWithColor(*retLine, RedHighlightColor));
}

TEST_F(ReturnHighlightTest, InvalidColorFallsBackToLastValid)
{
	Settings::Instance()->Set("returnHighlighter.highlightColor", "red");

	const Ref<Function> func = FindFunction("add");
	ASSERT_NE(func, nullptr);

	const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
	ASSERT_NE(llil, nullptr);

	// First pass with "red" to establish last valid color
	CollectLLILLines(llil);

	// Now set an invalid color
	Settings::Instance()->Set("returnHighlighter.highlightColor", "invalid");

	const std::vector<DisassemblyTextLine> allLines = CollectLLILLines(llil);

	const auto* retLine = FindLineByAddr(allLines, AddRetAddr);
	ASSERT_NE(retLine, nullptr);
	EXPECT_TRUE(IsHighlightedWithColor(*retLine, RedHighlightColor));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(ReturnHighlightTest, HexColorAppliesCustomHighlight)
{
	Settings::Instance()->Set("returnHighlighter.highlightColor", "#FF5500");

	const Ref<Function> func = FindFunction("add");
	ASSERT_NE(func, nullptr);

	const Ref<LowLevelILFunction> llil = func->GetLowLevelIL();
	ASSERT_NE(llil, nullptr);

	const std::vector<DisassemblyTextLine> allLines = CollectLLILLines(llil);

	const auto* retLine = FindLineByAddr(allLines, AddRetAddr);
	ASSERT_NE(retLine, nullptr);
	EXPECT_EQ(retLine->highlight.style, CustomHighlightColor);
	EXPECT_EQ(retLine->highlight.r, 0xFF);
	EXPECT_EQ(retLine->highlight.g, 0x55);
	EXPECT_EQ(retLine->highlight.b, 0x00);
	EXPECT_EQ(retLine->highlight.alpha, FullAlpha);
}

auto main(int argc, char** argv) -> int
{
	::testing::InitGoogleTest(&argc, argv);
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
	::testing::AddGlobalTestEnvironment(new TestHelpers::BinaryNinjaEnvironment());
	return RUN_ALL_TESTS();
}
