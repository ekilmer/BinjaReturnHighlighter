#include <BinjaReturnHighlighter/ReturnHighlightRenderLayer.hpp>

#include <binaryninjacore.h>
#include <binaryninjaapi.h>

using BinaryNinja::RenderLayer;

extern "C"
{
	BN_DECLARE_CORE_ABI_VERSION

	BINARYNINJAPLUGIN bool CorePluginInit()
	{
		static auto* layer = new ReturnHighlightRenderLayer();  // NOLINT(cppcoreguidelines-owning-memory)
		RenderLayer::Register(layer, DisabledByDefaultRenderLayerDefaultEnableState);
		return true;
	}
}
