#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include <binaryninjacore.h>

struct ColorDef
{
	std::string_view name;
	uint8_t r, g, b;
	BNHighlightStandardColor bnColor;
};

constexpr std::array<ColorDef, 9> ColorDefs = {{
	{.name = "blue", .r = 0, .g = 0, .b = 255, .bnColor = BlueHighlightColor},
	{.name = "green", .r = 0, .g = 255, .b = 0, .bnColor = GreenHighlightColor},
	{.name = "cyan", .r = 0, .g = 255, .b = 255, .bnColor = CyanHighlightColor},
	{.name = "red", .r = 255, .g = 0, .b = 0, .bnColor = RedHighlightColor},
	{.name = "magenta", .r = 255, .g = 0, .b = 255, .bnColor = MagentaHighlightColor},
	{.name = "yellow", .r = 255, .g = 255, .b = 0, .bnColor = YellowHighlightColor},
	{.name = "orange", .r = 255, .g = 165, .b = 0, .bnColor = OrangeHighlightColor},
	{.name = "white", .r = 255, .g = 255, .b = 255, .bnColor = WhiteHighlightColor},
	{.name = "black", .r = 0, .g = 0, .b = 0, .bnColor = BlackHighlightColor},
}};

constexpr const ColorDef* FindColorByName(std::string_view name)
{
	for (const auto& def : ColorDefs)
	{
		if (def.name == name)
		{
			return &def;
		}
	}
	return nullptr;
}
