// src/ui/SettingsDialog.h
#pragma once

#include <QDialog>
#include <QFont>
#include <QColor>
#include <QHash>
#include <QVector>

class QTabWidget;
class SettingsPage;
class FontSettingsPage;
class ThemeSettingsPage;
class CodeTableSettingsPage;

// 类型别名，避免 moc 解析嵌套模板
using ThemeColorMap = QHash<int, QColor>;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override;

signals:
    // 对外接口保持不变，MainWindow 无需改动
    void fontPreview(const QFont& f);
    void themeModePreview(int mode);
    void themeColorsPreview(const ThemeColorMap& colors);

private slots:
    void onAccept();
    void onCancel();

private:
    void setupUi();
    void addPage(SettingsPage* page);
    void connectPreviewSignals();

    // 数据
    QTabWidget*                 m_tabs = nullptr;
    QVector<SettingsPage*>      m_pages;

    // 具体页面（用于取快照）
    FontSettingsPage*           m_fontPage = nullptr;
    ThemeSettingsPage*          m_themePage = nullptr;
    CodeTableSettingsPage*      m_codeTablePage = nullptr;
};