// src/ui/settings/ThemeSettingsPage.h
#pragma once

#include "SettingsPage.h"
#include <QColor>
#include <QHash>

class QComboBox;
class QTableWidget;
class QPushButton;

class ThemeSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit ThemeSettingsPage(QWidget* parent = nullptr);

    QString title() const override { return tr("主题"); }
    bool isLivePreview() const override { return true; }

    void loadFromConfig() override;
    void saveToConfig() override;
    void resetToDefault() override;

    /// 取消时由 SettingsDialog 调用
    void restoreSnapshot();

signals:
    void modePreview(int mode);                     // ThemeManager::Mode 值
    void colorsPreview(const QHash<int, QColor>&);  // Role → Color

private slots:
    void onThemeModeChanged(int idx);
    void onCustomColorClicked(int row);
    void onCustomColorReset(int row);
    void onThemeResetAll();

private:
    void setupUi();
    void refreshColorButtons(const QHash<int, QColor>& custom);
    void updateColorButton(int row, const QColor& c);
    void emitModePreview();
    void emitColorsPreview();

    // 控件
    QComboBox*    m_themeModeCombo = nullptr;
    QTableWidget* m_colorTable = nullptr;
    QPushButton*  m_themeResetBtn = nullptr;

    // 快照
    int            m_originalMode = 0;
    QHash<int, QColor> m_originalColors;
};