// src/core/TypingSession.h
#pragma once
#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <QVector>
#include <QSet>

struct SpeedPointSnapshot
{
    int    position = 0;
    double timeSeconds = 0;
    int    keystrokes = 0;
    int    backspaces = 0;
};

class TypingSession : public QObject
{
    Q_OBJECT
public:
    enum State { Idle, Running, Paused, Finished };
    Q_ENUM(State)

    explicit TypingSession(QObject* parent = nullptr);

    void start(const QString& targetText);
    void reset();
    void pause();
    void resume();

    bool inputCharacter(QChar ch);
    bool backspace();
    void skipToPosition(int pos);

    State state() const { return m_state; }
    int currentIndex() const { return m_currentIndex; }
    int totalLength() const { return m_target.length(); }
    const QString& target() const { return m_target; }

    // 实时统计
    double elapsedSeconds() const;
    int    totalKeystrokes() const { return m_keystrokes; }
    int    errorChars() const { return m_errorChars; }
    int    backspaceCount() const { return m_backspaces; }
    int    correctChars() const { return m_currentIndex - m_errorChars; }

    double speedCPM() const;
    double keystrokePerSec() const;
    double codeLength() const;

    // 测速点
    void setSpeedPoints(const QVector<int>& positions);
    const QVector<SpeedPointSnapshot>& snapshots() const { return m_snapshots; }

signals:
    void positionChanged(int index, bool correct);
    void stateChanged(State s);
    void finished();

private:
    void checkSpeedPoint();

    QString m_target;
    int     m_currentIndex = 0;
    int     m_keystrokes = 0;
    int     m_errorChars = 0;
    int     m_backspaces = 0;
    State   m_state = Idle;
    QElapsedTimer m_timer;
    qint64  m_pausedElapsed = 0;

    QVector<int> m_speedPoints;
    QVector<SpeedPointSnapshot> m_snapshots;
    QSet<int> m_hitSpeedPoints;
};
