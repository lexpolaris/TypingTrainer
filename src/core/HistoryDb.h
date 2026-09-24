// src/core/HistoryDb.h
#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>

struct HistoryEntry
{
    int     id = -1;
    QDateTime timestamp;
    QString docName;
    QString docSource;
    int     charCount = 0;
    int     correct = 0;
    int     errors = 0;
    int     backspaces = 0;
    double  durationSeconds = 0;
    double  speedCPM = 0;
    double  accuracy = 0;
    int     keystrokes = 0;
    double  codeLength = 0;
};

/// 历史成绩数据库（SQLite）
///
/// - 单例
/// - 首次访问时自动打开并建表
/// - 路径：AppPaths::historyDbPath()
class HistoryDb
{
public:
    static HistoryDb& instance();

    /// 是否可用（打开成功且表就绪）
    bool isReady() const { return m_ready; }
    QString lastError() const { return m_lastError; }

    /// 追加一条记录
    bool add(const HistoryEntry& entry);

    /// 查询（按时间倒序）
    /// @param limit 最多返回多少条，0 表示不限制
    QVector<HistoryEntry> query(int limit = 0);

    /// 按文件名过滤
    QVector<HistoryEntry> queryByDoc(const QString& docName, int limit = 0);

    /// 全部删除
    bool clearAll();

    /// 删除一条
    bool remove(int id);

    /// 总数
    int count();

private:
    HistoryDb();
    ~HistoryDb();
    Q_DISABLE_COPY(HistoryDb)

    bool ensureTable();

    bool    m_ready = false;
    QString m_lastError;
};