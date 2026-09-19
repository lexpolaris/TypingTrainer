#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>

class QListWidget;
class QListWidgetItem;
class QTextEdit;
class QLineEdit;
class QLabel;
class QPushButton;

class TextLibraryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextLibraryDialog(QWidget* parent = nullptr);

    /// 用户确认打开的文件路径（内置文本用 ":/texts/xxx" 表示）
    QString selectedPath() const { return m_selectedPath; }

    /// 是否为内置资源
    bool isResource() const { return m_selectedPath.startsWith(":/"); }

signals:
    void textChosen(const QString& path);

private slots:
    void onSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onSearchChanged(const QString& keyword);
    void onImport();
    void onDelete();
    void onRename();
    void onOpen();
    void onRefresh();

private:
    void setupUi();
    void loadTextList();
    void addTextItem(const QString& displayName,
                     const QString& fullPath,
                     bool isResource);
    void updatePreview(const QString& path, bool isResource);
    void updateStatusLabel();
    QString userTextDir() const;

    // UI
    QLineEdit*    m_searchEdit = nullptr;
    QListWidget*  m_listWidget = nullptr;
    QTextEdit*    m_preview    = nullptr;
    QLabel*       m_statusLabel = nullptr;
    QPushButton*  m_btnOpen    = nullptr;
    QPushButton*  m_btnImport  = nullptr;
    QPushButton*  m_btnDelete  = nullptr;
    QPushButton*  m_btnRename  = nullptr;

    QString m_selectedPath;
    bool    m_selectedIsResource = false;
};