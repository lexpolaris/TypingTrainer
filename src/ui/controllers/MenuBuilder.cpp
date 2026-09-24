// src/ui/controllers/MenuBuilder.cpp
#include "MenuBuilder.h"

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QKeySequence>

MenuBuilder::MenuBuilder(QMainWindow* window, QObject* parent)
    : QObject(parent), m_window(window)
{
}

void MenuBuilder::build()
{
    // ---------- 文件 ----------
    auto* fileMenu = m_window->menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("打开文本..."), QKeySequence::Open,
                        this, &MenuBuilder::openTextRequested);
    fileMenu->addAction(tr("文本库..."), QKeySequence("Ctrl+L"),
                        this, &MenuBuilder::openTextLibraryRequested);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("设置..."), QKeySequence("Ctrl+,"),
                        this, &MenuBuilder::openSettingsRequested);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), QKeySequence::Quit,
                        m_window, &QWidget::close);

    // ---------- 模式 ----------
    auto* modeMenu = m_window->menuBar()->addMenu(tr("模式(&M)"));
    auto* modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);

    auto* actPacman = modeMenu->addAction(tr("吃豆人模式"));
    actPacman->setCheckable(true);
    actPacman->setShortcut(QKeySequence("F1"));
    actPacman->setChecked(true);
    modeGroup->addAction(actPacman);
    connect(actPacman, &QAction::triggered,
            this, &MenuBuilder::modePacmanRequested);

    auto* actTwoLine = modeMenu->addAction(tr("双行对照模式"));
    actTwoLine->setCheckable(true);
    actTwoLine->setShortcut(QKeySequence("F2"));
    modeGroup->addAction(actTwoLine);
    connect(actTwoLine, &QAction::triggered,
            this, &MenuBuilder::modeTwoLineRequested);

    // ---------- 跟打 ----------
    auto* typeMenu = m_window->menuBar()->addMenu(tr("跟打(&T)"));
    typeMenu->addAction(tr("重打当前段"), QKeySequence("F3"),
                        this, &MenuBuilder::retryRequested);
    typeMenu->addSeparator();
    auto* actShuffle = typeMenu->addAction(tr("乱序模式"));
    actShuffle->setCheckable(true);
    connect(actShuffle, &QAction::toggled,
            this, &MenuBuilder::shuffleToggled);
    typeMenu->addSeparator();
    typeMenu->addAction(tr("错字列表..."), QKeySequence("Ctrl+M"),
                        this, &MenuBuilder::showMistakesRequested);
    typeMenu->addSeparator();
    typeMenu->addAction(tr("历史成绩..."), QKeySequence("Ctrl+H"),
                        this, &MenuBuilder::showHistoryRequested);

    // ---------- 码表 ----------
    auto* codeMenu = m_window->menuBar()->addMenu(tr("码表(&C)"));
    codeMenu->addAction(tr("五笔86"), this,
                        [this] { emit builtinCodeTableRequested("wubi86"); });
    codeMenu->addAction(tr("五笔98"), this,
                        [this] { emit builtinCodeTableRequested("wubi98"); });
    codeMenu->addAction(tr("郑码"),   this,
                        [this] { emit builtinCodeTableRequested("zhengma"); });
    codeMenu->addSeparator();
    codeMenu->addAction(tr("导入码表..."), this,
                        &MenuBuilder::importCodeTableRequested);

    // ---------- 测速 ----------
    auto* speedMenu = m_window->menuBar()->addMenu(tr("测速(&S)"));
    speedMenu->addAction(tr("设置测速点..."), QKeySequence("Ctrl+Shift+S"),
                         this, &MenuBuilder::speedPointSettingsRequested);
    speedMenu->addAction(tr("查看测速结果..."), QKeySequence("Ctrl+Shift+R"),
                         this, &MenuBuilder::showSpeedChartRequested);

    // ---------- 主题 ----------
    auto* themeMenu = m_window->menuBar()->addMenu(tr("主题(&T)"));
    auto* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    m_actThemeSystem = themeMenu->addAction(tr("跟随系统"));
    m_actThemeLight  = themeMenu->addAction(tr("亮色"));
    m_actThemeDark   = themeMenu->addAction(tr("暗色"));
    for (auto* a : {m_actThemeSystem, m_actThemeLight, m_actThemeDark}) {
        a->setCheckable(true);
        themeGroup->addAction(a);
    }
    connect(m_actThemeSystem, &QAction::triggered, this,
            [this] { emit themeModeRequested(ThemeSystem); });
    connect(m_actThemeLight, &QAction::triggered, this,
            [this] { emit themeModeRequested(ThemeLight); });
    connect(m_actThemeDark, &QAction::triggered, this,
            [this] { emit themeModeRequested(ThemeDark); });

    themeMenu->addSeparator();
    themeMenu->addAction(tr("自定义当前字符颜色..."), this,
                         &MenuBuilder::customCurrentColorRequested);
    themeMenu->addAction(tr("清除所有自定义颜色"), this,
                         &MenuBuilder::clearCustomColorsRequested);

    // ---------- 音乐 ----------
    auto* musicMenu = m_window->menuBar()->addMenu(tr("音乐(&B)"));
    musicMenu->addAction(tr("音乐库..."), QKeySequence("Ctrl+Shift+M"),
                         this, &MenuBuilder::openMusicLibraryRequested);
    musicMenu->addSeparator();
    musicMenu->addAction(tr("播放/暂停"), QKeySequence("Ctrl+P"),
                         this, &MenuBuilder::playPauseMusicRequested);
    musicMenu->addAction(tr("下一首"), QKeySequence("Ctrl+Right"),
                         this, &MenuBuilder::nextMusicRequested);
    musicMenu->addAction(tr("上一首"), QKeySequence("Ctrl+Left"),
                         this, &MenuBuilder::prevMusicRequested);
}

void MenuBuilder::setThemeMode(ThemeMode mode)
{
    if (m_actThemeSystem) m_actThemeSystem->setChecked(mode == ThemeSystem);
    if (m_actThemeLight)  m_actThemeLight->setChecked(mode == ThemeLight);
    if (m_actThemeDark)   m_actThemeDark->setChecked(mode == ThemeDark);
}
