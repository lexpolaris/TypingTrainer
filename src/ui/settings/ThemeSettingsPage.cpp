// src/ui/settings/ThemeSettingsPage.cpp
#include "ThemeSettingsPage.h"

#include "app/ConfigManager.h"
#include "theme/ThemeManager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QColorDialog>

ThemeSettingsPage::ThemeSettingsPage(QWidget* parent) : SettingsPage(parent)
{
    setupUi();
}

void ThemeSettingsPage::setupUi()
{
    auto* layout = new QVBoxLayout(this);

    // ---- 模式 ----
    auto* modeBox = new QGroupBox(tr("主题模式"), this);
    auto* modeLayout = new QFormLayout(modeBox);
    m_themeModeCombo = new QComboBox(modeBox);
    m_themeModeCombo->addItem(tr("跟随系统"), int(ThemeManager::System));
    m_themeModeCombo->addItem(tr("亮色"),     int(ThemeManager::Light));
    m_themeModeCombo->addItem(tr("暗色"),     int(ThemeManager::Dark));
    modeLayout->addRow(tr("模式:"), m_themeModeCombo);
    layout->addWidget(modeBox);

    // ---- 自定义颜色表 ----
    auto* colorBox = new QGroupBox(tr("自定义颜色（留空表示使用默认）"), this);
    auto* colorLayout = new QVBoxLayout(colorBox);

    m_colorTable = new QTableWidget(colorBox);
    m_colorTable->setColumnCount(3);
    m_colorTable->setHorizontalHeaderLabels(
        {tr("角色"), tr("颜色"), tr("操作")});
    m_colorTable->verticalHeader()->setVisible(false);
    m_colorTable->horizontalHeader()->setStretchLastSection(false);
    m_colorTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_colorTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_colorTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_colorTable->setColumnWidth(1, 120);
    m_colorTable->setColumnWidth(2, 100);
    m_colorTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_colorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    const auto roles = ThemeManager::allRoles();
    m_colorTable->setRowCount(roles.size());
    for (int i = 0; i < roles.size(); ++i) {
        auto role = roles[i];

        auto* nameItem = new QTableWidgetItem(ThemeManager::roleName(role));
        nameItem->setData(Qt::UserRole, int(role));
        m_colorTable->setItem(i, 0, nameItem);

        auto* colorBtn = new QPushButton(m_colorTable);
        colorBtn->setFixedHeight(24);
        m_colorTable->setCellWidget(i, 1, colorBtn);
        connect(colorBtn, &QPushButton::clicked, this, [this, i]() {
            onCustomColorClicked(i);
        });

        auto* resetBtn = new QPushButton(tr("默认"), m_colorTable);
        resetBtn->setFixedHeight(24);
        m_colorTable->setCellWidget(i, 2, resetBtn);
        connect(resetBtn, &QPushButton::clicked, this, [this, i]() {
            onCustomColorReset(i);
        });
    }

    colorLayout->addWidget(m_colorTable, 1);

    m_themeResetBtn = new QPushButton(tr("恢复全部默认"), colorBox);
    colorLayout->addWidget(m_themeResetBtn);

    layout->addWidget(colorBox, 1);

    // ---- 连接 ----
    connect(m_themeModeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ThemeSettingsPage::onThemeModeChanged);
    connect(m_themeResetBtn, &QPushButton::clicked,
            this, &ThemeSettingsPage::onThemeResetAll);
}

// ---------------------------------------------------------------
// 配置 ↔ 控件
// ---------------------------------------------------------------
void ThemeSettingsPage::loadFromConfig()
{
    auto& cfg = ConfigManager::instance();

    // ---- 模式 ----
    QString modeStr = cfg.themeMode();
    int modeIdx = 0;
    if (modeStr == "light") modeIdx = 1;
    else if (modeStr == "dark") modeIdx = 2;

    {
        QSignalBlocker b(m_themeModeCombo);
        m_themeModeCombo->setCurrentIndex(modeIdx);
    }

    // ---- 自定义颜色 ----
    QHash<int, QColor> custom = cfg.customThemeColors();
    refreshColorButtons(custom);

    // ---- 快照 ----
    m_originalMode = modeIdx;
    m_originalColors = custom;

    // 刷新一次预览
    emitModePreview();
    emitColorsPreview();
}

