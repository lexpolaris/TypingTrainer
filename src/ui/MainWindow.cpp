// src/ui/MainWindow.cpp
#include "MainWindow.h"

#include "PacmanView.h"
#include "TwoLineView.h"
#include "CodeHintPanel.h"
#include "TextLibraryDialog.h"
#include "MistakeDialog.h"
#include "MusicLibraryDialog.h"
#include "HistoryDialog.h"

#include "controllers/MenuBuilder.h"
#include "controllers/MusicController.h"
#include "controllers/TextLoaderController.h"
#include "controllers/CodeTableController.h"
#include "controllers/SpeedController.h"
#include "controllers/ThemeController.h"
#include "controllers/HistoryController.h"

#include "core/TextDocument.h"
#include "core/TypingSession.h"
#include "core/CodeTable.h"

#include "app/ConfigManager.h"
#include "theme/ThemeManager.h"
#include "utils/AppPaths.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QFile>
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
    m_session = new TypingSession(this);

    setupUi();
    setupControllers();   // 创建控制器并构建菜单
    setupStatusBar();

    connect(m_session, &TypingSession::positionChanged,
            this, &MainWindow::updateStats);
    connect(m_session, &TypingSession::stateChanged,
            this, [this](TypingSession::State) { updateStats(); });
    connect(m_session, &TypingSession::mistakeAdded,
            this, [this](int) { updateStats(); });
    connect(m_session, &TypingSession::finished, this, [this]() {
        if (docName().isEmpty()) return;
        auto& cfg = ConfigManager::instance();
        cfg.setLastReadPosition(docName(), m_session->currentIndex());
        cfg.save();
    });
    connect(m_session, &TypingSession::finished, this, [this]() {
        m_history->save(m_session, docName(), m_textLoader->docKey());
    });

    auto& cfg = ConfigManager::instance();

    // 把配置应用到 UI（主题/字体）
    m_theme->applyConfigToUi(font());

    // 启动时按配置自动加载码表
    m_codeTables->loadAutoTable();

    // 启动时加载文本：记住上次且有效 → 上次文章；否则 → 欢迎内容
    if (cfg.loadLastTextOnStartup()) {
        const QString lastPath = cfg.lastTextPath();
        if (!lastPath.isEmpty() && QFile::exists(lastPath)) {
            if (lastPath.startsWith(":/"))
                loadResourceText(lastPath);
            else
                loadText(lastPath);
        } else {
            loadWelcomeText();
        }
    } else {
        loadWelcomeText();
    }

    QTimer::singleShot(0, this, &MainWindow::ensureViewFocus);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_view = new PacmanView(central);
    m_view->setSession(m_session);
    root->addWidget(m_view, 1);

    setCentralWidget(central);

    m_codeHint = new CodeHintPanel(this);
    m_codeHint->hide();

    connect(m_view, &TypingView::codeHintRequested,
            this, &MainWindow::onCodeHintRequested);
    connect(m_view, &TypingView::codeHintCleared,
            m_codeHint, &CodeHintPanel::clear);
    connect(m_view, &TypingView::requestRetry,
            this, &MainWindow::onRetry);
}

void MainWindow::ensureViewFocus()
{
    if (m_view) m_view->setFocus(Qt::OtherFocusReason);
}

// ---------------------------------------------------------------
// 控制器
// ---------------------------------------------------------------
void MainWindow::setupControllers()
{
    // ---- 文本加载控制器 ----
    m_textLoader = new TextLoaderController(this);
    m_doc = m_textLoader->document();
    m_view->setDocument(m_doc);

    // ---- 码表控制器 ----
    m_codeTables = new CodeTableController(this);
    m_codeTable = m_codeTables->codeTable();

    // ---- 测速 / 主题 / 历史 ----
    m_speed   = new SpeedController(this);
    m_theme   = new ThemeController(this);
    m_history = new HistoryController(this);

    // ---- 菜单构建器 ----
    m_menuBuilder = new MenuBuilder(this, this);
    m_menuBuilder->build();

    // ---- 音乐控制器 ----
    m_music = new MusicController(this);
    m_music->restoreFromConfig(true);

    connectControllers();
}

