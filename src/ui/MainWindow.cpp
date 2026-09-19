// src/ui/MainWindow.cpp
#include "MainWindow.h"

#include "PacmanView.h"
#include "TwoLineView.h"
#include "CodeHintPanel.h"
#include "TextLibraryDialog.h"
#include "SpeedPointDialog.h"
#include "SpeedChartDialog.h"

#include "core/TextDocument.h"
#include "core/TypingSession.h"
#include "core/CodeTable.h"

#include "app/ConfigManager.h"
#include "theme/ThemeManager.h"
#include "utils/AppPaths.h"
#include "utils/TextLoader.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    m_doc = new TextDocument(this);
    m_session = new TypingSession(this);
    m_codeTable = new CodeTable();

    setupUi();
    setupMenus();
    setupStatusBar();

    connect(m_session, &TypingSession::positionChanged,
            this, &MainWindow::updateStats);
    connect(m_session, &TypingSession::stateChanged,
            this, [this](TypingSession::State) { updateStats(); });

    applyConfigToUi();

    // 加载内置示例文本
    loadResourceText(":/texts/岳阳楼记.txt");

    // 启动后延迟聚焦视图，确保窗口已显示
    QTimer::singleShot(0, this, &MainWindow::ensureViewFocus);
}

MainWindow::~MainWindow()
{
    delete m_codeTable;
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 视图（唯一内容区）
    m_view = new PacmanView(central);
    m_view->setSession(m_session);
    m_view->setDocument(m_doc);
    root->addWidget(m_view, 1);

    setCentralWidget(central);

    // 编码提示（浮动，不加入布局，父对象设为主窗口）
    m_codeHint = new CodeHintPanel(this);
    m_codeHint->hide();

    // 信号连接
    connect(m_view, &TypingView::codeHintRequested,
            this, &MainWindow::onCodeHintRequested);
    connect(m_view, &TypingView::codeHintCleared,
            m_codeHint, &CodeHintPanel::clear);
}

void MainWindow::ensureViewFocus()
{
    if (m_view) {
        m_view->setFocus(Qt::OtherFocusReason);
    }
}

void MainWindow::setupMenus()
{
    // ---------- 文件 ----------
    auto* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("打开文本..."), QKeySequence::Open,
                        this, &MainWindow::onOpenText);
    fileMenu->addAction(tr("文本库..."), QKeySequence("Ctrl+L"),
                        this, &MainWindow::onOpenTextLibrary);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), QKeySequence::Quit,
                        this, &QWidget::close);

    // ---------- 模式 ----------
    auto* modeMenu = menuBar()->addMenu(tr("模式(&M)"));
    auto* modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);

    auto* actPacman = modeMenu->addAction(tr("吃豆人模式"));
    actPacman->setCheckable(true);
    actPacman->setShortcut(QKeySequence("F1"));
    actPacman->setChecked(true);
    modeGroup->addAction(actPacman);
    connect(actPacman, &QAction::triggered,
            this, &MainWindow::onSwitchModePacman);

    auto* actTwoLine = modeMenu->addAction(tr("双行对照模式"));
    actTwoLine->setCheckable(true);
    actTwoLine->setShortcut(QKeySequence("F2"));
    modeGroup->addAction(actTwoLine);
    connect(actTwoLine, &QAction::triggered,
            this, &MainWindow::onSwitchModeTwoLine);

    // ---------- 码表 ----------
    auto* codeMenu = menuBar()->addMenu(tr("码表(&C)"));
    codeMenu->addAction(tr("五笔86"), this,
                        [this] { loadBuiltinCodeTable("wubi86"); });
    codeMenu->addAction(tr("五笔98"), this,
                        [this] { loadBuiltinCodeTable("wubi98"); });
    codeMenu->addAction(tr("郑码"),   this,
                        [this] { loadBuiltinCodeTable("zhengma"); });
    codeMenu->addSeparator();
    codeMenu->addAction(tr("导入码表..."), this, &MainWindow::onImportCodeTable);

    // ---------- 测速 ----------
    auto* speedMenu = menuBar()->addMenu(tr("测速(&S)"));
    speedMenu->addAction(tr("设置测速点..."), QKeySequence("Ctrl+Shift+S"),
                         this, &MainWindow::onSpeedPointSettings);
    speedMenu->addAction(tr("查看测速结果..."), QKeySequence("Ctrl+Shift+R"),
                         this, &MainWindow::onShowSpeedChart);

    // ---------- 主题 ----------
    auto* themeMenu = menuBar()->addMenu(tr("主题(&T)"));
    auto* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    auto* actSys = themeMenu->addAction(tr("跟随系统"));
    auto* actLight = themeMenu->addAction(tr("亮色"));
    auto* actDark = themeMenu->addAction(tr("暗色"));
    for (auto* a : {actSys, actLight, actDark}) {
        a->setCheckable(true);
        themeGroup->addAction(a);
    }
    auto updateChecks = [=](ThemeManager::Mode m) {
        actSys->setChecked(m == ThemeManager::System);
        actLight->setChecked(m == ThemeManager::Light);
        actDark->setChecked(m == ThemeManager::Dark);
    };
    updateChecks(ThemeManager::instance().mode());

    connect(actSys, &QAction::triggered, this, [=] {
        ThemeManager::instance().setMode(ThemeManager::System);
        updateChecks(ThemeManager::System);
    });
    connect(actLight, &QAction::triggered, this, [=] {
        ThemeManager::instance().setMode(ThemeManager::Light);
        updateChecks(ThemeManager::Light);
    });
    connect(actDark, &QAction::triggered, this, [=] {
        ThemeManager::instance().setMode(ThemeManager::Dark);
        updateChecks(ThemeManager::Dark);
    });

    themeMenu->addSeparator();
    themeMenu->addAction(tr("自定义当前字符颜色..."), this, [this] {
        QMessageBox::information(this, tr("提示"),
            tr("自定义接口已就绪，可在 ThemeManager 中调用 setCustomColor()"));
    });
    themeMenu->addAction(tr("清除所有自定义颜色"), this, [] {
        ThemeManager::instance().clearAllCustomColors();
    });
}

