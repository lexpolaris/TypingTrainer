// src/ui/settings/StartupSettingsPage.cpp
#include "StartupSettingsPage.h"

#include "app/ConfigManager.h"
#include "utils/AppPaths.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <QSettings>
#endif

StartupSettingsPage::StartupSettingsPage(QWidget* parent) : SettingsPage(parent)
{
    setupUi();
    reloadCodeTableList();
    updateControlStates();
}

void StartupSettingsPage::setupUi()
{
    auto* layout = new QVBoxLayout(this);

    // =====================================================
    // 启动时
    // =====================================================
    auto* startupBox = new QGroupBox(tr("启动时"), this);
    auto* startupLayout = new QVBoxLayout(startupBox);

    // ---- 加载上次文章 ----
    m_loadLastText = new QCheckBox(tr("加载上次练习的文章"), startupBox);
    startupLayout->addWidget(m_loadLastText);

    // ---- 背景音乐 ----
    m_playBgMusic = new QCheckBox(tr("播放背景音乐"), startupBox);
    startupLayout->addWidget(m_playBgMusic);

    auto* musicRow = new QHBoxLayout();
    musicRow->addSpacing(24);
    musicRow->addWidget(new QLabel(tr("文件:"), startupBox));
    m_bgMusicPath = new QLineEdit(startupBox);
    m_bgMusicPath->setReadOnly(true);
    m_bgMusicPath->setPlaceholderText(tr("（未选择）"));
    musicRow->addWidget(m_bgMusicPath, 1);
    m_chooseBgMusicBtn = new QPushButton(tr("选择..."), startupBox);
    musicRow->addWidget(m_chooseBgMusicBtn);
    startupLayout->addLayout(musicRow);

    layout->addWidget(startupBox);

    // =====================================================
    // 打开文章时
    // =====================================================
    auto* openBox = new QGroupBox(tr("打开文章时"), this);
    auto* openLayout = new QVBoxLayout(openBox);

    m_openFromStart = new QRadioButton(tr("从头开始"), openBox);
    m_openRandom    = new QRadioButton(tr("随机选取位置"), openBox);
    m_openBreak     = new QRadioButton(tr("断点续打（从上次位置继续）"), openBox);

    openLayout->addWidget(m_openFromStart);
    openLayout->addWidget(m_openRandom);
    openLayout->addWidget(m_openBreak);

    m_openHint = new QLabel(
        tr("注意：断点续打需要程序保存上次阅读位置，"
           "目前仅在主窗口菜单中切换「乱序模式」时自动使用。"), openBox);
    m_openHint->setWordWrap(true);
    m_openHint->setForegroundRole(QPalette::PlaceholderText);
    openLayout->addWidget(m_openHint);

    layout->addWidget(openBox);

    // =====================================================
    // 倒计时测速
    // =====================================================
    auto* countdownBox = new QGroupBox(tr("倒计时测速"), this);
    auto* countdownLayout = new QHBoxLayout(countdownBox);

    m_countdownEnabled = new QCheckBox(tr("启用倒计时"), countdownBox);
    countdownLayout->addWidget(m_countdownEnabled);

    countdownLayout->addWidget(new QLabel(tr("分钟:"), countdownBox));
    m_countdownMinutes = new QSpinBox(countdownBox);
    m_countdownMinutes->setRange(1, 60);
    m_countdownMinutes->setSuffix(tr(" 分钟"));
    countdownLayout->addWidget(m_countdownMinutes);
    countdownLayout->addStretch();

    layout->addWidget(countdownBox);

    // =====================================================
    // 码表
    // =====================================================
    auto* codeBox = new QGroupBox(tr("启动时自动加载的码表"), this);
    auto* codeForm = new QFormLayout(codeBox);

    auto* codeRow = new QHBoxLayout();
    m_codeTableCombo = new QComboBox(codeBox);
    m_codeTableCombo->setMinimumWidth(280);
    m_codeTableImportBtn = new QPushButton(tr("导入..."), codeBox);
    m_codeTableClearBtn = new QPushButton(tr("不使用"), codeBox);
    codeRow->addWidget(m_codeTableCombo, 1);
    codeRow->addWidget(m_codeTableImportBtn);
    codeRow->addWidget(m_codeTableClearBtn);
    codeForm->addRow(tr("码表:"), codeRow);

    auto* codeHint = new QLabel(
        tr("内置码表随程序发布；"
           "导入的码表保存在用户数据目录，下次启动自动出现在此列表中。"),
        codeBox);
    codeHint->setWordWrap(true);
    codeHint->setForegroundRole(QPalette::PlaceholderText);
    codeForm->addRow(QString(), codeHint);

    layout->addWidget(codeBox);
    layout->addStretch();

    // =====================================================
    // 信号连接
    // =====================================================
    connect(m_loadLastText, &QCheckBox::toggled,
            this, [this](bool) { updateControlStates(); });
    connect(m_playBgMusic, &QCheckBox::toggled,
            this, &StartupSettingsPage::onPlayBgMusicToggled);
    connect(m_chooseBgMusicBtn, &QPushButton::clicked,
            this, &StartupSettingsPage::onChooseBgMusic);
    connect(m_countdownEnabled, &QCheckBox::toggled,
            this, &StartupSettingsPage::onCountdownToggled);

    connect(m_codeTableImportBtn, &QPushButton::clicked,
            this, &StartupSettingsPage::onImportCodeTable);
    connect(m_codeTableClearBtn, &QPushButton::clicked,
            this, &StartupSettingsPage::onClearCodeTable);
}

