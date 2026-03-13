#include "BinjaReturnHighlighter/ColorPickerAction.hpp"

#include <binaryninjacore.h>
#include <uitypes.h>

extern "C"
{
	BN_DECLARE_UI_ABI_VERSION

	BINARYNINJAPLUGIN bool UIPluginInit()
	{
		RegisterColorPickerAction();
		return true;
	}
}