void MainWindow::setupStatusBar()
{
    m_statusSpeed    = new QLabel(this);
    m_statusKey      = new QLabel(this);
    m_statusCode     = new QLabel(this);
    m_statusProgress = new QLabel(this);
    statusBar()->addWidget(m_statusProgress, 1);
    statusBar()->addPermanentWidget(m_statusSpeed);
    statusBar()->addPermanentWidget(m_statusKey);
    statusBar()->addPermanentWidget(m_statusCode);
    updateStats();
}

void MainWindow::applyConfigToUi()
{
    auto& cfg = ConfigManager::instance();

    // 主题
    QString tm = cfg.themeMode();
    ThemeManager::Mode m = ThemeManager::System;
    if (tm == "light") m = ThemeManager::Light;
    else if (tm == "dark") m = ThemeManager::Dark;
    ThemeManager::instance().setMode(m);

    // 字体
    QFont f = font();
    if (!cfg.fontFamily().isEmpty()) f.setFamily(cfg.fontFamily());
    f.setPointSize(cfg.fontPointSize());
    m_view->setTypingFont(f);
}

void MainWindow::saveConfigFromUi()
{
    auto& cfg = ConfigManager::instance();
    auto m = ThemeManager::instance().mode();
    cfg.set("general.themeMode",
            m == ThemeManager::Light ? "light" :
            m == ThemeManager::Dark  ? "dark"  : "system");
    cfg.save();
}

// ---------------------------------------------------------------
// 文件
// ---------------------------------------------------------------
void MainWindow::onOpenText()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("打开文本"), AppPaths::textDir(),
        tr("文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return;
    loadText(path);
    ConfigManager::instance().set("recent.lastTextPath", path);
    ConfigManager::instance().save();
}

void MainWindow::onOpenTextLibrary()
{
    TextLibraryDialog dlg(this);
    connect(&dlg, &TextLibraryDialog::textChosen, this,
            [this](const QString& path) {
        if (path.startsWith(":/"))
            loadResourceText(path);
        else
            loadText(path);
    });
    dlg.exec();
    ensureViewFocus();
}

void MainWindow::loadText(const QString& path)
{
    QString err;
    if (!m_doc->loadFromFile(path, &err)) {
        QMessageBox::warning(this, tr("打开失败"), err);
        return;
    }
    m_session->start(m_doc->text());
    setWindowTitle(tr("打字练习 - %1").arg(m_doc->name()));
    m_view->setDocument(m_doc);
    m_view->update();
    updateStats();
    ensureViewFocus();
}

void MainWindow::loadResourceText(const QString& resPath)
{
    QString err;
    QString text = TextLoader::loadResource(resPath, &err);
    if (text.isEmpty()) {
        QMessageBox::warning(this, tr("加载失败"), err);
        return;
    }
    m_doc->loadFromString(text, QFileInfo(resPath).fileName());
    m_session->start(text);
    setWindowTitle(tr("打字练习 - %1").arg(m_doc->name()));
    m_view->setDocument(m_doc);
    m_view->update();
    updateStats();
    ensureViewFocus();
}

