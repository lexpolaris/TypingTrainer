// src/ui/controllers/ThemeController.cpp
#include "ThemeController.h"

#include "ui/SettingsDialog.h"
#include "app/ConfigManager.h"
#include "theme/ThemeManager.h"

ThemeController::ThemeController(QObject* parent) : QObject(parent)
{
}

void ThemeController::applyConfigToUi(const QFont& hostDefaultFont)
{
    auto& cfg = ConfigManager::instance();

    // 主题模式
    const QString tm = cfg.themeMode();
    ThemeManager::Mode m = ThemeManager::System;
    if (tm == "light") m = ThemeManager::Light;
    else if (tm == "dark") m = ThemeManager::Dark;
    ThemeManager::instance().setMode(m);
    emit themeModeChanged(static_cast<int>(m));

    // 自定义颜色
    ThemeManager::instance().setCustomColors(cfg.customThemeColors());

    // 字体：配置为空时回退到宿主默认字体（等价于原 QWidget::font()）
    QFont f = cfg.typingFont();
    if (f.family().isEmpty()) {
        f = hostDefaultFont;
        f.setPointSize(18);
    }
    applyTypingFont(f);
}

void ThemeController::applyTypingFont(const QFont& f)
{
    m_typingFont = f;
    emit fontChanged(f);
}

void ThemeController::openSettings(QWidget* parent)
{
    SettingsDialog dlg(parent);

    // 实时预览
    connect(&dlg, &SettingsDialog::fontPreview,
            this, &ThemeController::applyTypingFont);
    connect(&dlg, &SettingsDialog::themeColorsPreview,
            this, [](const QHash<int, QColor>& colors) {
        ThemeManager::instance().setCustomColors(colors);
    });

    dlg.exec();
}

void ThemeController::setThemeMode(int mode)
{
    auto m = static_cast<ThemeManager::Mode>(mode);
    ThemeManager::instance().setMode(m);
    emit themeModeChanged(mode);
    saveThemeModeToConfig();
}

void ThemeController::saveThemeModeToConfig()
{
    auto& cfg = ConfigManager::instance();
    auto m = ThemeManager::instance().mode();
    cfg.set("general.themeMode",
            m == ThemeManager::Light ? "light" :
            m == ThemeManager::Dark  ? "dark"  : "system");
    cfg.save();
}