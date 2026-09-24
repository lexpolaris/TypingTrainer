// src/core/TypingSession.cpp
#include "TypingSession.h"
#include <algorithm>

TypingSession::TypingSession(QObject* parent) : QObject(parent)
{
    m_timeSpeedTimer = new QTimer(this);
    m_timeSpeedTimer->setSingleShot(false);
    connect(m_timeSpeedTimer, &QTimer::timeout,
            this, &TypingSession::onTimeSpeedTick);
    
    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setSingleShot(true);
    connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
        if (m_state == Running) {
            m_state = Finished;
            m_pausedElapsed += m_timer.elapsed();
            m_timeSpeedTimer->stop();
            emit stateChanged(m_state);
            emit countdownFinished();
            emit finished();
        }
    });
}

void TypingSession::onTimeSpeedTick()
{
    if (m_state != Running) return;
    m_snapshots.append({
        m_currentIndex,
        elapsedSeconds(),
        m_keystrokes,
        m_backspaces
    });
}

void TypingSession::setCountdown(int minutes)
{
    m_countdownMinutes = qMax(0, minutes);
}

// ---------------------------------------------------------------
// 开始 / 重置
// ---------------------------------------------------------------
void TypingSession::start(const QString& targetText)
{
    startFrom(targetText, 0);
}

void TypingSession::startFrom(const QString& targetText, int startIndex)
{
    m_target = targetText;
    m_initialStartIndex = qBound(0, startIndex, qMax(0, targetText.length() - 1));
    m_currentIndex = m_initialStartIndex;
    m_keystrokes = 0;
    m_errorChars = 0;
    m_backspaces = 0;
    m_snapshots.clear();
    m_hitSpeedPoints.clear();
    m_mistakes.clear();
    m_userInput.clear();
    m_pausedElapsed = 0;
    m_state = Running;
    m_timer.start();
    if (m_speedPointMode == TimeBased)
        m_timeSpeedTimer->start(m_timeIntervalSec * 1000);

    if (m_countdownMinutes > 0)
        m_countdownTimer->start(m_countdownMinutes * 60 * 1000);

    emit stateChanged(m_state);
    emit positionChanged(m_currentIndex, true);
}

void TypingSession::retry()
{
    if (m_target.isEmpty()) return;
    startFrom(m_target, m_initialStartIndex);
}

void TypingSession::reset()
{
    m_state = Idle;
    m_currentIndex = 0;
    m_initialStartIndex = 0;
    m_keystrokes = 0;
    m_errorChars = 0;
    m_backspaces = 0;
    m_snapshots.clear();
    m_hitSpeedPoints.clear();
    m_mistakes.clear();
    m_userInput.clear();
    m_timeSpeedTimer->stop();
    m_countdownTimer->stop();
    emit stateChanged(m_state);
    emit positionChanged(0, true);
}

void TypingSession::pause()
{
    if (m_state != Running) return;
    m_pausedElapsed += m_timer.elapsed();
    m_state = Paused;
    m_timeSpeedTimer->stop();
    m_countdownTimer->stop();
    emit stateChanged(m_state);
}

void TypingSession::resume()
{
    if (m_state != Paused) return;
    m_timer.restart();
    if (m_speedPointMode == TimeBased)
        m_timeSpeedTimer->start(m_timeIntervalSec * 1000);
    
    if (m_countdownMinutes > 0)
        m_countdownTimer->start(m_countdownMinutes * 60 * 1000);
    
    m_state = Running;
    emit stateChanged(m_state);
}

double TypingSession::elapsedSeconds() const
{
    qint64 ms = m_pausedElapsed;
    if (m_state == Running) ms += m_timer.elapsed();
    return ms / 1000.0;
}

// ---------------------------------------------------------------
// 输入
// ---------------------------------------------------------------
bool TypingSession::inputCharacter(QChar ch)
{
    if (m_state != Running) return false;
    if (m_currentIndex >= m_target.length()) return false;

    // 先跳过换行，再判断是否已完成
    while (m_currentIndex < m_target.length() &&
           m_target.at(m_currentIndex) == '\n') {
        ++m_currentIndex;
    }
    if (m_currentIndex >= m_target.length()) {
        if (m_state != Finished) {
            m_state = Finished;
            m_pausedElapsed += m_timer.elapsed();
            m_timeSpeedTimer->stop();
            emit stateChanged(m_state);
            emit finished();
        }
        return false;
    }
    // 跳过后再判定能否接受输入
    if (m_state != Running) return false;

    // 记录用户输入（无论对错都记）
    m_userInput[m_currentIndex] = ch;

    ++m_keystrokes;
    const QChar expected = m_target.at(m_currentIndex);

    if (ch == expected) {
        ++m_currentIndex;
        checkSpeedPoint();
        emit positionChanged(m_currentIndex - 1, true);

        // 检查末尾
        int probe = m_currentIndex;
        while (probe < m_target.length() && m_target.at(probe) == '\n')
            ++probe;
        if (probe >= m_target.length()) {
            m_currentIndex = probe;
            m_state = Finished;
            m_pausedElapsed += m_timer.elapsed();
            m_timeSpeedTimer->stop();
            emit stateChanged(m_state);
            emit finished();
        }
        return true;
    } else {
        ++m_errorChars;
        recordMistake(m_currentIndex, expected, ch);
        emit positionChanged(m_currentIndex, false);
        return false;
    }
}

bool TypingSession::backspace()
{
    if (m_state == Idle) return false;
    if (m_currentIndex <= m_initialStartIndex) return false;

    --m_currentIndex;
    while (m_currentIndex > m_initialStartIndex &&
           m_target.at(m_currentIndex) == '\n') {
        --m_currentIndex;
    }

    // 移除该位置的用户输入
    m_userInput.remove(m_currentIndex);

    ++m_backspaces;
    if (m_state == Finished) {
        m_state = Running;
        m_timer.restart();
        emit stateChanged(m_state);
    }
    emit positionChanged(m_currentIndex, true);
    return true;
}

void TypingSession::skipToPosition(int pos)
{
    if (pos < 0 || pos > m_target.length()) return;
    m_currentIndex = pos;
    emit positionChanged(m_currentIndex, true);
}

// ---------------------------------------------------------------
// 错字记录
// ---------------------------------------------------------------
void TypingSession::recordMistake(int position, QChar expected, QChar actual)
{
    // 查找该位置是否已有记录
    for (auto& m : m_mistakes) {
        if (m.position == position) {
            ++m.count;
            m.actual = actual;
            emit mistakeAdded(position);
            return;
        }
    }
    // 新记录
    MistakeRecord m;
    m.position = position;
    m.expected = expected;
    m.actual = actual;
    m.count = 1;
    m_mistakes.append(m);
    emit mistakeAdded(position);
}

// ---------------------------------------------------------------
// 测速点
// ---------------------------------------------------------------
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

// ---------------------------------------------------------------
// 统计
// ---------------------------------------------------------------
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

// ---------------------------------------------------------------
// 模式
// ---------------------------------------------------------------
void TypingSession::setSpeedPointMode(SpeedPointMode mode)
{
    if (m_speedPointMode == mode) return;
    m_speedPointMode = mode;

    // 运行时切换：停掉旧的，按新决定是否启动
    m_timeSpeedTimer->stop();
    if (m_speedPointMode == TimeBased && m_state == Running) {
        m_timeSpeedTimer->start(m_timeIntervalSec * 1000);
    }
}