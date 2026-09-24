// src/ui/controllers/HistoryController.h
#pragma once

#include <QObject>
#include <QString>

class TypingSession;

/// 历史成绩控制器
///
/// 职责：把一次会话的统计快照保存到历史数据库。
class HistoryController : public QObject
{
    Q_OBJECT
public:
    explicit HistoryController(QObject* parent = nullptr);

    /// 保存一条历史记录
    /// @param session   会话（提供统计）
    /// @param docName   文档显示名
    /// @param docSource 文档来源键（可为空）
    void save(const TypingSession* session,
              const QString& docName,
              const QString& docSource);
};