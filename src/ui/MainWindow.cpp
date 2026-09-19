// src/ui/MainWindow.cpp
#include "MainWindow.h"
#include "PacmanView.h"
#include "TwoLineView.h"
#include "CodeHintPanel.h"
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
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QInputDialog>
#include <QKeyEvent>
#include <QInputMethodEvent>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    m_doc = new TextDocument(this);
    m_session = new TypingSession(this);
    m_codeTable = new CodeTable();

    setupUi();
    setupMenus();
    setupStatusBar();

    // 连接 session 统计更新
    connect(m_session, &TypingSession::positionChanged,
            this, &MainWindow::updateStats);
    connect(m_session, &TypingSession::stateChanged,
            this, [this](TypingSession::State) { updateStats(); });

    applyConfigToUi();

    // 加载内置示例文本
    loadResourceText(":/texts/岳阳楼记.txt");
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

    // 顶部工具条（模式选择 + 码表）
    auto* topBar = new QWidget(central);
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(8, 4, 8, 4);

    topLayout->addWidget(new QLabel(tr("模式:"), topBar));
    m_modeCombo = new QComboBox(topBar);
    m_modeCombo->addItem(PacmanView(nullptr).modeName());
    m_modeCombo->addItem(TwoLineView(nullptr).modeName());
    topLayout->addWidget(m_modeCombo);
    topLayout->addStretch();
    root->addWidget(topBar);

    // 视图占位（默认吃豆人）
    m_view = new PacmanView(central);
    m_view->setSession(m_session);
    m_view->setDocument(m_doc);
    root->addWidget(m_view, 1);

    // 编码提示面板
    m_codeHint = new CodeHintPanel(central);
    root->addWidget(m_codeHint);

    setCentralWidget(central);

    connect(m_modeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSwitchMode);
    connect(m_view, &TypingView::codeHintRequested,
            this, &MainWindow::onCodeHintRequested);
}

void MainWindow::setupMenus()
{
    // 文件
    auto* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("打开文本..."), QKeySequence::Open,
                        this, &MainWindow::onOpenText);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), QKeySequence::Quit,
                        this, &QWidget::close);

    // 模式
    auto* modeMenu = menuBar()->addMenu(tr("模式(&M)"));
    modeMenu->addAction(tr("吃豆人模式"), this, [this] { m_modeCombo->setCurrentIndex(0); });
    modeMenu->addAction(tr("双行对照模式"), this, [this] { m_modeCombo->setCurrentIndex(1); });

    // 码表
    auto* codeMenu = menuBar()->addMenu(tr("码表(&C)"));
    codeMenu->addAction(tr("五笔86"), this, [this] { loadBuiltinCodeTable("wubi86"); });
    codeMenu->addAction(tr("五笔98"), this, [this] { loadBuiltinCodeTable("wubi98"); });
    codeMenu->addAction(tr("郑码"),   this, [this] { loadBuiltinCodeTable("zhengma"); });
    codeMenu->addSeparator();
    codeMenu->addAction(tr("导入码表..."), this, &MainWindow::onImportCodeTable);

    // 主题
    auto* themeMenu = menuBar()->addMenu(tr("主题(&T)"));
    auto* actSys = themeMenu->addAction(tr("跟随系统"));
    auto* actLight = themeMenu->addAction(tr("亮色"));
    auto* actDark = themeMenu->addAction(tr("暗色"));
    actSys->setCheckable(true); actLight->setCheckable(true); actDark->setCheckable(true);
    auto updateChecks = [=](ThemeManager::Mode m) {
        actSys->setChecked(m == ThemeManager::System);
        actLight->setChecked(m == ThemeManager::Light);
        actDark->setChecked(m == ThemeManager::Dark);
    };
    updateChecks(ThemeManager::instance().mode());
    connect(actSys, &QAction::triggered, this, [=] {
        ThemeManager::instance().setMode(ThemeManager::System); updateChecks(ThemeManager::System);
    });
    connect(actLight, &QAction::triggered, this, [=] {
        ThemeManager::instance().setMode(ThemeManager::Light); updateChecks(ThemeManager::Light);
    });
    connect(actDark, &QAction::triggered, this, [=] {
        ThemeManager::instance().setMode(ThemeManager::Dark); updateChecks(ThemeManager::Dark);
    });
    themeMenu->addSeparator();
    themeMenu->addAction(tr("自定义当前字符颜色..."), this, [this] {
        // 演示自定义接口：这里可以弹 QColorDialog
        // ThemeManager::instance().setCustomColor(ThemeManager::Current, chosen);
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

    // 码表（延迟到资源加载后）
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
}

void MainWindow::onSwitchMode(int index)
{
    if (!m_view) return;
    TypingView* newView = (index == 0)
        ? static_cast<TypingView*>(new PacmanView(this))
        : static_cast<TypingView*>(new TwoLineView(this));

    newView->setSession(m_session);
    newView->setDocument(m_doc);
    newView->setCodeTable(m_codeTable);
    newView->setTypingFont(m_view->typingFont());

    // 替换 central 中的 view
    auto* central = centralWidget();
    auto* layout = qobject_cast<QVBoxLayout*>(central->layout());
    layout->replaceWidget(m_view, newView);
    m_view->deleteLater();
    m_view = newView;

    connect(m_view, &TypingView::codeHintRequested,
            this, &MainWindow::onCodeHintRequested);
}

void MainWindow::loadBuiltinCodeTable(const QString& name)
{
    QString path = QString(":/tables/%1.txt").arg(name);
    QString err;
    if (!m_codeTable->loadFromFile(path, &err)) {
        // 资源尚未提供时给出提示
        QMessageBox::information(this, tr("码表"),
            tr("内置码表 %1 尚未提供。\n请通过\"导入码表\"加载。").arg(name));
        return;
    }
    m_view->setCodeTable(m_codeTable);
    m_codeHint->setCodeTable(m_codeTable);
    statusBar()->showMessage(tr("已加载码表: %1").arg(m_codeTable->name()), 3000);
}

void MainWindow::onLoadCodeTable(int) {}
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
}

void MainWindow::onThemeModeChanged(int) {}

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

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Backspace) {
        m_session->backspace();
        e->accept();
        return;
    }
    if (e->key() == Qt::Key_Escape) {
        if (m_session->state() == TypingSession::Running)
            m_session->pause();
        else if (m_session->state() == TypingSession::Paused)
            m_session->resume();
        e->accept();
        return;
    }

    QString text = e->text();
    if (!text.isEmpty() && !e->modifiers().testFlag(Qt::ControlModifier)
        && !e->modifiers().testFlag(Qt::AltModifier)) {
        for (QChar ch : text) {
            if (ch.isPrint()) {
                m_session->inputCharacter(ch);
            }
        }
        e->accept();
        return;
    }
    QMainWindow::keyPressEvent(e);
}

void MainWindow::inputMethodEvent(QInputMethodEvent* e)
{
    // 中文输入法最终提交的字符串
    if (!e->commitString().isEmpty()) {
        for (QChar ch : e->commitString())
            m_session->inputCharacter(ch);
    }
    e->accept();
}
