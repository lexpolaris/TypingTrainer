// src/ui/settings/CodeTableSettingsPage.h
#pragma once

#include "SettingsPage.h"

class QComboBox;
class QPushButton;

class CodeTableSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit CodeTableSettingsPage(QWidget* parent = nullptr);

    QString title() const override { return tr("码表"); }
    bool isLivePreview() const override { return false; }

    void loadFromConfig() override;
    void saveToConfig() override;
    void resetToDefault() override;

private slots:
    void onImportClicked();
    void onClearClicked();

private:
    void setupUi();
    void reloadCodeTableList();

    QComboBox*    m_codeTableCombo = nullptr;
    QPushButton*  m_codeTableImportBtn = nullptr;
    QPushButton*  m_codeTableClearBtn = nullptr;
};