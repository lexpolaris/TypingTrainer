// src/ui/SettingsDialog.cpp
#include "SettingsDialog.h"

#include "app/ConfigManager.h"
#include "theme/ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QColorDialog>
#include <QMessageBox>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("设置"));
    resize(640, 540);
    setupUi();
    loadFromConfig();
}

SettingsDialog::~SettingsDialog() = default;

// ---------------------------------------------------------------
// UI
// ---------------------------------------------------------------
void SettingsDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    auto* tabs = new QTabWidget(this);
    tabs->addTab(buildFontPage(), tr("字体"));
    tabs->addTab(buildThemePage(), tr("主题"));
    root->addWidget(tabs, 1);

    // 底部按钮
    auto* btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted,
            this, &SettingsDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected,
            this, &SettingsDialog::onCancel);
    root->addWidget(btnBox);
}

QWidget* SettingsDialog::buildFontPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    // ---- 字体选择 ----
    auto* formBox = new QGroupBox(tr("跟打区字体"), page);
    auto* form = new QFormLayout(formBox);

    m_fontCombo = new QFontComboBox(formBox);
    form->addRow(tr("字体族:"), m_fontCombo);

    m_fontSizeSpin = new QSpinBox(formBox);
    m_fontSizeSpin->setRange(8, 72);
    m_fontSizeSpin->setSuffix(tr(" pt"));
    form->addRow(tr("字号:"), m_fontSizeSpin);

    auto* styleRow = new QHBoxLayout();
    m_fontBoldCheck = new QCheckBox(tr("粗体"), formBox);
    m_fontItalicCheck = new QCheckBox(tr("斜体"), formBox);
    styleRow->addWidget(m_fontBoldCheck);
    styleRow->addWidget(m_fontItalicCheck);
    styleRow->addStretch();
    form->addRow(tr("样式:"), styleRow);

    m_fontResetBtn = new QPushButton(tr("恢复默认"), formBox);
    form->addRow(QString(), m_fontResetBtn);

    layout->addWidget(formBox);

    // ---- 预览 ----
    auto* previewBox = new QGroupBox(tr("预览"), page);
    auto* previewLayout = new QVBoxLayout(previewBox);

    m_fontPreviewLabel = new QLabel(previewBox);
    m_fontPreviewLabel->setText(tr("春眠不觉晓，处处闻啼鸟。"));
    m_fontPreviewLabel->setAlignment(Qt::AlignCenter);
    m_fontPreviewLabel->setWordWrap(true);
    m_fontPreviewLabel->setMinimumHeight(60);

    m_fontPreviewEdit = new QPlainTextEdit(previewBox);
    m_fontPreviewEdit->setPlainText(
        tr("春眠不觉晓，处处闻啼鸟。\n"
           "夜来风雨声，花落知多少。"));
    m_fontPreviewEdit->setReadOnly(true);
    m_fontPreviewEdit->setMaximumHeight(120);

    previewLayout->addWidget(m_fontPreviewLabel);
    previewLayout->addWidget(m_fontPreviewEdit);

    layout->addWidget(previewBox, 1);

    // ---- 连接 ----
    connect(m_fontCombo, &QFontComboBox::currentFontChanged,
            this, &SettingsDialog::onFontFamilyChanged);
    connect(m_fontSizeSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, &SettingsDialog::onFontSizeChanged);
    connect(m_fontBoldCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onFontBoldChanged);
    connect(m_fontItalicCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onFontItalicChanged);
    connect(m_fontResetBtn, &QPushButton::clicked,
            this, &SettingsDialog::onFontReset);

    return page;
}

QWidget* SettingsDialog::buildThemePage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    // ---- 模式 ----
    auto* modeBox = new QGroupBox(tr("主题模式"), page);
    auto* modeLayout = new QFormLayout(modeBox);
    m_themeModeCombo = new QComboBox(modeBox);
    m_themeModeCombo->addItem(tr("跟随系统"), int(ThemeManager::System));
    m_themeModeCombo->addItem(tr("亮色"),     int(ThemeManager::Light));
    m_themeModeCombo->addItem(tr("暗色"),     int(ThemeManager::Dark));
    modeLayout->addRow(tr("模式:"), m_themeModeCombo);
    layout->addWidget(modeBox);

    // ---- 自定义颜色表 ----
    auto* colorBox = new QGroupBox(tr("自定义颜色（留空表示使用默认）"), page);
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

        // 第 0 列：角色名
        auto* nameItem = new QTableWidgetItem(ThemeManager::roleName(role));
        nameItem->setData(Qt::UserRole, int(role));
        m_colorTable->setItem(i, 0, nameItem);

        // 第 1 列：颜色按钮
        auto* colorBtn = new QPushButton(m_colorTable);
        colorBtn->setFixedHeight(24);
        colorBtn->setProperty("role", int(role));
        m_colorTable->setCellWidget(i, 1, colorBtn);
        connect(colorBtn, &QPushButton::clicked, this, [this, i, role]() {
            onCustomColorClicked(i, 0);
        });

        // 第 2 列：恢复默认
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
            this, &SettingsDialog::onThemeModeChanged);
    connect(m_themeResetBtn, &QPushButton::clicked,
            this, &SettingsDialog::onThemeResetAll);

    return page;
}

