// src/app/MusicPlayer.cpp
#include "MusicPlayer.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFileInfo>
#include <QUrl>

MusicPlayer::MusicPlayer(QObject* parent) : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audio  = new QAudioOutput(this);
    m_player->setAudioOutput(m_audio);
    m_audio->setVolume(0.5);

    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus s) {
        onMediaStatusChanged(int(s));
    });

    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, [this](QMediaPlayer::PlaybackState) {
        emit stateChanged();
    });

    connect(m_player, &QMediaPlayer::errorOccurred,
            this, [this](QMediaPlayer::Error, const QString& msg) {
        emit errorOccurred(msg);
    });
}

MusicPlayer::~MusicPlayer() = default;

// ---------------------------------------------------------------
// 列表管理
// ---------------------------------------------------------------
void MusicPlayer::setPlaylist(const QStringList& files)
{
    m_playlist = files;
    if (m_currentIndex >= m_playlist.size())
        m_currentIndex = m_playlist.isEmpty() ? -1 : 0;
    emit playlistChanged();
}

QString MusicPlayer::currentTrack() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size())
        return {};
    return m_playlist[m_currentIndex];
}

// ---------------------------------------------------------------
// 播放控制
// ---------------------------------------------------------------
void MusicPlayer::play()
{
    if (m_playlist.isEmpty()) return;

    // 当前没有源或索引变化时，加载新文件
    if (m_currentIndex < 0) m_currentIndex = 0;

    const QString path = m_playlist[m_currentIndex];
    if (m_player->source() != QUrl::fromLocalFile(path)) {
        m_player->setSource(QUrl::fromLocalFile(path));
        emit trackChanged(path, m_currentIndex);
    }

    m_player->play();
}

void MusicPlayer::pause()
{
    m_player->pause();
}

void MusicPlayer::togglePlayPause()
{
    if (isPlaying()) pause();
    else             play();
}

void MusicPlayer::stop()
{
    m_player->stop();
}

void MusicPlayer::next()
{
    if (m_playlist.isEmpty()) return;

    m_currentIndex = (m_currentIndex + 1) % m_playlist.size();
    m_player->setSource(QUrl::fromLocalFile(m_playlist[m_currentIndex]));
    emit trackChanged(m_playlist[m_currentIndex], m_currentIndex);
    m_player->play();
}

void MusicPlayer::previous()
{
    if (m_playlist.isEmpty()) return;

    m_currentIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
    m_player->setSource(QUrl::fromLocalFile(m_playlist[m_currentIndex]));
    emit trackChanged(m_playlist[m_currentIndex], m_currentIndex);
    m_player->play();
}

void MusicPlayer::playFile(const QString& path)
{
    int idx = m_playlist.indexOf(path);
    if (idx < 0) {
        // 不在列表中，加进去
        m_playlist.append(path);
        idx = m_playlist.size() - 1;
        emit playlistChanged();
    }
    m_currentIndex = idx;
    m_player->setSource(QUrl::fromLocalFile(path));
    emit trackChanged(path, idx);
    m_player->play();
}

// ---------------------------------------------------------------
// 属性
// ---------------------------------------------------------------
bool MusicPlayer::isPlaying() const
{
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

int MusicPlayer::volume() const
{
    return int(m_audio->volume() * 100);
}

void MusicPlayer::setVolume(int v)
{
    m_audio->setVolume(qBound(0, v, 100) / 100.0);
}

void MusicPlayer::setLoopMode(LoopMode m)
{
    m_loopMode = m;
}

// ---------------------------------------------------------------
// 自动切歌
// ---------------------------------------------------------------
void MusicPlayer::onMediaStatusChanged(int status)
{
    if (status != int(QMediaPlayer::EndOfMedia)) return;

    switch (m_loopMode) {
    case NoLoop:
        m_player->stop();
        break;
    case LoopOne:
        m_player->setPosition(0);
        m_player->play();
        break;
    case LoopAll:
        next();
        break;
    }
}