// src/app/MusicPlayer.h
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QMediaPlayer;
class QAudioOutput;

/// 背景音乐播放器
///
/// - 支持播放列表、上一首/下一首、循环、音量
/// - 状态持久化交给 ConfigManager，本类只负责运行时行为
class MusicPlayer : public QObject
{
    Q_OBJECT
public:
    enum LoopMode { NoLoop, LoopAll, LoopOne };
    Q_ENUM(LoopMode)

    explicit MusicPlayer(QObject* parent = nullptr);
    ~MusicPlayer() override;

    // ---- 列表管理 ----
    void setPlaylist(const QStringList& files);
    QStringList playlist() const { return m_playlist; }
    int currentIndex() const { return m_currentIndex; }
    QString currentTrack() const;

    // ---- 播放控制 ----
    void play();
    void pause();
    void togglePlayPause();
    void stop();
    void next();
    void previous();

    // ---- 属性 ----
    bool isPlaying() const;
    int  volume() const;              // 0..100
    void setVolume(int v);
    LoopMode loopMode() const { return m_loopMode; }
    void setLoopMode(LoopMode m);

    /// 播放指定文件（会尝试匹配列表中的位置）
    void playFile(const QString& path);

signals:
    void stateChanged();              // 播放/暂停/停止状态变化
    void trackChanged(const QString& path, int index);
    void playlistChanged();
    void errorOccurred(const QString& message);

private slots:
    void onMediaStatusChanged(int status);   // QMediaPlayer::MediaStatus

private:
    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audio = nullptr;

    QStringList m_playlist;
    int         m_currentIndex = -1;
    LoopMode    m_loopMode = LoopAll;
};