// ---------------------------------------------------------------
// 加载配置
// ---------------------------------------------------------------
void SettingsDialog::loadFromConfig()
{
    auto& cfg = ConfigManager::instance();

    // ---- 字体 ----
    QFont f = cfg.typingFont();
    if (f.family().isEmpty()) f = font();
    m_fontCombo->setCurrentFont(f);
    m_fontSizeSpin->setValue(f.pointSize());
    m_fontBoldCheck->setChecked(f.bold());
    m_fontItalicCheck->setChecked(f.italic());

    // ---- 主题模式 ----
    QString modeStr = cfg.themeMode();
    int modeIdx = 0;
    if (modeStr == "light") modeIdx = 1;
    else if (modeStr == "dark") modeIdx = 2;
    m_themeModeCombo->setCurrentIndex(modeIdx);

    // ---- 自定义颜色 ----
    QHash<int, QColor> customColors = cfg.customThemeColors();

    // 更新表格颜色按钮
    for (int i = 0; i < m_colorTable->rowCount(); ++i) {
        int role = m_colorTable->item(i, 0)->data(Qt::UserRole).toInt();
        auto* btn = qobject_cast<QPushButton*>(m_colorTable->cellWidget(i, 1));
        if (!btn) continue;
        QColor c = customColors.value(role);
        if (c.isValid()) {
            btn->setText(c.name());
            btn->setStyleSheet(QString("background-color: %1; color: %2;")
                .arg(c.name(), c.lightness() > 128 ? "black" : "white"));
        } else {
            QColor def = ThemeManager::instance().color(
                static_cast<ThemeManager::Role>(role));
            btn->setText(def.name());
            btn->setStyleSheet(QString("background-color: %1; color: %2;")
                .arg(def.name(), def.lightness() > 128 ? "black" : "white"));
        }
    }

    // ---- 保存快照 ----
    m_originalFont = cfg.typingFont();
    m_originalThemeMode = modeIdx;
    m_originalCustomColors = customColors;

    // 触发一次预览
    emitFontPreview();
    emitThemePreview();
}

// ---------------------------------------------------------------
// 保存配置
// ---------------------------------------------------------------
void SettingsDialog::saveToConfig()
{
    auto& cfg = ConfigManager::instance();

    // 字体
    QFont f;
    f.setFamily(m_fontCombo->currentFont().family());
    f.setPointSize(m_fontSizeSpin->value());
    f.setBold(m_fontBoldCheck->isChecked());
    f.setItalic(m_fontItalicCheck->isChecked());
    cfg.setTypingFont(f);

    // 主题模式
    int modeInt = m_themeModeCombo->currentData().toInt();
    QString modeStr = (modeInt == int(ThemeManager::Light)) ? "light" :
                      (modeInt == int(ThemeManager::Dark))  ? "dark"  : "system";
    cfg.set("general.themeMode", modeStr);

    // 自定义颜色
    cfg.setCustomThemeColors(ThemeManager::instance().customColors());

    cfg.save();
}

// ---------------------------------------------------------------
// 字体事件
// ---------------------------------------------------------------
void SettingsDialog::onFontFamilyChanged()
{
    emitFontPreview();
}

void SettingsDialog::onFontSizeChanged(int)
{
    emitFontPreview();
}

void SettingsDialog::onFontBoldChanged(bool)
{
    emitFontPreview();
}

void SettingsDialog::onFontItalicChanged(bool)
{
    emitFontPreview();
}

void SettingsDialog::onFontReset()
{
    QFont def;
    def.setFamily("");  // 系统默认
    def.setPointSize(18);
    m_fontCombo->setCurrentFont(QFont());
    m_fontSizeSpin->setValue(18);
    m_fontBoldCheck->setChecked(false);
    m_fontItalicCheck->setChecked(false);
    emitFontPreview();
}

