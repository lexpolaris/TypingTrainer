// src/ui/settings/FontSettingsPage.h
#pragma once

#include "SettingsPage.h"
#include <QFont>

class QFontComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;

class FontSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit FontSettingsPage(QWidget* parent = nullptr);

    QString title() const override { return tr("字体"); }
    bool isLivePreview() const override { return true; }

    void loadFromConfig() override;
    void saveToConfig() override;
    void resetToDefault() override;

    /// 取消时由 SettingsDialog 调用：恢复原始字体并广播
    void restoreSnapshot();

signals:
    /// 字体变化（实时预览）
    void fontPreview(const QFont& f);

private slots:
    void onFontFamilyChanged();
    void onFontSizeChanged(int);
    void onFontBoldChanged(bool);
    void onFontItalicChanged(bool);
    void onFontReset();

private:
    void setupUi();
    void emitFontPreview();
    QFont currentFont() const;

    // 控件
    QFontComboBox*   m_fontCombo = nullptr;
    QSpinBox*        m_fontSizeSpin = nullptr;
    QCheckBox*       m_fontBoldCheck = nullptr;
    QCheckBox*       m_fontItalicCheck = nullptr;
    QLabel*          m_fontPreviewLabel = nullptr;
    QPlainTextEdit*  m_fontPreviewEdit = nullptr;
    QPushButton*     m_fontResetBtn = nullptr;

    // 快照（取消时恢复）
    QFont m_originalFont;
};