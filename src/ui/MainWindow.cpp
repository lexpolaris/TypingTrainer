// src/ui/MainWindow.cpp
#include "MainWindow.h"

#include "PacmanView.h"
#include "TwoLineView.h"
#include "CodeHintPanel.h"
#include "TextLibraryDialog.h"
#include "SpeedPointDialog.h"
#include "SpeedChartDialog.h"
#include "SettingsDialog.h"
#include "MistakeDialog.h"
#include "MusicLibraryDialog.h"
#include "HistoryDialog.h"

#include "core/TextDocument.h"
#include "core/TypingSession.h"
#include "core/CodeTable.h"
#include "core/TextShuffler.h"
#include "core/TextFilter.h"
#include "core/HistoryDb.h"

#include "app/ConfigManager.h"
#include "app/MusicPlayer.h"
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
#include <QColorDialog>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QRandomGenerator>
#include <QToolButton>
#include <QStyle>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    m_doc = new TextDocument(this);
    m_session = new TypingSession(this);
    m_codeTable = new CodeTable();

    setupUi();
    setupMenus();
    setupStatusBar();
    setupMusicPlayer();

    connect(m_session, &TypingSession::positionChanged,
            this, &MainWindow::updateStats);
    connect(m_session, &TypingSession::stateChanged,
            this, [this](TypingSession::State) { updateStats(); });
    connect(m_session, &TypingSession::mistakeAdded,
            this, [this](int) { updateStats(); });
    // 记录断点：会话结束时保存当前位置
    connect(m_session, &TypingSession::finished, this, [this]() {
        if (m_docName.isEmpty()) return;
        auto& cfg = ConfigManager::instance();
        cfg.setLastReadPosition(m_docName, m_session->currentIndex());
        cfg.save();
    });
    connect(m_session, &TypingSession::finished, this, [this]() {
        saveHistoryEntry();
    });

    auto& cfg = ConfigManager::instance();
    applyConfigToUi();

    // 启动时按配置自动加载码表
    const QString autoTable = ConfigManager::instance().autoLoadCodeTablePath();
    if (!autoTable.isEmpty()) {
        QString err;
        if (m_codeTable->loadFromFile(autoTable, &err)) {
            m_view->setCodeTable(m_codeTable);
            m_codeHint->setCodeTable(m_codeTable);
            statusBar()->showMessage(
                tr("已自动加载码表: %1").arg(m_codeTable->name()), 3000);
        } else {
            qWarning() << "自动加载码表失败:" << autoTable << err;
        }
    }
   
    if (cfg.loadLastTextOnStartup()) {
        const QString lastPath = cfg.lastTextPath();
        if (!lastPath.isEmpty() && QFile::exists(lastPath)) {
            loadText(lastPath);
        }
    }

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
    connect(m_view, &TypingView::requestRetry,
            this, &MainWindow::onRetry);
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
    fileMenu->addAction(tr("设置..."), QKeySequence("Ctrl+,"),
                        this, &MainWindow::onOpenSettings);
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
    
    // ---------- 跟打 ----------
    auto* typeMenu = menuBar()->addMenu(tr("跟打(&T)"));
    typeMenu->addAction(tr("重打当前段"), QKeySequence("F3"),
                        this, &MainWindow::onRetry);
    typeMenu->addSeparator();
    auto* actShuffle = typeMenu->addAction(tr("乱序模式"));
    actShuffle->setCheckable(true);
    connect(actShuffle, &QAction::toggled,
            this, &MainWindow::onToggleShuffle);
    typeMenu->addSeparator();
    typeMenu->addAction(tr("错字列表..."), QKeySequence("Ctrl+M"),
                        this, &MainWindow::onShowMistakes);
    typeMenu->addSeparator();
    typeMenu->addAction(tr("历史成绩..."), QKeySequence("Ctrl+H"),
                        this, [this]() {
        HistoryDialog dlg(this);
        dlg.exec();
        ensureViewFocus();
    });

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
        saveConfigFromUi();
    });
    connect(actLight, &QAction::triggered, this, [=] {
        ThemeManager::instance().setMode(ThemeManager::Light);
        updateChecks(ThemeManager::Light);
        saveConfigFromUi();
    });
    connect(actDark, &QAction::triggered, this, [=] {
        ThemeManager::instance().setMode(ThemeManager::Dark);
        updateChecks(ThemeManager::Dark);
        saveConfigFromUi();
    });

    themeMenu->addSeparator();
    themeMenu->addAction(tr("自定义当前字符颜色..."), this, [this] {
        const auto role = ThemeManager::Current;
        QColor current = ThemeManager::instance().color(role);
        QColor chosen = QColorDialog::getColor(
            current, this, tr("选择当前字符颜色"));
        if (!chosen.isValid()) return;

        ThemeManager::instance().setCustomColor(role, chosen);

        auto& cfg = ConfigManager::instance();
        cfg.setCustomThemeColors(ThemeManager::instance().customColors());
        cfg.save();
    });
    themeMenu->addAction(tr("清除所有自定义颜色"), this, [] {
        ThemeManager::instance().clearAllCustomColors();
        ConfigManager::instance().setCustomThemeColors({});
        ConfigManager::instance().save();
        ThemeManager::instance().applyToApplication();
    });

    // ---------- 音乐 ----------
    auto* musicMenu = menuBar()->addMenu(tr("音乐(&B)"));
    musicMenu->addAction(tr("音乐库..."), QKeySequence("Ctrl+Shift+M"),
                        this, &MainWindow::onOpenMusicLibrary);
    musicMenu->addSeparator();
    musicMenu->addAction(tr("播放/暂停"), QKeySequence("Ctrl+P"),
                        this, &MainWindow::onPlayPauseMusic);
    musicMenu->addAction(tr("下一首"), QKeySequence("Ctrl+Right"),
                        this, &MainWindow::onNextMusic);
    musicMenu->addAction(tr("上一首"), QKeySequence("Ctrl+Left"),
                        this, &MainWindow::onPrevMusic);
}