void SettingsDialog::emitFontPreview()
{
    QFont f;
    f.setFamily(m_fontCombo->currentFont().family());
    f.setPointSize(m_fontSizeSpin->value());
    f.setBold(m_fontBoldCheck->isChecked());
    f.setItalic(m_fontItalicCheck->isChecked());

    // 更新预览
    m_fontPreviewLabel->setFont(f);
    m_fontPreviewEdit->setFont(f);

    emit fontPreview(f);
}

// ---------------------------------------------------------------
// 主题事件
// ---------------------------------------------------------------
void SettingsDialog::onThemeModeChanged(int)
{
    emitThemePreview();
}

void SettingsDialog::onCustomColorClicked(int row, int)
{
    auto* nameItem = m_colorTable->item(row, 0);
    if (!nameItem) return;
    int role = nameItem->data(Qt::UserRole).toInt();

    QColor current = ThemeManager::instance().color(
        static_cast<ThemeManager::Role>(role));

    QColor chosen = QColorDialog::getColor(current, this,
        tr("选择颜色 - %1").arg(ThemeManager::roleName(
            static_cast<ThemeManager::Role>(role))));
    if (!chosen.isValid()) return;

    ThemeManager::instance().setCustomColor(
        static_cast<ThemeManager::Role>(role), chosen);

    // 更新按钮
    auto* btn = qobject_cast<QPushButton*>(m_colorTable->cellWidget(row, 1));
    if (btn) {
        btn->setText(chosen.name());
        btn->setStyleSheet(QString("background-color: %1; color: %2;")
            .arg(chosen.name(), chosen.lightness() > 128 ? "black" : "white"));
    }

    emit themeColorsPreview(ThemeManager::instance().customColors());
}

void SettingsDialog::onCustomColorReset(int row)
{
    auto* nameItem = m_colorTable->item(row, 0);
    if (!nameItem) return;
    int role = nameItem->data(Qt::UserRole).toInt();

    ThemeManager::instance().clearCustomColor(
        static_cast<ThemeManager::Role>(role));

    // 更新按钮为默认色
    QColor def = ThemeManager::instance().color(
        static_cast<ThemeManager::Role>(role));
    auto* btn = qobject_cast<QPushButton*>(m_colorTable->cellWidget(row, 1));
    if (btn) {
        btn->setText(def.name());
        btn->setStyleSheet(QString("background-color: %1; color: %2;")
            .arg(def.name(), def.lightness() > 128 ? "black" : "white"));
    }

    emit themeColorsPreview(ThemeManager::instance().customColors());
}

void SettingsDialog::onThemeResetAll()
{
    ThemeManager::instance().clearAllCustomColors();

    // 刷新所有颜色按钮
    for (int i = 0; i < m_colorTable->rowCount(); ++i) {
        onCustomColorReset(i);
    }

    emit themeColorsPreview({});
}

void SettingsDialog::emitThemePreview()
{
    int modeInt = m_themeModeCombo->currentData().toInt();

    ThemeManager::Mode mode = ThemeManager::System;
    if (modeInt == int(ThemeManager::Light)) mode = ThemeManager::Light;
    else if (modeInt == int(ThemeManager::Dark)) mode = ThemeManager::Dark;

    if (ThemeManager::instance().mode() != mode)
        ThemeManager::instance().setMode(mode);

    emit themeModePreview(modeInt);

    // 同时把当前自定义颜色应用到全局
    emit themeColorsPreview(ThemeManager::instance().customColors());
}

// ---------------------------------------------------------------
// 确定 / 取消
// ---------------------------------------------------------------
void SettingsDialog::onAccept()
{
    saveToConfig();
    accept();
}

void SettingsDialog::onCancel()
{
    // ---- 恢复字体 ----
    QFont f = m_originalFont;
    if (f.family().isEmpty()) {
        // 原始为空，用系统默认
        f.setPointSize(18);
    }
    emit fontPreview(f);

    // ---- 恢复主题模式 ----
    ThemeManager::Mode mode = ThemeManager::System;
    if (m_originalThemeMode == 1) mode = ThemeManager::Light;
    else if (m_originalThemeMode == 2) mode = ThemeManager::Dark;
    ThemeManager::instance().setMode(mode);

    // ---- 恢复自定义颜色 ----
    ThemeManager::instance().setCustomColors(m_originalCustomColors);

    reject();
}