void ThemeSettingsPage::saveToConfig()
{
    auto& cfg = ConfigManager::instance();

    int modeInt = m_themeModeCombo->currentData().toInt();
    QString modeStr = (modeInt == int(ThemeManager::Light)) ? "light" :
                      (modeInt == int(ThemeManager::Dark))  ? "dark"  : "system";
    cfg.set("general.themeMode", modeStr);
    cfg.setCustomThemeColors(ThemeManager::instance().customColors());
}

void ThemeSettingsPage::resetToDefault()
{
    ThemeManager::instance().clearAllCustomColors();
    for (int i = 0; i < m_colorTable->rowCount(); ++i)
        onCustomColorReset(i);

    {
        QSignalBlocker b(m_themeModeCombo);
        m_themeModeCombo->setCurrentIndex(0);  // 跟随系统
    }

    emitModePreview();
    emitColorsPreview();
}

// ---------------------------------------------------------------
// 取消时恢复
// ---------------------------------------------------------------
void ThemeSettingsPage::restoreSnapshot()
{
    // 1. 恢复模式控件 + ThemeManager
    {
        QSignalBlocker b(m_themeModeCombo);
        m_themeModeCombo->setCurrentIndex(m_originalMode);
    }

    // 2. 恢复自定义颜色
    ThemeManager::instance().setCustomColors(m_originalColors);
    refreshColorButtons(m_originalColors);

    // 3. 通过统一入口广播（读的是已恢复的状态）
    emitModePreview();
    emitColorsPreview();
}

// ---------------------------------------------------------------
// 事件
// ---------------------------------------------------------------
void ThemeSettingsPage::onThemeModeChanged(int)
{
    emitModePreview();
}

void ThemeSettingsPage::onCustomColorClicked(int row)
{
    auto* nameItem = m_colorTable->item(row, 0);
    if (!nameItem) return;
    int roleInt = nameItem->data(Qt::UserRole).toInt();
    auto role = static_cast<ThemeManager::Role>(roleInt);

    QColor current = ThemeManager::instance().color(role);
    QColor chosen = QColorDialog::getColor(
        current, this,
        tr("选择颜色 - %1").arg(ThemeManager::roleName(role)));
    if (!chosen.isValid()) return;

    ThemeManager::instance().setCustomColor(role, chosen);
    updateColorButton(row, chosen);
    emitColorsPreview();
}

void ThemeSettingsPage::onCustomColorReset(int row)
{
    auto* nameItem = m_colorTable->item(row, 0);
    if (!nameItem) return;
    int roleInt = nameItem->data(Qt::UserRole).toInt();
    auto role = static_cast<ThemeManager::Role>(roleInt);

    ThemeManager::instance().clearCustomColor(role);
    QColor def = ThemeManager::instance().color(role);
    updateColorButton(row, def);
    emitColorsPreview();
}

void ThemeSettingsPage::onThemeResetAll()
{
    ThemeManager::instance().clearAllCustomColors();
    for (int i = 0; i < m_colorTable->rowCount(); ++i)
        onCustomColorReset(i);
    emitColorsPreview();
}

// ---------------------------------------------------------------
// 工具
// ---------------------------------------------------------------
void ThemeSettingsPage::refreshColorButtons(const QHash<int, QColor>& custom)
{
    for (int i = 0; i < m_colorTable->rowCount(); ++i) {
        int roleInt = m_colorTable->item(i, 0)->data(Qt::UserRole).toInt();
        QColor c = custom.value(roleInt);
        if (!c.isValid()) {
            c = ThemeManager::instance().color(
                static_cast<ThemeManager::Role>(roleInt));
        }
        updateColorButton(i, c);
    }
}

void ThemeSettingsPage::updateColorButton(int row, const QColor& c)
{
    auto* btn = qobject_cast<QPushButton*>(m_colorTable->cellWidget(row, 1));
    if (!btn) return;
    btn->setText(c.name());
    btn->setStyleSheet(QString("background-color: %1; color: %2;")
        .arg(c.name(), c.lightness() > 128 ? "black" : "white"));
}

void ThemeSettingsPage::emitModePreview()
{
    int modeInt = m_themeModeCombo->currentData().toInt();

    ThemeManager::Mode mode = ThemeManager::System;
    if (modeInt == int(ThemeManager::Light)) mode = ThemeManager::Light;
    else if (modeInt == int(ThemeManager::Dark)) mode = ThemeManager::Dark;

    if (ThemeManager::instance().mode() != mode)
        ThemeManager::instance().setMode(mode);

    emit modePreview(modeInt);
}

void ThemeSettingsPage::emitColorsPreview()
{
    emit colorsPreview(ThemeManager::instance().customColors());
}