void MainWindow::setupStatusBar()
{
    m_statusSpeed    = new QLabel(this);
    m_statusKey      = new QLabel(this);
    m_statusCode     = new QLabel(this);
    m_statusProgress = new QLabel(this);
    m_statusStats    = new QLabel(this);
    m_statusMistakes = new QLabel(this);
    m_statusMistakes->setStyleSheet("color: palette(bright-text);");

    statusBar()->addWidget(m_statusProgress, 1);
    statusBar()->addPermanentWidget(m_statusSpeed);
    statusBar()->addPermanentWidget(m_statusKey);
    statusBar()->addPermanentWidget(m_statusCode);
    statusBar()->addPermanentWidget(m_statusStats);
    statusBar()->addPermanentWidget(m_statusMistakes);

    setupStatusBarMusic();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this] {
        m_statusMistakes->setStyleSheet(QString("color: %1;")
            .arg(ThemeManager::instance().color(ThemeManager::Error).name()));
    });
    
    updateStats();
}

void MainWindow::applyConfigToUi()
{
    auto& cfg = ConfigManager::instance();

    // 主题模式
    QString tm = cfg.themeMode();
    ThemeManager::Mode m = ThemeManager::System;
    if (tm == "light") m = ThemeManager::Light;
    else if (tm == "dark") m = ThemeManager::Dark;
    ThemeManager::instance().setMode(m);

    // 自定义颜色
    QHash<int, QColor> customColors = cfg.customThemeColors();
    ThemeManager::instance().setCustomColors(customColors);

    // 字体
    QFont f = cfg.typingFont();
    if (f.family().isEmpty()) {
        f = font();
        f.setPointSize(18);
    }
    applyTypingFont(f);
}

