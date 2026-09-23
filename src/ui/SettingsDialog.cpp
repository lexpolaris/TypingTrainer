// src/ui/SettingsDialog.cpp
#include "SettingsDialog.h"

#include "settings/FontSettingsPage.h"
#include "settings/ThemeSettingsPage.h"
#include "settings/StartupSettingsPage.h"
#include "settings/FilterSettingsPage.h"

#include "app/ConfigManager.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("设置"));
    resize(640, 540);
    setupUi();
    connectPreviewSignals();

    // 通知各页从配置加载
    for (auto* page : m_pages)
        page->loadFromConfig();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);
    root->addWidget(m_tabs, 1);

    // 创建并添加各页
    m_fontPage      = new FontSettingsPage(this);
    m_themePage     = new ThemeSettingsPage(this);
    m_filterPage    = new FilterSettingsPage(this);
    m_startupPage = new StartupSettingsPage(this);

    addPage(m_fontPage);
    addPage(m_themePage);
    addPage(m_filterPage);
    addPage(m_startupPage);

    // 底部按钮
    auto* btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted,
            this, &SettingsDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected,
            this, &SettingsDialog::onCancel);
    root->addWidget(btnBox);
}

void SettingsDialog::addPage(SettingsPage* page)
{
    m_pages.append(page);
    m_tabs->addTab(page, page->title());
}

void SettingsDialog::connectPreviewSignals()
{
    // 字体页：实时预览
    connect(m_fontPage, &FontSettingsPage::fontPreview,
            this, &SettingsDialog::fontPreview);

    // 主题页：实时预览模式与颜色
    connect(m_themePage, &ThemeSettingsPage::modePreview,
            this, &SettingsDialog::themeModePreview);
    connect(m_themePage, &ThemeSettingsPage::colorsPreview,
            this, &SettingsDialog::themeColorsPreview);
}

// ---------------------------------------------------------------
// 确定：所有页写回配置，统一保存
// ---------------------------------------------------------------
void SettingsDialog::onAccept()
{
    for (auto* page : m_pages)
        page->saveToConfig();

    ConfigManager::instance().save();
    accept();
}

// ---------------------------------------------------------------
// 取消：恢复实时预览过的项（字体、主题）
// ---------------------------------------------------------------
void SettingsDialog::onCancel()
{
    // 字体页：恢复原始字体并广播
    m_fontPage->restoreSnapshot();

    // 主题页：恢复原始模式与颜色
    m_themePage->restoreSnapshot();

    reject();
}