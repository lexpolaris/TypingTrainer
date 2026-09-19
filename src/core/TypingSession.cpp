// src/core/TypingSession.cpp
#include "TypingSession.h"

TypingSession::TypingSession(QObject* parent) : QObject(parent) {}

void TypingSession::start(const QString& targetText)
{
    m_target = targetText;
    m_currentIndex = 0;
    m_keystrokes = 0;
    m_errorChars = 0;
    m_backspaces = 0;
    m_snapshots.clear();
    m_hitSpeedPoints.clear();
    m_pausedElapsed = 0;
    m_state = Running;
    m_timer.start();
    emit stateChanged(m_state);
    emit positionChanged(0, true);
}

void TypingSession::reset()
{
    m_state = Idle;
    m_currentIndex = 0;
    m_keystrokes = 0;
    m_errorChars = 0;
    m_backspaces = 0;
    m_snapshots.clear();
    m_hitSpeedPoints.clear();
    emit stateChanged(m_state);
    emit positionChanged(0, true);
}

void TypingSession::pause()
{
    if (m_state != Running) return;
    m_pausedElapsed += m_timer.elapsed();
    m_state = Paused;
    emit stateChanged(m_state);
}

void TypingSession::resume()
{
    if (m_state != Paused) return;
    m_timer.restart();
    m_state = Running;
    emit stateChanged(m_state);
}

double TypingSession::elapsedSeconds() const
{
    qint64 ms = m_pausedElapsed;
    if (m_state == Running) ms += m_timer.elapsed();
    return ms / 1000.0;
}

bool TypingSession::inputCharacter(QChar ch)
{
    if (m_state != Running) return false;
    if (m_currentIndex >= m_target.length()) return false;

    ++m_keystrokes;
    QChar expected = m_target.at(m_currentIndex);

    if (ch == expected) {
        ++m_currentIndex;
        checkSpeedPoint();
        emit positionChanged(m_currentIndex - 1, true);
        if (m_currentIndex >= m_target.length()) {
            m_state = Finished;
            emit stateChanged(m_state);
            emit finished();
        }
        return true;
    } else {
        ++m_errorChars;
        emit positionChanged(m_currentIndex, false);
        return false;
    }
}

bool TypingSession::backspace()
{
    if (m_state != Running) return false;
    if (m_currentIndex <= 0) return false;
    --m_currentIndex;
    ++m_backspaces;
    emit positionChanged(m_currentIndex, true);
    return true;
}

void TypingSession::skipToPosition(int pos)
{
    if (pos < 0 || pos > m_target.length()) return;
    m_currentIndex = pos;
    emit positionChanged(m_currentIndex, true);
}

void TypingSession::setSpeedPoints(const QVector<int>& positions)
{
    m_speedPoints = positions;
    m_hitSpeedPoints.clear();
}

void TypingSession::checkSpeedPoint()
{
    int pos = m_currentIndex - 1;
    if (!m_speedPoints.contains(pos)) return;
    if (m_hitSpeedPoints.contains(pos)) return;

    m_hitSpeedPoints.insert(pos);
    m_snapshots.append({
        m_currentIndex,
        elapsedSeconds(),
        m_keystrokes,
        m_backspaces
    });
}

double TypingSession::speedCPM() const
{
    double sec = elapsedSeconds();
    if (sec <= 0) return 0;
    return correctChars() * 60.0 / sec;
}

double TypingSession::keystrokePerSec() const
{
    double sec = elapsedSeconds();
    return sec > 0 ? m_keystrokes / sec : 0;
}

double TypingSession::codeLength() const
{
    int c = correctChars();
    return c > 0 ? double(m_keystrokes) / c : 0;
}
