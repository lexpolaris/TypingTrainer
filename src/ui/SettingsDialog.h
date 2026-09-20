// src/ui/SettingsDialog.h
#pragma once

#include <QDialog>
#include <QFont>
#include <QColor>
#include <QHash>

// 类型别名，避免 moc 解析嵌套模板
using ThemeColorMap = QHash<int, QColor>;

class QTabWidget;
class QFontComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;
class QPlainTextEdit;
class QComboBox;
class QTableWidget;
class QPushButton;
class QLineEdit;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override;

signals:
    void fontPreview(const QFont& f);
    void themeModePreview(int mode);
    void themeColorsPreview(const ThemeColorMap& colors);

private slots:
    void onFontFamilyChanged();
    void onFontSizeChanged(int size);
    void onFontBoldChanged(bool);
    void onFontItalicChanged(bool);
    void onFontReset();

    void onThemeModeChanged(int idx);
    void onCustomColorClicked(int row, int col);
    void onCustomColorReset(int row);
    void onThemeResetAll();

    void onAccept();
    void onCancel();

private:
    void setupUi();
    QWidget* buildFontPage();
    QWidget* buildThemePage();
    QWidget* buildCodeTablePage();

    void loadFromConfig();
    void saveToConfig();
    void reloadCodeTableList();

    void emitFontPreview();
    void emitThemePreview();

    // ---- 快照（用于取消恢复） ----
    QFont          m_originalFont;
    int            m_originalThemeMode = 0;
    ThemeColorMap  m_originalCustomColors;

    // ---- 字体页控件 ----
    QFontComboBox*   m_fontCombo = nullptr;
    QSpinBox*        m_fontSizeSpin = nullptr;
    QCheckBox*       m_fontBoldCheck = nullptr;
    QCheckBox*       m_fontItalicCheck = nullptr;
    QPlainTextEdit*  m_fontPreviewEdit = nullptr;
    QLabel*          m_fontPreviewLabel = nullptr;
    QPushButton*     m_fontResetBtn = nullptr;

    // ---- 主题页控件 ----
    QComboBox*       m_themeModeCombo = nullptr;
    QTableWidget*    m_colorTable = nullptr;
    QPushButton*     m_themeResetBtn = nullptr;

    // ---- 码表页控件 ----
    QComboBox*       m_codeTableCombo = nullptr;
    QPushButton*     m_codeTableImportBtn = nullptr;
    QPushButton*     m_codeTableClearBtn = nullptr;
};