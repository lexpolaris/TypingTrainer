// src/ui/settings/CodeTableSettingsPage.cpp
#include "CodeTableSettingsPage.h"

#include "app/ConfigManager.h"
#include "utils/AppPaths.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>

CodeTableSettingsPage::CodeTableSettingsPage(QWidget* parent)
    : SettingsPage(parent)
{
    setupUi();
    reloadCodeTableList();
}

void CodeTableSettingsPage::setupUi()
{
    auto* layout = new QVBoxLayout(this);

    auto* box = new QGroupBox(tr("启动时自动加载的码表"), this);
    auto* form = new QFormLayout(box);

    auto* row = new QHBoxLayout();
    m_codeTableCombo = new QComboBox(box);
    m_codeTableCombo->setMinimumWidth(280);
    m_codeTableImportBtn = new QPushButton(tr("导入..."), box);
    m_codeTableClearBtn = new QPushButton(tr("不使用"), box);
    row->addWidget(m_codeTableCombo, 1);
    row->addWidget(m_codeTableImportBtn);
    row->addWidget(m_codeTableClearBtn);
    form->addRow(tr("码表:"), row);

    auto* hint = new QLabel(
        tr("内置码表随程序发布；"
           "导入的码表保存在用户数据目录，下次启动自动出现在此列表中。"), box);
    hint->setWordWrap(true);
    hint->setForegroundRole(QPalette::PlaceholderText);
    form->addRow(QString(), hint);

    layout->addWidget(box);
    layout->addStretch();

    connect(m_codeTableImportBtn, &QPushButton::clicked,
            this, &CodeTableSettingsPage::onImportClicked);
    connect(m_codeTableClearBtn, &QPushButton::clicked,
            this, &CodeTableSettingsPage::onClearClicked);
}

// ---------------------------------------------------------------
// 配置
// ---------------------------------------------------------------
void CodeTableSettingsPage::loadFromConfig()
{
    const QString savedPath = ConfigManager::instance().autoLoadCodeTablePath();
    for (int i = 0; i < m_codeTableCombo->count(); ++i) {
        if (m_codeTableCombo->itemData(i).toString() == savedPath) {
            m_codeTableCombo->setCurrentIndex(i);
            return;
        }
    }
    m_codeTableCombo->setCurrentIndex(0);
}

void CodeTableSettingsPage::saveToConfig()
{
    ConfigManager::instance().setAutoLoadCodeTablePath(
        m_codeTableCombo->currentData().toString());
}

void CodeTableSettingsPage::resetToDefault()
{
    m_codeTableCombo->setCurrentIndex(0);  // "不使用"
}

// ---------------------------------------------------------------
// 事件
// ---------------------------------------------------------------
void CodeTableSettingsPage::onImportClicked()
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

void CodeTableSettingsPage::onClearClicked()
{
    m_codeTableCombo->setCurrentIndex(0);
}

// ---------------------------------------------------------------
// 列表
// ---------------------------------------------------------------
void CodeTableSettingsPage::reloadCodeTableList()
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