void MainWindow::connectControllers()
{
    // ===== 文本加载 =====
    connect(m_textLoader, &TextLoaderController::textReady,
            this, &MainWindow::onTextReady);
    connect(m_textLoader, &TextLoaderController::loadFailed,
            this, [this](const QString& msg) {
        QMessageBox::warning(this, tr("打开失败"), msg);
    });
    connect(m_textLoader, &TextLoaderController::textPathChanged,
            this, [](const QString& path) {
        auto& cfg = ConfigManager::instance();
        cfg.setLastTextPath(path);
        cfg.save();
    });

    // ===== 码表 =====
    connect(m_codeTables, &CodeTableController::codeTableChanged,
            this, [this](CodeTable*) {
        m_view->setCodeTable(m_codeTable);
        m_codeHint->setCodeTable(m_codeTable);
    });
    connect(m_codeTables, &CodeTableController::statusMessage,
            this, [this](const QString& msg, int t) {
        statusBar()->showMessage(msg, t);
    });
    connect(m_codeTables, &CodeTableController::infoMessage,
            this, [this](const QString& title, const QString& msg) {
        QMessageBox::information(this, title, msg);
    });

    // ===== 测速 =====
    connect(m_speed, &SpeedController::retrySegmentRequested,
            this, [this](const QString& segText) {
        startSessionForText(segText, m_doc->name() + " (重打段)");
    });
    connect(m_speed, &SpeedController::statusMessage,
            this, [this](const QString& msg, int t) {
        statusBar()->showMessage(msg, t);
    });
    connect(m_speed, &SpeedController::infoMessage,
            this, [this](const QString& title, const QString& msg) {
        QMessageBox::information(this, title, msg);
    });

    // ===== 主题 =====
    connect(m_theme, &ThemeController::fontChanged,
            this, [this](const QFont& f) {
        if (m_view) m_view->setTypingFont(f);
    });
    connect(m_theme, &ThemeController::themeModeChanged,
            this, [this](int mode) {
        m_menuBuilder->setThemeMode(
            static_cast<MenuBuilder::ThemeMode>(mode));
    });

    // ===== 菜单 =====
    connect(m_menuBuilder, &MenuBuilder::openTextRequested,
            this, &MainWindow::onOpenText);
    connect(m_menuBuilder, &MenuBuilder::openTextLibraryRequested,
            this, &MainWindow::onOpenTextLibrary);
    connect(m_menuBuilder, &MenuBuilder::openSettingsRequested,
            this, [this]() { m_theme->openSettings(this); ensureViewFocus(); });
    connect(m_menuBuilder, &MenuBuilder::modePacmanRequested,
            this, &MainWindow::onSwitchModePacman);
    connect(m_menuBuilder, &MenuBuilder::modeTwoLineRequested,
            this, &MainWindow::onSwitchModeTwoLine);
    connect(m_menuBuilder, &MenuBuilder::retryRequested,
            this, &MainWindow::onRetry);
    connect(m_menuBuilder, &MenuBuilder::shuffleToggled,
            this, &MainWindow::onToggleShuffle);
    connect(m_menuBuilder, &MenuBuilder::showMistakesRequested,
            this, &MainWindow::onShowMistakes);
    connect(m_menuBuilder, &MenuBuilder::showHistoryRequested,
            this, [this]() {
        HistoryDialog dlg(this);
        dlg.exec();
        ensureViewFocus();
    });
    connect(m_menuBuilder, &MenuBuilder::builtinCodeTableRequested,
            this, &MainWindow::loadBuiltinCodeTable);
    connect(m_menuBuilder, &MenuBuilder::importCodeTableRequested,
            this, [this]() { m_codeTables->importFromFile(this); });
    connect(m_menuBuilder, &MenuBuilder::speedPointSettingsRequested,
            this, [this]() {
        m_speed->openSpeedPointSettings(this, m_doc, m_session);
        ensureViewFocus();
    });
    connect(m_menuBuilder, &MenuBuilder::showSpeedChartRequested,
            this, [this]() {
        m_speed->openSpeedChart(this, m_doc, m_session);
        ensureViewFocus();
    });
    connect(m_menuBuilder, &MenuBuilder::themeModeRequested,
            this, [this](MenuBuilder::ThemeMode mode) {
        m_theme->setThemeMode(static_cast<int>(mode));
    });
    connect(m_menuBuilder, &MenuBuilder::customCurrentColorRequested,
            this, [this] {
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
    connect(m_menuBuilder, &MenuBuilder::clearCustomColorsRequested,
            this, [] {
        ThemeManager::instance().clearAllCustomColors();
        ConfigManager::instance().setCustomThemeColors({});
        ConfigManager::instance().save();
        ThemeManager::instance().applyToApplication();
    });
    connect(m_menuBuilder, &MenuBuilder::openMusicLibraryRequested,
            this, &MainWindow::onOpenMusicLibrary);
    connect(m_menuBuilder, &MenuBuilder::playPauseMusicRequested,
            m_music, &MusicController::togglePlayPause);
    connect(m_menuBuilder, &MenuBuilder::nextMusicRequested,
            m_music, &MusicController::next);
    connect(m_menuBuilder, &MenuBuilder::prevMusicRequested,
            m_music, &MusicController::previous);

    // ===== 音乐 =====
    connect(m_music, &MusicController::openLibraryRequested,
            this, &MainWindow::onOpenMusicLibrary);
    connect(m_music, &MusicController::errorMessage,
            this, [this](const QString& msg) {
        statusBar()->showMessage(msg, 5000);
    });
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

    for (QWidget* w : m_music->statusWidgets())
        statusBar()->addPermanentWidget(w);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this] {
        m_statusMistakes->setStyleSheet(QString("color: %1;")
            .arg(ThemeManager::instance().color(ThemeManager::Error).name()));
    });

    updateStats();
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
    m_textLoader->loadFile(path);
}

