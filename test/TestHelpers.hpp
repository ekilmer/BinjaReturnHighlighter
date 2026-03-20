#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <binaryninjaapi.h>
#include <binaryninjacore.h>

#include <gtest/gtest.h>

namespace TestHelpers {

	constexpr uint8_t FullAlpha = 255;

	BinaryNinja::Ref<BinaryNinja::BinaryView>& GlobalBv();

	BinaryNinja::Ref<BinaryNinja::Function> FindFunctionByName(const std::string& name);

	inline bool IsHighlightedWithColor(
		const BinaryNinja::DisassemblyTextLine& line, BNHighlightStandardColor expectedColor)
	{
		return line.highlight.style == StandardHighlightColor && line.highlight.color == expectedColor
			&& line.highlight.alpha == FullAlpha;
	}

	inline bool IsHighlighted(const BinaryNinja::DisassemblyTextLine& line)
	{
		return IsHighlightedWithColor(line, BlueHighlightColor);
	}

	const BinaryNinja::DisassemblyTextLine* FindLineByAddr(
		const std::vector<BinaryNinja::DisassemblyTextLine>& lines, uint64_t addr);

	bool AnyLineHighlightedAtAddr(const std::vector<BinaryNinja::DisassemblyTextLine>& lines, uint64_t addr);

	std::vector<BinaryNinja::LinearDisassemblyLine> BuildLinearLines(
		const BinaryNinja::Ref<BinaryNinja::Function>& func,
		const BinaryNinja::Ref<BinaryNinja::HighLevelILFunction>& hlil);

	class BinaryNinjaEnvironment : public ::testing::Environment
	{
	public:
		void SetUp() override;
		void TearDown() override;
	};

}  // namespace TestHelpers
