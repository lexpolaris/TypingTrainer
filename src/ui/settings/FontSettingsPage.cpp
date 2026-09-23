// src/ui/settings/FontSettingsPage.cpp
#include "FontSettingsPage.h"

#include "app/ConfigManager.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

FontSettingsPage::FontSettingsPage(QWidget* parent) : SettingsPage(parent)
{
    setupUi();
}

void FontSettingsPage::setupUi()
{
    auto* layout = new QVBoxLayout(this);

    // ---- 字体选择 ----
    auto* formBox = new QGroupBox(tr("跟打区字体"), this);
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
    auto* previewBox = new QGroupBox(tr("预览"), this);
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
            this, &FontSettingsPage::onFontFamilyChanged);
    connect(m_fontSizeSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, &FontSettingsPage::onFontSizeChanged);
    connect(m_fontBoldCheck, &QCheckBox::toggled,
            this, &FontSettingsPage::onFontBoldChanged);
    connect(m_fontItalicCheck, &QCheckBox::toggled,
            this, &FontSettingsPage::onFontItalicChanged);
    connect(m_fontResetBtn, &QPushButton::clicked,
            this, &FontSettingsPage::onFontReset);
}

// ---------------------------------------------------------------
// 配置 ↔ 控件
// ---------------------------------------------------------------
void FontSettingsPage::loadFromConfig()
{
    auto& cfg = ConfigManager::instance();
    QFont f = cfg.typingFont();
    if (f.family().isEmpty()) f = font();

    // 阻断信号，避免 load 时触发预览
    {
        QSignalBlocker b1(m_fontCombo);
        QSignalBlocker b2(m_fontSizeSpin);
        QSignalBlocker b3(m_fontBoldCheck);
        QSignalBlocker b4(m_fontItalicCheck);

        m_fontCombo->setCurrentFont(f);
        m_fontSizeSpin->setValue(f.pointSize());
        m_fontBoldCheck->setChecked(f.bold());
        m_fontItalicCheck->setChecked(f.italic());
    }

    // 保存快照
    m_originalFont = f;

    // 刷新预览（不 emit，避免外层重复设置）
    m_fontPreviewLabel->setFont(f);
    m_fontPreviewEdit->setFont(f);
}

void FontSettingsPage::saveToConfig()
{
    ConfigManager::instance().setTypingFont(currentFont());
}

void FontSettingsPage::resetToDefault()
{
    QFont def;
    def.setFamily(QString());   // 系统默认
    def.setPointSize(18);
    def.setBold(false);
    def.setItalic(false);

    {
        QSignalBlocker b1(m_fontCombo);
        QSignalBlocker b2(m_fontSizeSpin);
        QSignalBlocker b3(m_fontBoldCheck);
        QSignalBlocker b4(m_fontItalicCheck);

        m_fontCombo->setCurrentFont(QFont());
        m_fontSizeSpin->setValue(18);
        m_fontBoldCheck->setChecked(false);
        m_fontItalicCheck->setChecked(false);
    }

    emitFontPreview();
}

// ---------------------------------------------------------------
// 取消时恢复
// ---------------------------------------------------------------
void FontSettingsPage::restoreSnapshot()
{
    QFont f = m_originalFont;
    if (f.family().isEmpty()) {
        f = QApplication::font();
        f.setPointSize(18);
    }
    emit fontPreview(f);
}

// ---------------------------------------------------------------
// 事件
// ---------------------------------------------------------------
void FontSettingsPage::onFontFamilyChanged() { emitFontPreview(); }
void FontSettingsPage::onFontSizeChanged(int) { emitFontPreview(); }
void FontSettingsPage::onFontBoldChanged(bool) { emitFontPreview(); }
void FontSettingsPage::onFontItalicChanged(bool) { emitFontPreview(); }
void FontSettingsPage::onFontReset() { resetToDefault(); }

QFont FontSettingsPage::currentFont() const
{
    QFont f;
    f.setFamily(m_fontCombo->currentFont().family());
    f.setPointSize(m_fontSizeSpin->value());
    f.setBold(m_fontBoldCheck->isChecked());
    f.setItalic(m_fontItalicCheck->isChecked());
    return f;
}

void FontSettingsPage::emitFontPreview()
{
    QFont f = currentFont();
    m_fontPreviewLabel->setFont(f);
    m_fontPreviewEdit->setFont(f);
    emit fontPreview(f);
    emit changed();
}