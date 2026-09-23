// src/ui/settings/StartupSettingsPage.h
#pragma once

#include "SettingsPage.h"

class QCheckBox;
class QComboBox;
class QSpinBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QLabel;

class StartupSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit StartupSettingsPage(QWidget* parent = nullptr);

    QString title() const override { return tr("启动"); }
    bool isLivePreview() const override { return false; }

    void loadFromConfig() override;
    void saveToConfig() override;
    void resetToDefault() override;

private slots:
    void onLoadLastTextToggled(bool on);
    void onPlayBgMusicToggled(bool on);
    void onCountdownToggled(bool on);
    void onImportCodeTable();
    void onClearCodeTable();

private:
    void setupUi();
    void reloadCodeTableList();
    void updateControlStates();

    // ---- 启动时 ----
    QCheckBox*   m_loadLastText = nullptr;

    QCheckBox*   m_playBgMusic = nullptr;
    
    // ---- 打开文章时 ----
    QRadioButton* m_openFromStart = nullptr;
    QRadioButton* m_openRandom   = nullptr;
    QRadioButton* m_openBreak    = nullptr;
    QLabel*       m_openHint     = nullptr;

    // ---- 倒计时 ----
    QCheckBox*   m_countdownEnabled = nullptr;
    QSpinBox*    m_countdownMinutes = nullptr;

    // ---- 码表 ----
    QComboBox*   m_codeTableCombo = nullptr;
    QPushButton* m_codeTableImportBtn = nullptr;
    QPushButton* m_codeTableClearBtn = nullptr;
};