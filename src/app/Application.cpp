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
    ConfigManager::instance().load();
    ThemeManager::instance().applyToApplication();
}

Application::~Application()
{
    ConfigManager::instance().save();
}