// ---------------------------------------------------------------
// 控件联动
// ---------------------------------------------------------------
void StartupSettingsPage::updateControlStates()
{
    // 背景音乐选择按钮：勾选才能用
    m_bgMusicPath->setEnabled(m_playBgMusic->isChecked());
    m_chooseBgMusicBtn->setEnabled(m_playBgMusic->isChecked());

    // 倒计时分钟：勾选才能用
    m_countdownMinutes->setEnabled(m_countdownEnabled->isChecked());
}

void StartupSettingsPage::onLoadLastTextToggled(bool) { updateControlStates(); }
void StartupSettingsPage::onPlayBgMusicToggled(bool)  { updateControlStates(); }
void StartupSettingsPage::onCountdownToggled(bool)    { updateControlStates(); }

// ---------------------------------------------------------------
// 码表列表
// ---------------------------------------------------------------
void StartupSettingsPage::reloadCodeTableList()
{
    if (!m_codeTableCombo) return;

    const QString prev = m_codeTableCombo->currentData().toString();
    m_codeTableCombo->clear();

    m_codeTableCombo->addItem(tr("（不使用）"), QString());

    // 内置码表
    QDir resDir(":/tables");
    const auto resFiles = resDir.entryInfoList(
        QStringList() << "*.txt" << "*.mb", QDir::Files, QDir::Name);
    for (const QFileInfo& fi : resFiles) {
        m_codeTableCombo->addItem(
            tr("[内置] %1").arg(fi.completeBaseName()),
            ":/tables/" + fi.fileName());
    }

    // 用户码表
    QDir userDir(AppPaths::codeTableDir());
    const auto userFiles = userDir.entryInfoList(
        QStringList() << "*.txt" << "*.mb", QDir::Files, QDir::Name);
    for (const QFileInfo& fi : userFiles) {
        m_codeTableCombo->addItem(
            tr("[用户] %1").arg(fi.completeBaseName()),
            fi.absoluteFilePath());
    }

    if (!prev.isEmpty()) {
        for (int i = 0; i < m_codeTableCombo->count(); ++i) {
            if (m_codeTableCombo->itemData(i).toString() == prev) {
                m_codeTableCombo->setCurrentIndex(i);
                return;
            }
        }
    }
    m_codeTableCombo->setCurrentIndex(0);
}

