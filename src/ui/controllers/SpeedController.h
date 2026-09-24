// src/ui/controllers/SpeedController.h
#pragma once

#include <QObject>
#include <QString>

class QWidget;
class TypingSession;
class TextDocument;

/// 测速控制器
///
/// 职责：测速点设置对话框、测速结果图表对话框。
class SpeedController : public QObject
{
    Q_OBJECT
public:
    explicit SpeedController(QObject* parent = nullptr);

public slots:
    /// 设置测速点（依赖当前文档与会话）；parent 作为对话框父窗口
    void openSpeedPointSettings(QWidget* parent,
                                TextDocument* doc,
                                TypingSession* session);

    /// 查看测速结果（依赖当前会话与文档）；parent 作为对话框父窗口
    void openSpeedChart(QWidget* parent,
                        TextDocument* doc,
                        TypingSession* session);

signals:
    /// 图表中请求重打某段文本
    void retrySegmentRequested(const QString& segmentText);

    /// 状态栏提示
    void statusMessage(const QString& message, int timeoutMs);

    /// 信息提示（未加载文本 / 尚未开始等）
    void infoMessage(const QString& title, const QString& message);
};