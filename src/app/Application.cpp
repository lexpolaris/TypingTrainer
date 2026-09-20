// src/app/Application.cpp
#include "Application.h"
#include "ConfigManager.h"
#include "theme/ThemeManager.h"
#include "utils/AppPaths.h"

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setApplicationName("TypingTrainer");
    setOrganizationName("TypingTrainer");
    setApplicationVersion("0.1.0");

    AppPaths::ensureDirsExist();

    // 加载配置 → 应用主题
    // ConfigManager 构造时已 load()；这里显式读取主题模式并应用
    auto& cfg = ConfigManager::instance();
    const QString tm = cfg.themeMode();
    ThemeManager::Mode mode = ThemeManager::System;
    if (tm == "light")      mode = ThemeManager::Light;
    else if (tm == "dark")  mode = ThemeManager::Dark;
    ThemeManager::instance().setMode(mode);
    ThemeManager::instance().setCustomColors(cfg.customThemeColors());
    ThemeManager::instance().applyToApplication();
}

Application::~Application()
{
    ConfigManager::instance().save();
}