void MainWindow::loadResourceText(const QString& resPath)
{
    m_textLoader->loadResource(resPath);
}

void MainWindow::loadWelcomeText()
{
    m_textLoader->loadWelcome();
}

// ---------------------------------------------------------------
// 文本就绪回调
// ---------------------------------------------------------------
void MainWindow::onTextReady(const QString& content, int startIndex)
{
    const bool isFollowView = qobject_cast<TwoLineView*>(m_view) != nullptr;
    if (isFollowView || m_textLoader->shuffleMode()) {
        m_session->setSpeedPointMode(TypingSession::TimeBased);
        m_session->setTimeInterval(20);
    } else {
        m_session->setSpeedPointMode(TypingSession::PositionBased);
    }

    auto& cfg = ConfigManager::instance();
    if (cfg.countdownEnabled())
        m_session->setCountdown(cfg.countdownMinutes());
    else
        m_session->setCountdown(0);

    startSessionForText(content, m_textLoader->docName());
    Q_UNUSED(startIndex);
}

// ---------------------------------------------------------------
// 模式切换
// ---------------------------------------------------------------
void MainWindow::switchMode(bool pacman)
{
    if (!m_view) return;

    bool currentlyPacman = qobject_cast<PacmanView*>(m_view) != nullptr;
    if (pacman == currentlyPacman) {
        ensureViewFocus();
        return;
    }

    if (pacman) {
        m_session->setSpeedPointMode(TypingSession::PositionBased);
    } else {
        m_session->setSpeedPointMode(TypingSession::TimeBased);
        m_session->setTimeInterval(20);
    }

    TypingView* newView = pacman
        ? static_cast<TypingView*>(new PacmanView(this))
        : static_cast<TypingView*>(new TwoLineView(this));

    newView->setSession(m_session);
    newView->setDocument(m_doc);
    newView->setCodeTable(m_codeTable);

    auto* central = centralWidget();
    auto* layout = qobject_cast<QVBoxLayout*>(central->layout());
    QLayoutItem* oldItem = layout->replaceWidget(m_view, newView);
    delete oldItem;

    TypingView* old = m_view;
    m_view = newView;
    old->deleteLater();

    newView->setTypingFont(m_theme->typingFont());
    newView->update();

    connect(m_view, &TypingView::codeHintRequested,
            this, &MainWindow::onCodeHintRequested);

    if (!m_session->target().isEmpty())
        m_session->retry();

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
    m_codeTables->loadBuiltin(name);
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

void MainWindow::onToggleShuffle(bool on)
{
    m_textLoader->setShuffleMode(on);
    m_textLoader->reload();
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

// ---------------------------------------------------------------
// 会话启动 / 重打段
// ---------------------------------------------------------------
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
// 音乐
// ---------------------------------------------------------------
void MainWindow::onOpenMusicLibrary()
{
    MusicLibraryDialog dlg(this);
    connect(&dlg, &MusicLibraryDialog::playRequested,
            m_music, &MusicController::playFile);
    if (dlg.exec() == QDialog::Accepted) {
        m_music->setPlaylist(dlg.playlist());
        ConfigManager::instance().save();
    }
}

QString MainWindow::docName() const
{
    return m_textLoader ? m_textLoader->docName() : QString();
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    auto& cfg = ConfigManager::instance();

    if (m_session && !docName().isEmpty()) {
        cfg.setLastReadPosition(docName(), m_session->currentIndex());
    }

    if (m_music)
        m_music->persistToConfig();

    cfg.save();
    QMainWindow::closeEvent(e);
}