#include "BinjaReturnHighlighter/ColorPickerAction.hpp"

#include <string>

#include <QColor>
#include <QColorDialog>
#include <QMainWindow>
#include <QString>
#include <QWidget>

#include <binaryninjaapi.h>
#include <action.h>
#include <uicontext.h>

#include "BinjaReturnHighlighter/ColorDefs.hpp"

using BinaryNinja::Settings;

namespace {
	QColor NameToQColor(const std::string& name)
	{
		if (auto def = FindColorByName(name))
		{
			return {(*def)->r, (*def)->g, (*def)->b};
		}
		const auto& fallback = ColorDefs.front();
		return {fallback.r, fallback.g, fallback.b};
	}

	QColor CurrentSettingToQColor()
	{
		const std::string colorSetting = Settings::Instance()->Get<std::string>("returnHighlighter.highlightColor");

		if (!colorSetting.empty() && colorSetting.front() == '#')
		{
			const QColor color(QString::fromStdString(colorSetting));
			if (color.isValid())
			{
				return color;
			}
		}

		return NameToQColor(colorSetting);
	}

	constexpr const char* ActionNameStr = "Return Highlighter\\Choose Color...";
}  // namespace

void RegisterColorPickerAction()
{
	const QString actionName(ActionNameStr);

	UIAction::registerAction(actionName);

	UIActionHandler::globalActions()->bindAction(actionName, UIAction([] {
		QWidget* parent = nullptr;
		UIContext* ctx = UIContext::activeContext();
		if (ctx != nullptr)
		{
			parent = ctx->mainWindow();
		}

		const QColor initial = CurrentSettingToQColor();
		const QColor chosen = QColorDialog::getColor(initial, parent, "Return Highlighter Color");

		if (chosen.isValid())
		{
			const std::string hex = chosen.name().toStdString();  // "#rrggbb"
			Settings::Instance()->Set("returnHighlighter.highlightColor", hex);
		}
	}));

	Menu::mainMenu("Plugins")->addAction(actionName, "Choose Color");
}
