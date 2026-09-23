// src/ui/settings/FilterSettingsPage.cpp
#include "FilterSettingsPage.h"

#include "app/ConfigManager.h"
#include "core/TextFilter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>

FilterSettingsPage::FilterSettingsPage(QWidget* parent) : SettingsPage(parent)
{
    setupUi();
    connectMutualExclusion();
}

void FilterSettingsPage::setupUi()
{
    auto* layout = new QVBoxLayout(this);

    // ---- 过滤 ----
    auto* filterBox = new QGroupBox(tr("过滤"), this);
    auto* filterLayout = new QVBoxLayout(filterBox);

    m_filterHan    = new QCheckBox(tr("过滤汉字及全角标点"), filterBox);
    m_filterNonHan = new QCheckBox(tr("过滤非汉字字符（保留汉字）"), filterBox);
    m_filterUpper  = new QCheckBox(tr("过滤大写字母"), filterBox);
    m_filterLower  = new QCheckBox(tr("过滤小写字母"), filterBox);
    m_filterDigit  = new QCheckBox(tr("过滤数字"), filterBox);
    m_filterSpace  = new QCheckBox(tr("过滤空格（半角 + 全角）"), filterBox);

    filterLayout->addWidget(m_filterHan);
    filterLayout->addWidget(m_filterNonHan);
    filterLayout->addWidget(m_filterUpper);
    filterLayout->addWidget(m_filterLower);
    filterLayout->addWidget(m_filterDigit);
    filterLayout->addWidget(m_filterSpace);

    layout->addWidget(filterBox);

    // ---- 转换 ----
    auto* convertBox = new QGroupBox(tr("转换"), this);
    auto* convertLayout = new QVBoxLayout(convertBox);

    m_upperToLower = new QCheckBox(tr("大写字母转换为小写字母"), convertBox);
    m_lowerToUpper = new QCheckBox(tr("小写字母转换为大写字母"), convertBox);

    convertLayout->addWidget(m_upperToLower);
    convertLayout->addWidget(m_lowerToUpper);

    layout->addWidget(convertBox);

    // ---- 提示 ----
    m_hintLabel = new QLabel(this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setForegroundRole(QPalette::PlaceholderText);
    layout->addWidget(m_hintLabel);

    layout->addStretch();

    updateHintLabel();
}

// ---------------------------------------------------------------
// 互斥
// ---------------------------------------------------------------
void FilterSettingsPage::connectMutualExclusion()
{
    // 过滤汉字 ↔ 过滤非汉字：互斥
    connect(m_filterHan, &QCheckBox::toggled,
            this, &FilterSettingsPage::onFilterHanToggled);
    connect(m_filterNonHan, &QCheckBox::toggled,
            this, &FilterSettingsPage::onFilterNonHanToggled);

    // 转换大小写：互斥（同一字母不能既转大写又转小写）
    connect(m_upperToLower, &QCheckBox::toggled, this, [this](bool on) {
        if (on && m_lowerToUpper->isChecked()) {
            QSignalBlocker b(m_lowerToUpper);
            m_lowerToUpper->setChecked(false);
        }
    });
    connect(m_lowerToUpper, &QCheckBox::toggled, this, [this](bool on) {
        if (on && m_upperToLower->isChecked()) {
            QSignalBlocker b(m_upperToLower);
            m_upperToLower->setChecked(false);
        }
    });
}

void FilterSettingsPage::onFilterHanToggled(bool on)
{
    if (on && m_filterNonHan->isChecked()) {
        QSignalBlocker b(m_filterNonHan);
        m_filterNonHan->setChecked(false);
    }
    updateHintLabel();
}

void FilterSettingsPage::onFilterNonHanToggled(bool on)
{
    if (on && m_filterHan->isChecked()) {
        QSignalBlocker b(m_filterHan);
        m_filterHan->setChecked(false);
    }
    updateHintLabel();
}

void FilterSettingsPage::updateHintLabel()
{
    QString hint;

    if (m_filterHan->isChecked() && m_filterNonHan->isChecked()) {
        // 理论上互斥不会同时勾选，这里只是防御
        hint = tr("「过滤汉字」与「过滤非汉字」互斥，同时勾选会导致空文本。");
    } else if (m_filterNonHan->isChecked()) {
        hint = tr("仅保留汉字，删除标点、字母、数字、空格及所有非汉字字符。");
    } else if (m_filterHan->isChecked()) {
        hint = tr("删除所有汉字及全角标点，适合英文打字练习。");
    } else if (m_filterUpper->isChecked() && m_upperToLower->isChecked()) {
        hint = tr("「过滤大写」会先删除大写字母，转换步骤对大写无效。");
    } else if (m_filterLower->isChecked() && m_lowerToUpper->isChecked()) {
        hint = tr("「过滤小写」会先删除小写字母，转换步骤对小写无效。");
    }

    m_hintLabel->setText(hint);
}

// ---------------------------------------------------------------
// 配置
// ---------------------------------------------------------------
void FilterSettingsPage::loadFromConfig()
{
    FilterOptions opt = ConfigManager::instance().filterOptions();

    // 阻断信号，避免加载时触发互斥逻辑
    QSignalBlocker b1(m_filterHan);
    QSignalBlocker b2(m_filterNonHan);
    QSignalBlocker b3(m_filterUpper);
    QSignalBlocker b4(m_filterLower);
    QSignalBlocker b5(m_filterDigit);
    QSignalBlocker b6(m_filterSpace);
    QSignalBlocker b7(m_upperToLower);
    QSignalBlocker b8(m_lowerToUpper);

    m_filterHan->setChecked(opt.filterHan);
    m_filterNonHan->setChecked(opt.filterNonHan);
    m_filterUpper->setChecked(opt.filterUpper);
    m_filterLower->setChecked(opt.filterLower);
    m_filterDigit->setChecked(opt.filterDigit);
    m_filterSpace->setChecked(opt.filterSpace);
    m_upperToLower->setChecked(opt.upperToLower);
    m_lowerToUpper->setChecked(opt.lowerToUpper);

    updateHintLabel();
}

void FilterSettingsPage::saveToConfig()
{
    FilterOptions opt;
    opt.filterHan    = m_filterHan->isChecked();
    opt.filterNonHan = m_filterNonHan->isChecked();
    opt.filterUpper  = m_filterUpper->isChecked();
    opt.filterLower  = m_filterLower->isChecked();
    opt.filterDigit  = m_filterDigit->isChecked();
    opt.filterSpace  = m_filterSpace->isChecked();
    opt.upperToLower = m_upperToLower->isChecked();
    opt.lowerToUpper = m_lowerToUpper->isChecked();

    ConfigManager::instance().setFilterOptions(opt);
}

void FilterSettingsPage::resetToDefault()
{
    {
        QSignalBlocker b1(m_filterHan);
        QSignalBlocker b2(m_filterNonHan);
        QSignalBlocker b3(m_filterUpper);
        QSignalBlocker b4(m_filterLower);
        QSignalBlocker b5(m_filterDigit);
        QSignalBlocker b6(m_filterSpace);
        QSignalBlocker b7(m_upperToLower);
        QSignalBlocker b8(m_lowerToUpper);

        m_filterHan->setChecked(false);
        m_filterNonHan->setChecked(false);
        m_filterUpper->setChecked(false);
        m_filterLower->setChecked(false);
        m_filterDigit->setChecked(false);
        m_filterSpace->setChecked(false);
        m_upperToLower->setChecked(false);
        m_lowerToUpper->setChecked(false);
    }
    updateHintLabel();
}