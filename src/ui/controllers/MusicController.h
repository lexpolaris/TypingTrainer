// src/ui/controllers/MusicController.h
#pragma once

#include <QObject>
#include <QList>
#include <QStringList>

class QLabel;
class QToolButton;
class QWidget;

class MusicPlayer;

/// 背景音乐控制器
///
/// 职责：
///   - 拥有并封装 MusicPlayer
///   - 提供状态栏音乐控件（曲名 + 上一首/播放/下一首）
///   - 播放控制（播放/暂停、上一首、下一首）
///
/// 与 MainWindow 通过信号槽通信：
///   - 宿主把 statusWidgets() 返回的控件加入状态栏
///   - 宿主连接 openLibraryRequested 以弹出音乐库对话框
///   - 宿主在关闭时调用 persistToConfig() 持久化运行时状态
class MusicController : public QObject
{
    Q_OBJECT
public:
    explicit MusicController(QObject* parent = nullptr);

    /// 供宿主加入状态栏的控件（曲名、上一首、播放/暂停、下一首）
    QList<QWidget*> statusWidgets() const;

    /// 启动时按配置恢复列表/音量/循环模式，并（可选）延迟自动播放
    void restoreFromConfig(bool autoPlayOnStartup);

    /// 关闭时把当前音量/循环/列表/索引写回配置（不调用 save）
    void persistToConfig() const;

    /// 更新播放列表（例如从音乐库对话框返回后）
    void setPlaylist(const QStringList& files);

    /// 列表是否为空
    bool isPlaylistEmpty() const;

    MusicPlayer* player() const { return m_player; }

public slots:
    void togglePlayPause();
    void next();
    void previous();

    /// 播放指定文件（来自音乐库对话框等）
    void playFile(const QString& path);

    /// 根据播放状态/列表刷新按钮与曲名
    void refreshUi();

signals:
    /// 用户点击播放但列表为空 → 宿主应打开音乐库
    void openLibraryRequested();
    /// 播放出错 → 宿主可在状态栏提示
    void errorMessage(const QString& message);

private:
    MusicPlayer* m_player = nullptr;

    QLabel*      m_trackLabel = nullptr;
    QToolButton* m_prevBtn = nullptr;
    QToolButton* m_playBtn = nullptr;
    QToolButton* m_nextBtn = nullptr;
};
