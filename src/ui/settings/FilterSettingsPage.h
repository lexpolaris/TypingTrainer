// src/ui/settings/FilterSettingsPage.h
#pragma once

#include "SettingsPage.h"

class QCheckBox;
class QGroupBox;
class QLabel;

class FilterSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit FilterSettingsPage(QWidget* parent = nullptr);

    QString title() const override { return tr("过滤"); }
    bool isLivePreview() const override { return false; }

    void loadFromConfig() override;
    void saveToConfig() override;
    void resetToDefault() override;

private slots:
    void onFilterHanToggled(bool on);
    void onFilterNonHanToggled(bool on);

private:
    void setupUi();
    void connectMutualExclusion();
    void updateHintLabel();

    // 控件
    QCheckBox* m_filterHan     = nullptr;
    QCheckBox* m_filterNonHan  = nullptr;
    QCheckBox* m_filterUpper   = nullptr;
    QCheckBox* m_filterLower   = nullptr;
    QCheckBox* m_filterDigit   = nullptr;
    QCheckBox* m_filterSpace   = nullptr;
    QCheckBox* m_upperToLower  = nullptr;
    QCheckBox* m_lowerToUpper  = nullptr;
    QLabel*    m_hintLabel     = nullptr;
};