// ---------------------------------------------------------------
// 配置：加载
// ---------------------------------------------------------------
void StartupSettingsPage::loadFromConfig()
{
    auto& cfg = ConfigManager::instance();

    // ---- 启动时 ----
    m_loadLastText->setChecked(cfg.loadLastTextOnStartup());
    m_playBgMusic->setChecked(cfg.playBgMusicOnStartup());
    m_bgMusicPath->setText(cfg.bgMusicPath());
    m_bgMusicPath->setToolTip(cfg.bgMusicPath());

    // ---- 打开文章时 ----
    int om = cfg.openMode();
    if (om == 1)      m_openRandom->setChecked(true);
    else if (om == 2) m_openBreak->setChecked(true);
    else              m_openFromStart->setChecked(true);

    // ---- 倒计时 ----
    m_countdownEnabled->setChecked(cfg.countdownEnabled());
    m_countdownMinutes->setValue(cfg.countdownMinutes());

    // ---- 码表 ----
    const QString savedCodeTable = cfg.autoLoadCodeTablePath();
    for (int i = 0; i < m_codeTableCombo->count(); ++i) {
        if (m_codeTableCombo->itemData(i).toString() == savedCodeTable) {
            m_codeTableCombo->setCurrentIndex(i);
            break;
        }
    }

    updateControlStates();
}

// ---------------------------------------------------------------
// 配置：保存
// ---------------------------------------------------------------
void StartupSettingsPage::saveToConfig()
{
    auto& cfg = ConfigManager::instance();

    cfg.setLoadLastTextOnStartup(m_loadLastText->isChecked());
    cfg.setPlayBgMusicOnStartup(m_playBgMusic->isChecked());
    cfg.setBgMusicPath(m_bgMusicPath->text());

    int om = 0;
    if (m_openRandom->isChecked()) om = 1;
    else if (m_openBreak->isChecked()) om = 2;
    cfg.setOpenMode(om);

    cfg.setCountdownEnabled(m_countdownEnabled->isChecked());
    cfg.setCountdownMinutes(m_countdownMinutes->value());

    cfg.setAutoLoadCodeTablePath(m_codeTableCombo->currentData().toString());
}

// ---------------------------------------------------------------
// 配置：恢复默认
// ---------------------------------------------------------------
void StartupSettingsPage::resetToDefault()
{
    m_loadLastText->setChecked(true);
    m_playBgMusic->setChecked(false);
    m_bgMusicPath->clear();
    m_countdownEnabled->setChecked(false);
    m_countdownMinutes->setValue(5);

    m_openFromStart->setChecked(true);

    if (m_codeTableCombo->count() > 0)
        m_codeTableCombo->setCurrentIndex(0);

    updateControlStates();
}

// ---------------------------------------------------------------
// 事件
// ---------------------------------------------------------------
void StartupSettingsPage::onChooseBgMusic()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择背景音乐"),
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        tr("音频文件 (*.mp3 *.wav *.ogg *.flac *.m4a);;所有文件 (*)"));
    if (path.isEmpty()) return;

    m_bgMusicPath->setText(path);
    m_bgMusicPath->setToolTip(path);
}

void StartupSettingsPage::onImportCodeTable()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("导入码表"), QDir::homePath(),
        tr("码表 (*.txt *.mb);;所有文件 (*)"));
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    QString dest = AppPaths::codeTableDir() + "/" + fi.fileName();
    if (QFile::exists(dest)) {
        auto ret = QMessageBox::question(this, tr("导入码表"),
            tr("\"%1\" 已存在，是否覆盖？").arg(fi.fileName()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
        QFile::remove(dest);
    }
    if (!QFile::copy(path, dest)) {
        QMessageBox::warning(this, tr("导入失败"),
            tr("无法复制到: %1").arg(dest));
        return;
    }

    reloadCodeTableList();
    for (int i = 0; i < m_codeTableCombo->count(); ++i) {
        if (m_codeTableCombo->itemData(i).toString() == dest) {
            m_codeTableCombo->setCurrentIndex(i);
            break;
        }
    }
}

void StartupSettingsPage::onClearCodeTable()
{
    m_codeTableCombo->setCurrentIndex(0);
}