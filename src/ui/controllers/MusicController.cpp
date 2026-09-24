// src/ui/controllers/MusicController.cpp
#include "MusicController.h"

#include "app/MusicPlayer.h"
#include "app/ConfigManager.h"

#include <QLabel>
#include <QToolButton>
#include <QWidget>
#include <QStyle>
#include <QFileInfo>
#include <QTimer>

MusicController::MusicController(QObject* parent) : QObject(parent)
{
    m_player = new MusicPlayer(this);

    // 曲名标签
    m_trackLabel = new QLabel(tr("（无音乐）"));
    m_trackLabel->setMinimumWidth(120);
    m_trackLabel->setMaximumWidth(200);
    m_trackLabel->setStyleSheet("color: palette(mid);");

    // 按钮（用标准图标，避免额外图片资源）
    m_prevBtn = new QToolButton;
    m_prevBtn->setIcon(m_prevBtn->style()->standardIcon(QStyle::SP_MediaSkipBackward));
    m_prevBtn->setToolTip(tr("上一首"));
    m_prevBtn->setAutoRaise(true);

    m_playBtn = new QToolButton;
    m_playBtn->setIcon(m_playBtn->style()->standardIcon(QStyle::SP_MediaPlay));
    m_playBtn->setToolTip(tr("播放/暂停"));
    m_playBtn->setAutoRaise(true);

    m_nextBtn = new QToolButton;
    m_nextBtn->setIcon(m_nextBtn->style()->standardIcon(QStyle::SP_MediaSkipForward));
    m_nextBtn->setToolTip(tr("下一首"));
    m_nextBtn->setAutoRaise(true);

    // 连接播放器状态
    connect(m_player, &MusicPlayer::stateChanged,
            this, &MusicController::refreshUi);
    connect(m_player, &MusicPlayer::trackChanged,
            this, [this](const QString& path, int) {
        m_trackLabel->setText(QFileInfo(path).fileName());
        m_trackLabel->setToolTip(path);
        refreshUi();
    });
    connect(m_player, &MusicPlayer::playlistChanged,
            this, &MusicController::refreshUi);
    connect(m_player, &MusicPlayer::errorOccurred,
            this, [this](const QString& msg) {
        emit errorMessage(tr("音乐播放错误: %1").arg(msg));
    });

    // 按钮点击
    connect(m_prevBtn, &QToolButton::clicked,
            this, &MusicController::previous);
    connect(m_playBtn, &QToolButton::clicked,
            this, &MusicController::togglePlayPause);
    connect(m_nextBtn, &QToolButton::clicked,
            this, &MusicController::next);

    refreshUi();
}

QList<QWidget*> MusicController::statusWidgets() const
{
    return {m_trackLabel, m_prevBtn, m_playBtn, m_nextBtn};
}

void MusicController::restoreFromConfig(bool autoPlayOnStartup)
{
    auto& cfg = ConfigManager::instance();
    m_player->setPlaylist(cfg.musicFiles());
    m_player->setVolume(cfg.musicVolume());
    m_player->setLoopMode(
        static_cast<MusicPlayer::LoopMode>(cfg.musicLoopMode()));

    if (!autoPlayOnStartup) return;

    // 延迟启动播放（等窗口显示后）
    QTimer::singleShot(300, this, [this]() {
        auto& cfg = ConfigManager::instance();
        if (!cfg.playBgMusicOnStartup()) return;
        if (m_player->playlist().isEmpty()) return;

        const int lastIdx = cfg.musicCurrentIndex();
        if (lastIdx >= 0 && lastIdx < m_player->playlist().size())
            m_player->playFile(m_player->playlist().at(lastIdx));
        else
            m_player->play();
    });
}

void MusicController::persistToConfig() const
{
    auto& cfg = ConfigManager::instance();
    cfg.setMusicVolume(m_player->volume());
    cfg.setMusicLoopMode(int(m_player->loopMode()));
    cfg.setMusicFiles(m_player->playlist());
    cfg.setMusicCurrentIndex(m_player->currentIndex());
}

void MusicController::setPlaylist(const QStringList& files)
{
    m_player->setPlaylist(files);
    refreshUi();
}

bool MusicController::isPlaylistEmpty() const
{
    return m_player->playlist().isEmpty();
}

void MusicController::togglePlayPause()
{
    if (m_player->playlist().isEmpty()) {
        emit openLibraryRequested();
        return;
    }
    m_player->togglePlayPause();
}

void MusicController::next()
{
    m_player->next();
}

void MusicController::previous()
{
    m_player->previous();
}

void MusicController::playFile(const QString& path)
{
    m_player->playFile(path);
}

void MusicController::refreshUi()
{
    const bool playing = m_player->isPlaying();
    m_playBtn->setText(playing ? tr("暂停") : tr("播放"));
    m_playBtn->setToolTip(playing ? tr("暂停") : tr("播放"));

    const bool hasMusic = !m_player->playlist().isEmpty();
    m_prevBtn->setEnabled(hasMusic);
    m_playBtn->setEnabled(hasMusic);
    m_nextBtn->setEnabled(hasMusic);

    if (!hasMusic)
        m_trackLabel->setText(tr("（无音乐）"));
}
