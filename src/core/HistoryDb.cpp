// src/core/HistoryDb.cpp
#include "HistoryDb.h"
#include "utils/AppPaths.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

// ---------------------------------------------------------------
// 单例
// ---------------------------------------------------------------
HistoryDb& HistoryDb::instance()
{
    static HistoryDb inst;
    return inst;
}

HistoryDb::HistoryDb()
{
    const QString path = AppPaths::historyDbPath();

    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        m_lastError = QStringLiteral("SQLite 驱动不可用");
        qWarning() << "HistoryDb:" << m_lastError;
        return;
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE", "history");
    db.setDatabaseName(path);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        qWarning() << "HistoryDb 打开失败:" << m_lastError;
        return;
    }

    if (!ensureTable()) return;
    m_ready = true;
}

HistoryDb::~HistoryDb()
{
    if (QSqlDatabase::contains("history")) {
        {
            auto db = QSqlDatabase::database("history");
            if (db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase("history");
    }
}

bool HistoryDb::ensureTable()
{
    auto db = QSqlDatabase::database("history");
    QSqlQuery q(db);

    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS history (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp   TEXT    NOT NULL,
            doc_name    TEXT    NOT NULL,
            doc_source  TEXT,
            char_count  INTEGER NOT NULL,
            correct     INTEGER NOT NULL,
            errors      INTEGER NOT NULL,
            backspaces  INTEGER NOT NULL,
            duration_s  REAL    NOT NULL,
            speed_cpm   REAL    NOT NULL,
            accuracy    REAL    NOT NULL,
            keystrokes  INTEGER NOT NULL,
            code_length REAL    NOT NULL
        )
    )";

    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        qWarning() << "HistoryDb 建表失败:" << m_lastError;
        return false;
    }

    if (!q.exec("CREATE INDEX IF NOT EXISTS idx_timestamp "
                "ON history(timestamp DESC)")) {
        qWarning() << "HistoryDb 建索引失败:" << q.lastError().text();
        // 索引失败不算致命
    }

    return true;
}

// ---------------------------------------------------------------
// 写入
// ---------------------------------------------------------------
bool HistoryDb::add(const HistoryEntry& e)
{
    if (!m_ready) return false;

    auto db = QSqlDatabase::database("history");
    QSqlQuery q(db);

    q.prepare(R"(
        INSERT INTO history
            (timestamp, doc_name, doc_source,
             char_count, correct, errors, backspaces,
             duration_s, speed_cpm, accuracy,
             keystrokes, code_length)
        VALUES
            (:timestamp, :doc_name, :doc_source,
             :char_count, :correct, :errors, :backspaces,
             :duration_s, :speed_cpm, :accuracy,
             :keystrokes, :code_length)
    )");

    q.bindValue(":timestamp",
                e.timestamp.isValid()
                    ? e.timestamp.toString(Qt::ISODate)
                    : QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":doc_name", e.docName);
    q.bindValue(":doc_source", e.docSource);
    q.bindValue(":char_count", e.charCount);
    q.bindValue(":correct", e.correct);
    q.bindValue(":errors", e.errors);
    q.bindValue(":backspaces", e.backspaces);
    q.bindValue(":duration_s", e.durationSeconds);
    q.bindValue(":speed_cpm", e.speedCPM);
    q.bindValue(":accuracy", e.accuracy);
    q.bindValue(":keystrokes", e.keystrokes);
    q.bindValue(":code_length", e.codeLength);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        qWarning() << "HistoryDb 插入失败:" << m_lastError;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------
// 查询
// ---------------------------------------------------------------
QVector<HistoryEntry> HistoryDb::query(int limit)
{
    QVector<HistoryEntry> result;
    if (!m_ready) return result;

    auto db = QSqlDatabase::database("history");
    QSqlQuery q(db);

    QString sql = "SELECT id, timestamp, doc_name, doc_source, "
                  "char_count, correct, errors, backspaces, "
                  "duration_s, speed_cpm, accuracy, keystrokes, code_length "
                  "FROM history ORDER BY timestamp DESC";
    if (limit > 0) sql += QString(" LIMIT %1").arg(limit);

    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        return result;
    }

    while (q.next()) {
        HistoryEntry e;
        e.id              = q.value(0).toInt();
        e.timestamp       = QDateTime::fromString(q.value(1).toString(),
                                                  Qt::ISODate);
        e.docName         = q.value(2).toString();
        e.docSource       = q.value(3).toString();
        e.charCount       = q.value(4).toInt();
        e.correct         = q.value(5).toInt();
        e.errors          = q.value(6).toInt();
        e.backspaces      = q.value(7).toInt();
        e.durationSeconds = q.value(8).toDouble();
        e.speedCPM        = q.value(9).toDouble();
        e.accuracy        = q.value(10).toDouble();
        e.keystrokes      = q.value(11).toInt();
        e.codeLength      = q.value(12).toDouble();
        result.append(e);
    }
    return result;
}

QVector<HistoryEntry> HistoryDb::queryByDoc(const QString& docName, int limit)
{
    QVector<HistoryEntry> result;
    if (!m_ready) return result;

    auto db = QSqlDatabase::database("history");
    QSqlQuery q(db);

    QString sql = "SELECT id, timestamp, doc_name, doc_source, "
                  "char_count, correct, errors, backspaces, "
                  "duration_s, speed_cpm, accuracy, keystrokes, code_length "
                  "FROM history WHERE doc_name = :name "
                  "ORDER BY timestamp DESC";
    if (limit > 0) sql += QString(" LIMIT %1").arg(limit);

    q.prepare(sql);
    q.bindValue(":name", docName);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return result;
    }

    while (q.next()) {
        HistoryEntry e;
        e.id              = q.value(0).toInt();
        e.timestamp       = QDateTime::fromString(q.value(1).toString(),
                                                  Qt::ISODate);
        e.docName         = q.value(2).toString();
        e.docSource       = q.value(3).toString();
        e.charCount       = q.value(4).toInt();
        e.correct         = q.value(5).toInt();
        e.errors          = q.value(6).toInt();
        e.backspaces      = q.value(7).toInt();
        e.durationSeconds = q.value(8).toDouble();
        e.speedCPM        = q.value(9).toDouble();
        e.accuracy        = q.value(10).toDouble();
        e.keystrokes      = q.value(11).toInt();
        e.codeLength      = q.value(12).toDouble();
        result.append(e);
    }
    return result;
}

// ---------------------------------------------------------------
// 删除
// ---------------------------------------------------------------
bool HistoryDb::clearAll()
{
    if (!m_ready) return false;

    auto db = QSqlDatabase::database("history");
    QSqlQuery q(db);
    if (!q.exec("DELETE FROM history")) {
        m_lastError = q.lastError().text();
        return false;
    }
    // 重置自增 ID（可选）
    q.exec("DELETE FROM sqlite_sequence WHERE name='history'");
    return true;
}

bool HistoryDb::remove(int id)
{
    if (!m_ready) return false;

    auto db = QSqlDatabase::database("history");
    QSqlQuery q(db);
    q.prepare("DELETE FROM history WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

int HistoryDb::count()
{
    if (!m_ready) return 0;

    auto db = QSqlDatabase::database("history");
    QSqlQuery q(db);
    if (!q.exec("SELECT COUNT(*) FROM history") || !q.next())
        return 0;
    return q.value(0).toInt();
}