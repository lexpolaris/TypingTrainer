// src/core/TypingSession.h
#pragma once

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <QVector>
#include <QSet>
#include <QTimer>

struct SpeedPointSnapshot
{
    int    position = 0;
    double timeSeconds = 0;
    int    keystrokes = 0;
    int    backspaces = 0;
};

/// 错字记录
struct MistakeRecord
{
    int     position = 0;     // 字符位置
    QChar   expected;         // 期望字符
    QChar   actual;           // 最近一次的错误输入
    int     count = 0;        // 累计错误次数
};

class TypingSession : public QObject
{
    Q_OBJECT
public:
    enum State { Idle, Running, Paused, Finished };
    Q_ENUM(State)

    enum SpeedPointMode {
        PositionBased,   // 位置驱动（打到预设位置记录）
        TimeBased        // 时间驱动（每 N 秒记录一次）
    };
    Q_ENUM(SpeedPointMode)

    explicit TypingSession(QObject* parent = nullptr);

    /// 从头开始
    void start(const QString& targetText);

    /// 从指定位置开始（用于跳段）
    void startFrom(const QString& targetText, int startIndex);

    /// 重打当前段（回到 m_initialStartIndex）
    void retry();

    void reset();
    void pause();
    void resume();

    bool inputCharacter(QChar ch);
    bool backspace();

    /// 跳转光标位置（不重置统计）
    void skipToPosition(int pos);

    State state() const { return m_state; }
    int currentIndex() const { return m_currentIndex; }
    int initialStartIndex() const { return m_initialStartIndex; }
    int totalLength() const { return m_target.length(); }
    const QString& target() const { return m_target; }

    // 实时统计
    double elapsedSeconds() const;
    int    totalKeystrokes() const { return m_keystrokes; }
    int    errorChars() const { return m_errorChars; }
    int    backspaceCount() const { return m_backspaces; }
    int    correctChars() const {
        int net = m_currentIndex - m_initialStartIndex;
        return net > m_errorChars ? net - m_errorChars : 0;
    }

    double speedCPM() const;
    double keystrokePerSec() const;
    double codeLength() const;

    // 错字
    const QVector<MistakeRecord>& mistakes() const { return m_mistakes; }
    int mistakeCount() const { return m_mistakes.size(); }

    // 测速点
    void setSpeedPoints(const QVector<int>& positions);
    const QVector<SpeedPointSnapshot>& snapshots() const { return m_snapshots; }
    void setSpeedPointMode(SpeedPointMode mode) { m_speedPointMode = mode; }
    SpeedPointMode speedPointMode() const { return m_speedPointMode; }
    void setTimeInterval(int seconds) { m_timeIntervalSec = qMax(1, seconds); }
    int timeInterval() const { return m_timeIntervalSec; }
    
    // 倒计时
    void setCountdown(int minutes);   // 0 表示不启用
    int countdown() const;

signals:
    void positionChanged(int index, bool correct);
    void stateChanged(State s);
    void finished();
    void mistakeAdded(int position);   // 新错字 / 已有错字 count 增加
    void countdownFinished();  // 倒计时

private:
    void checkSpeedPoint();
    void recordMistake(int position, QChar expected, QChar actual);
    void onTimeSpeedTick();

    QString m_target;
    int     m_currentIndex = 0;
    int     m_initialStartIndex = 0;   // 供 retry 使用
    int     m_keystrokes = 0;
    int     m_errorChars = 0;
    int     m_backspaces = 0;
    State   m_state = Idle;
    QElapsedTimer m_timer;
    qint64  m_pausedElapsed = 0;

    QVector<int> m_speedPoints;
    QVector<SpeedPointSnapshot> m_snapshots;
    QSet<int> m_hitSpeedPoints;
    QVector<MistakeRecord> m_mistakes;

    SpeedPointMode m_speedPointMode = PositionBased;
    int m_timeIntervalSec = 20;
    QTimer* m_timeSpeedTimer = nullptr;

    int m_countdownMinutes = 0;
    QTimer* m_countdownTimer = nullptr;
};