// src/ui/controllers/HistoryController.cpp
#include "HistoryController.h"

#include "core/TypingSession.h"
#include "core/HistoryDb.h"

#include <QDateTime>

HistoryController::HistoryController(QObject* parent) : QObject(parent)
{
}

void HistoryController::save(const TypingSession* session,
                             const QString& docName,
                             const QString& docSource)
{
    if (!session || session->target().isEmpty()) return;
    if (docName.isEmpty()) return;

    // 过滤太短的记录
    if (session->currentIndex() < 10) return;

    HistoryEntry e;
    e.timestamp       = QDateTime::currentDateTime();
    e.docName         = docName;
    e.docSource       = docSource.isEmpty() ? docName : docSource;
    e.charCount       = session->currentIndex()
                        - session->initialStartIndex();
    e.correct         = session->correctChars();
    e.errors          = session->errorChars();
    e.backspaces      = session->backspaceCount();
    e.durationSeconds = session->elapsedSeconds();
    e.speedCPM        = session->speedCPM();
    e.keystrokes      = session->totalKeystrokes();

    const int total = e.charCount;
    e.accuracy = (total > 0) ? (total - e.errors) * 100.0 / total : 0.0;
    if (e.accuracy < 0) e.accuracy = 0;

    e.codeLength = (e.correct > 0)
                       ? double(e.keystrokes) / e.correct
                       : 0.0;

    if (!HistoryDb::instance().add(e)) {
        qWarning() << "保存历史失败:" << HistoryDb::instance().lastError();
    }
}