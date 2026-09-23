// src/ui/MusicLibraryDialog.h
#pragma once

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QListWidget;
class QLabel;
class QPushButton;

class MusicLibraryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MusicLibraryDialog(QWidget* parent = nullptr);

    /// 用户确认后的播放列表（按列表顺序）
    QStringList playlist() const;

signals:
    /// 请求播放指定文件（双击时发射）
    void playRequested(const QString& path);

private slots:
    void onChooseFolder();
    void onRefresh();
    void onAddFile();
    void onRemoveSelected();
    void onClearAll();
    void onItemDoubleClicked();
    void onMoveUp();
    void onMoveDown();
    void onAccept();

private:
    void setupUi();
    void scanFolder(const QString& folder);
    void refreshList();
    void updateStatus();

    QLineEdit*    m_folderEdit = nullptr;
    QPushButton*  m_chooseFolderBtn = nullptr;
    QPushButton*  m_refreshBtn = nullptr;

    QListWidget*  m_listWidget = nullptr;

    QPushButton*  m_addBtn = nullptr;
    QPushButton*  m_removeBtn = nullptr;
    QPushButton*  m_clearBtn = nullptr;
    QPushButton*  m_upBtn = nullptr;
    QPushButton*  m_downBtn = nullptr;

    QLabel*       m_statusLabel = nullptr;
};