// ---------------------------------------------------------------
// 模式切换
// ---------------------------------------------------------------
void MainWindow::switchMode(bool pacman)
{
    if (!m_view) return;

    // 判断当前是否已经是目标模式，避免无谓重建
    bool currentlyPacman = qobject_cast<PacmanView*>(m_view) != nullptr;
    if (pacman == currentlyPacman) {
        ensureViewFocus();
        return;
    }

    TypingView* newView = pacman
        ? static_cast<TypingView*>(new PacmanView(this))
        : static_cast<TypingView*>(new TwoLineView(this));

    newView->setSession(m_session);
    newView->setDocument(m_doc);
    newView->setCodeTable(m_codeTable);
    newView->setTypingFont(m_view->typingFont());

    auto* central = centralWidget();
    auto* layout = qobject_cast<QVBoxLayout*>(central->layout());
    layout->replaceWidget(m_view, newView);

    TypingView* old = m_view;
    m_view = newView;
    old->deleteLater();

    connect(m_view, &TypingView::codeHintRequested,
            this, &MainWindow::onCodeHintRequested);

    ensureViewFocus();
}

void MainWindow::onSwitchModePacman()
{
    switchMode(true);
}

void MainWindow::onSwitchModeTwoLine()
{
    switchMode(false);
}

// ---------------------------------------------------------------
// 码表
// ---------------------------------------------------------------
void MainWindow::loadBuiltinCodeTable(const QString& name)
{
    QString path = QString(":/tables/%1.txt").arg(name);
    QString err;
    if (!m_codeTable->loadFromFile(path, &err)) {
        QMessageBox::information(this, tr("码表"),
            tr("内置码表 %1 尚未提供。\n请通过\"导入码表\"加载。").arg(name));
        return;
    }
    m_view->setCodeTable(m_codeTable);
    m_codeHint->setCodeTable(m_codeTable);
    statusBar()->showMessage(tr("已加载码表: %1").arg(m_codeTable->name()), 3000);
    ensureViewFocus();
}

void MainWindow::onImportCodeTable()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("导入码表"), AppPaths::codeTableDir(),
        tr("码表 (*.txt *.mb);;所有文件 (*)"));
    if (path.isEmpty()) return;
    QString err;
    if (!m_codeTable->loadFromFile(path, &err)) {
        QMessageBox::warning(this, tr("导入失败"), err);
        return;
    }
    m_view->setCodeTable(m_codeTable);
    m_codeHint->setCodeTable(m_codeTable);
    statusBar()->showMessage(tr("已导入码表: %1").arg(m_codeTable->name()), 3000);
    ensureViewFocus();
}

// ---------------------------------------------------------------
// 测速
// ---------------------------------------------------------------
void MainWindow::onSpeedPointSettings()
{
    if (!m_doc || m_doc->isEmpty()) {
        QMessageBox::information(this, tr("测速点"), tr("请先加载文本。"));
        return;
    }

    SpeedPointDialog dlg(m_doc->text(), this);
    if (dlg.exec() != QDialog::Accepted) {
        ensureViewFocus();
        return;
    }

    QVector<int> pts = dlg.selectedPoints();
    m_session->setSpeedPoints(pts);
    statusBar()->showMessage(tr("已设置 %1 个测速点").arg(pts.size()), 3000);
    ensureViewFocus();
}

void MainWindow::onShowSpeedChart()
{
    if (!m_session || m_session->currentIndex() == 0) {
        QMessageBox::information(this, tr("测速结果"), tr("尚未开始跟打。"));
        return;
    }

    SpeedChartDialog dlg(m_session, m_doc->text(), this);
    connect(&dlg, &SpeedChartDialog::requestRetry, this,
            [this](const QString& segText) {
        startSessionForText(segText, m_doc->name() + " (重打段)");
    });
    dlg.exec();
    ensureViewFocus();
}

void MainWindow::startSessionForText(const QString& targetText,
                                     const QString& name)
{
    m_doc->loadFromString(targetText, name);
    m_session->setSpeedPoints({});
    m_session->start(targetText);
    m_view->setDocument(m_doc);
    m_view->update();
    setWindowTitle(tr("打字练习 - %1").arg(name));
    updateStats();
    ensureViewFocus();
}

// ---------------------------------------------------------------
// 编码提示 & 状态栏
// ---------------------------------------------------------------
void MainWindow::onCodeHintRequested(QChar current, QChar next)
{
    m_codeHint->showFor(current, next);
}

void MainWindow::updateStats()
{
    if (!m_session) return;
    m_statusSpeed->setText(tr("速度: %1 字/分")
        .arg(m_session->speedCPM(), 0, 'f', 1));
    m_statusKey->setText(tr("击键: %1  码长: %2")
        .arg(m_session->keystrokePerSec(), 0, 'f', 1)
        .arg(m_session->codeLength(), 0, 'f', 2));
    m_statusCode->setText(tr("错字: %1  回改: %2")
        .arg(m_session->errorChars())
        .arg(m_session->backspaceCount()));
    m_statusProgress->setText(tr("进度: %1 / %2")
        .arg(m_session->currentIndex())
        .arg(m_session->totalLength()));
}