void MainWindow::applyTypingFont(const QFont& f)
{
    m_currentTypingFont = f;
    if (m_view) m_view->setTypingFont(f);
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(this);

    // 实时预览连接
    connect(&dlg, &SettingsDialog::fontPreview,
            this, &MainWindow::applyTypingFont);
    connect(&dlg, &SettingsDialog::themeColorsPreview,
            this, [this](const QHash<int, QColor>& colors) {
        ThemeManager::instance().setCustomColors(colors);
    });
    // themeModePreview 由 SettingsDialog 内部直接调 ThemeManager，
    // 无需在这里额外处理

    dlg.exec();
    ensureViewFocus();
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
    QString text = TextLoader::loadFile(path, &err);
    if (text.isEmpty() && !err.isEmpty()) {
        QMessageBox::warning(this, tr("打开失败"), err);
        return;
    }
    m_docKey = QFileInfo(path).absoluteFilePath();
    loadTextContent(text, QFileInfo(path).fileName());
}

void MainWindow::loadResourceText(const QString& resPath)
{
    QString err;
    QString text = TextLoader::loadResource(resPath, &err);
    if (text.isEmpty()) {
        QMessageBox::warning(this, tr("加载失败"), err);
        return;
    }
    m_docKey = resPath;
    loadTextContent(text, QFileInfo(resPath).fileName());
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

    // 先设置测速点模式（避免 retry 时启动错误的定时器）
    if (pacman) {
        m_session->setSpeedPointMode(TypingSession::PositionBased);
    } else {
        m_session->setSpeedPointMode(TypingSession::TimeBased);
        m_session->setTimeInterval(20);
    }

    // 创建新视图
    TypingView* newView = pacman
        ? static_cast<TypingView*>(new PacmanView(this))
        : static_cast<TypingView*>(new TwoLineView(this));

    newView->setSession(m_session);
    newView->setDocument(m_doc);
    newView->setCodeTable(m_codeTable);

    // 替换视图
    auto* central = centralWidget();
    auto* layout = qobject_cast<QVBoxLayout*>(central->layout());
    QLayoutItem* oldItem = layout->replaceWidget(m_view, newView);
    delete oldItem;

    TypingView* old = m_view;
    m_view = newView;
    old->deleteLater();

    // 设置字体
    newView->setTypingFont(m_currentTypingFont);
    newView->update();   // 触发一次重绘

    // 连接信号
    connect(m_view, &TypingView::codeHintRequested,
            this, &MainWindow::onCodeHintRequested);

    // 最后 retry（会按已设好的测速点模式启动定时器）
    if (!m_session->target().isEmpty()) {
        m_session->retry();
    }

    updateStats();
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
    ConfigManager::instance().setAutoLoadCodeTablePath(path);
    ConfigManager::instance().save();
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
    // 若源文件不在用户码表目录，复制一份进去，便于设置对话框列表统一管理
    QFileInfo fi(path);
    const QString dest = AppPaths::codeTableDir() + "/" + fi.fileName();
    if (fi.absolutePath() != AppPaths::codeTableDir() && !QFile::exists(dest))
        QFile::copy(path, dest);

    ConfigManager::instance().setAutoLoadCodeTablePath(
        QFile::exists(dest) ? dest : path);
    ConfigManager::instance().save();
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
    // 若会话已开始，重设测速点需要重置会话统计
    if (m_session->currentIndex() > m_session->initialStartIndex()) {
        auto ret = QMessageBox::question(this, tr("测速点"),
            tr("重设测速点会重置当前统计，是否继续？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) { ensureViewFocus(); return; }
        m_session->startFrom(m_doc->text(), m_session->initialStartIndex());
    }
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

    m_statusProgress->setText(tr("进度: %1 / %2")
        .arg(m_session->currentIndex())
        .arg(m_session->totalLength()));

    m_statusStats->setText(
        tr("速度 %1 字/分  ·  击键 %2  ·  码长 %3  ·  错字 %4  ·  回改 %5")
            .arg(m_session->speedCPM(), 0, 'f', 1)
            .arg(m_session->keystrokePerSec(), 0, 'f', 1)
            .arg(m_session->codeLength(), 0, 'f', 2)
            .arg(m_session->errorChars())
            .arg(m_session->backspaceCount()));

    int mistakeCount = m_session->mistakeCount();
    m_statusMistakes->setText(
        mistakeCount > 0 ? tr("错字位置 %1 (Ctrl+M)").arg(mistakeCount)
                         : QString());
}

// ---------------------------------------------------------------
// 段落导航
// ---------------------------------------------------------------
int MainWindow::currentParagraphIndex() const
{
    if (!m_doc || !m_session) return 0;
    const auto& paras = m_doc->paragraphs();
    if (paras.isEmpty()) return 0;

    int idx = m_session->currentIndex();
    for (int i = 0; i < paras.size(); ++i) {
        int start = paras[i].startIndex;
        int end = start + paras[i].length;
        if (idx >= start && idx < end) return i;
    }
    return paras.size() - 1;
}

void MainWindow::jumpToParagraph(int index)
{
    if (!m_doc || !m_session) return;
    const auto& paras = m_doc->paragraphs();
    if (index < 0 || index >= paras.size()) return;

    int start = paras[index].startIndex;
    m_session->startFrom(m_doc->text(), start);

    setWindowTitle(tr("打字练习 - %1 [第 %2/%3 段]")
        .arg(m_doc->name())
        .arg(index + 1)
        .arg(paras.size()));

    m_view->setDocument(m_doc);
    m_view->update();
    updateStats();
    ensureViewFocus();
}

void MainWindow::onRetry()
{
    if (!m_session || m_session->target().isEmpty()) return;
    m_session->retry();
    updateStats();
    ensureViewFocus();
}

void MainWindow::loadTextContent(const QString& raw, const QString& name)
{
    m_originalText = raw;
    m_docName = name;

    // ---- 1. 过滤 ----
    FilterOptions filterOpt = ConfigManager::instance().filterOptions();
    QString filtered = TextFilter::apply(raw, filterOpt);

    // ---- 2. 乱序 ----
    QString content = m_shuffleMode
                          ? TextShuffler::shuffle(filtered)
                          : filtered;

    // ---- 3. 加载到文档 ----
    m_doc->loadFromString(content, name);

    // ---- 4. 设置测速点模式 ----
    const bool isFollowView = qobject_cast<TwoLineView*>(m_view) != nullptr;
    if (isFollowView || m_shuffleMode) {
        m_session->setSpeedPointMode(TypingSession::TimeBased);
        m_session->setTimeInterval(20);
    } else {
        m_session->setSpeedPointMode(TypingSession::PositionBased);
    }

    // ---- 5. 设置倒计时（在 startFrom 之前） ----
    auto& cfg = ConfigManager::instance();
    if (cfg.countdownEnabled()) {
        m_session->setCountdown(cfg.countdownMinutes());
    } else {
        m_session->setCountdown(0);
    }

    // ---- 6. 根据 openMode 决定起始位置 ----
    startSessionByOpenMode(content);
}

void MainWindow::onToggleShuffle(bool on)
{
    m_shuffleMode = on;
    if (m_originalText.isEmpty()) return;
    // 重载当前文本，按新模式重建会话
    loadTextContent(m_originalText, m_docName);
}

// ---------------------------------------------------------------
// 错字列表
// ---------------------------------------------------------------
void MainWindow::onShowMistakes()
{
    if (!m_session) return;

    const auto& mistakes = m_session->mistakes();
    if (mistakes.isEmpty()) {
        QMessageBox::information(this, tr("错字列表"),
            tr("没有错字记录。"));
        ensureViewFocus();
        return;
    }

    MistakeDialog dlg(mistakes, m_session->target(), this);
    connect(&dlg, &MistakeDialog::jumpRequested,
            this, [this](int pos) {
        m_session->skipToPosition(pos);
        m_view->update();
        updateStats();
    });
    dlg.exec();
    ensureViewFocus();
}

void MainWindow::startSessionByOpenMode(const QString& content)
{
    auto& cfg = ConfigManager::instance();
    const int om = cfg.openMode();   // 0从头 1随机 2断点

    int startIndex = 0;

    if (om == 1) {
        // 随机选取位置
        const int total = content.length();
        if (total > 100) {
            // 在 [0, total-100) 之间随机
            startIndex = QRandomGenerator::global()->bounded(total - 100);
        } else {
            startIndex = 0;
        }
    } else if (om == 2) {
        // 断点续打：从 ConfigManager 读上次位置
        // 现在先占位，等 HistoryDb 做完再补
        startIndex = cfg.lastReadPosition(m_docName);
        if (startIndex < 0 || startIndex >= content.length())
            startIndex = 0;
    } else {
        // 从头开始
        startIndex = 0;
    }

    // 从指定位置开始会话
    m_session->startFrom(content, startIndex);

    // 更新视图和标题
    m_view->setDocument(m_doc);
    m_view->update();
    setWindowTitle(tr("打字练习 - %1").arg(m_docName));
    updateStats();
    ensureViewFocus();
}

void MainWindow::setupMusicPlayer()
{
    m_musicPlayer = new MusicPlayer(this);

    connect(m_musicPlayer, &MusicPlayer::stateChanged,
            this, &MainWindow::updateMusicUi);
    connect(m_musicPlayer, &MusicPlayer::trackChanged,
            this, [this](const QString& path, int) {
        if (m_musicTrackLabel) {
            m_musicTrackLabel->setText(QFileInfo(path).fileName());
            m_musicTrackLabel->setToolTip(path);
        }
        updateMusicUi();
    });
    connect(m_musicPlayer, &MusicPlayer::errorOccurred,
            this, [this](const QString& msg) {
        statusBar()->showMessage(tr("音乐播放错误: %1").arg(msg), 5000);
    });
    connect(m_musicPlayer, &MusicPlayer::playlistChanged,
            this, &MainWindow::updateMusicUi);

    auto& cfg = ConfigManager::instance();
    m_musicPlayer->setPlaylist(cfg.musicFiles());
    m_musicPlayer->setVolume(cfg.musicVolume());
    m_musicPlayer->setLoopMode(
        static_cast<MusicPlayer::LoopMode>(cfg.musicLoopMode()));

    // 延迟启动播放（等窗口显示后）
    QTimer::singleShot(300, this, [this]() {
        auto& cfg = ConfigManager::instance();
        if (!cfg.playBgMusicOnStartup()) return;
        if (!m_musicPlayer) return;

        // 列表为空 → 不播
        if (m_musicPlayer->playlist().isEmpty()) return;

        // 从上次播放的索引开始（默认 0）
        const int lastIdx = cfg.musicCurrentIndex();
        if (lastIdx >= 0 && lastIdx < m_musicPlayer->playlist().size()) {
            m_musicPlayer->playFile(m_musicPlayer->playlist().at(lastIdx));
        } else {
            m_musicPlayer->play();   // 从第 1 首开始
        }
    });
}

void MainWindow::setupStatusBarMusic()
{
    // 曲名标签
    m_musicTrackLabel = new QLabel(tr("（无音乐）"), this);
    m_musicTrackLabel->setMinimumWidth(120);
    m_musicTrackLabel->setMaximumWidth(200);
    m_musicTrackLabel->setStyleSheet("color: palette(mid);");

    // 按钮（用文字符号，避免额外图片资源）
    m_musicPrevBtn = new QToolButton(this);
    m_musicPrevBtn->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    m_musicPrevBtn->setToolTip(tr("上一首"));
    m_musicPrevBtn->setAutoRaise(true);

    m_musicPlayBtn = new QToolButton(this);
    m_musicPlayBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_musicPlayBtn->setToolTip(tr("播放/暂停"));
    m_musicPlayBtn->setAutoRaise(true);

    m_musicNextBtn = new QToolButton(this);
    m_musicNextBtn->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    m_musicNextBtn->setToolTip(tr("下一首"));
    m_musicNextBtn->setAutoRaise(true);

    // 加到状态栏右侧（PermanentWidget 是右对齐）
    statusBar()->addPermanentWidget(m_musicTrackLabel);
    statusBar()->addPermanentWidget(m_musicPrevBtn);
    statusBar()->addPermanentWidget(m_musicPlayBtn);
    statusBar()->addPermanentWidget(m_musicNextBtn);

    // 连接
    connect(m_musicPrevBtn, &QToolButton::clicked,
            this, &MainWindow::onPrevMusic);
    connect(m_musicPlayBtn, &QToolButton::clicked,
            this, &MainWindow::onPlayPauseMusic);
    connect(m_musicNextBtn, &QToolButton::clicked,
            this, &MainWindow::onNextMusic);
}

void MainWindow::onPlayPauseMusic()
{
    if (m_musicPlayer->playlist().isEmpty()) {
        // 没有音乐，打开音乐库
        onOpenMusicLibrary();
        return;
    }
    m_musicPlayer->togglePlayPause();
}

void MainWindow::onNextMusic()
{
    m_musicPlayer->next();
}

void MainWindow::onPrevMusic()
{
    m_musicPlayer->previous();
}

void MainWindow::onOpenMusicLibrary()
{
    MusicLibraryDialog dlg(this);
    connect(&dlg, &MusicLibraryDialog::playRequested,
            this, [this](const QString& path) {
        m_musicPlayer->playFile(path);
    });
    if (dlg.exec() == QDialog::Accepted) {
        // 更新播放器列表
        m_musicPlayer->setPlaylist(dlg.playlist());
        ConfigManager::instance().save();
    }
}

void MainWindow::updateMusicUi()
{
    const bool playing = m_musicPlayer->isPlaying();
    m_musicPlayBtn->setText(playing ? "暂停" : "播放");
    m_musicPlayBtn->setToolTip(playing ? tr("暂停") : tr("播放"));

    // 没音乐时禁用按钮
    const bool hasMusic = !m_musicPlayer->playlist().isEmpty();
    m_musicPrevBtn->setEnabled(hasMusic);
    m_musicPlayBtn->setEnabled(hasMusic);
    m_musicNextBtn->setEnabled(hasMusic);

    if (!hasMusic)
        m_musicTrackLabel->setText(tr("（无音乐）"));
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    auto& cfg = ConfigManager::instance();

    // 断点
    if (m_session && !m_docName.isEmpty()) {
        cfg.setLastReadPosition(m_docName, m_session->currentIndex());
    }

    // 音乐
    if (m_musicPlayer) {
        cfg.setMusicVolume(m_musicPlayer->volume());
        cfg.setMusicLoopMode(int(m_musicPlayer->loopMode()));
        cfg.setMusicFiles(m_musicPlayer->playlist());
        cfg.setMusicCurrentIndex(m_musicPlayer->currentIndex());
    }

    cfg.save();
    QMainWindow::closeEvent(e);
}

void MainWindow::saveHistoryEntry()
{
    if (!m_session || m_session->target().isEmpty()) return;
    if (m_docName.isEmpty()) return;

    // 过滤太短的记录（比如只打了两三个字就结束）
    if (m_session->currentIndex() < 10) return;

    HistoryEntry e;
    e.timestamp       = QDateTime::currentDateTime();
    e.docName         = m_docName;
    e.docSource       = m_docKey.isEmpty() ? m_docName : m_docKey;
    e.charCount       = m_session->currentIndex()
                        - m_session->initialStartIndex();
    e.correct         = m_session->correctChars();
    e.errors          = m_session->errorChars();
    e.backspaces      = m_session->backspaceCount();
    e.durationSeconds = m_session->elapsedSeconds();
    e.speedCPM        = m_session->speedCPM();
    e.keystrokes      = m_session->totalKeystrokes();

    // 准确率
    const int total = e.charCount;
    e.accuracy = (total > 0)
                     ? (total - e.errors) * 100.0 / total
                     : 0.0;
    if (e.accuracy < 0) e.accuracy = 0;

    // 码长
    e.codeLength = (e.correct > 0)
                       ? double(e.keystrokes) / e.correct
                       : 0.0;

    if (!HistoryDb::instance().add(e)) {
        qWarning() << "保存历史失败:" << HistoryDb::instance().lastError();
    }
}