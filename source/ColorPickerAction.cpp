#include "BinjaReturnHighlighter/ColorPickerAction.hpp"

#include <string>

#include <QColor>
#include <QColorDialog>
#include <QDialog>
#include <QMainWindow>
#include <QObject>
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
		if (const auto* def = FindColorByName(name))
		{
			return {def->r, def->g, def->b};
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

		const std::string originalSetting = Settings::Instance()->Get<std::string>("returnHighlighter.highlightColor");
		const QColor initial = CurrentSettingToQColor();

		QColorDialog dialog(initial, parent);
		dialog.setWindowTitle("Return Highlighter Color");
		dialog.setOption(QColorDialog::DontUseNativeDialog);

		QObject::connect(&dialog, &QColorDialog::currentColorChanged, [](const QColor& color) {
			if (color.isValid())
			{
				const std::string hex = color.name().toStdString();
				Settings::Instance()->Set("returnHighlighter.highlightColor", hex);

				if (UIContext* activeCtx = UIContext::activeContext())
				{
					activeCtx->refreshCurrentViewContents();
				}
			}
		});

		if (dialog.exec() == QDialog::Accepted)
		{
			const QColor chosen = dialog.selectedColor();
			if (chosen.isValid())
			{
				const std::string hex = chosen.name().toStdString();
				Settings::Instance()->Set("returnHighlighter.highlightColor", hex);
			}
		}
		else
		{
			// User cancelled — restore original color
			Settings::Instance()->Set("returnHighlighter.highlightColor", originalSetting);
		}

		if (UIContext* activeCtx = UIContext::activeContext())
		{
			activeCtx->refreshCurrentViewContents();
		}
	}));

	Menu::mainMenu("Plugins")->addAction(actionName, "Choose Color");
}
