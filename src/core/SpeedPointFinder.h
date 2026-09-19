#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct SpeedPointCandidate
{
    int     index;          // 在全文中的字符索引
    QString prefix;         // 标记前 N 字（作为说明）
    bool    autoChecked;    // 是否自动勾选
};

struct SpeedPointFinderConfig
{
    QString marker = QStringLiteral("：");     // 标记字符（支持正则）
    int     prefixLength = 2;                   // 取标记前 N 字
    int     maxPoints = 10;                     // 最多测速点
    int     minDistance = 20;                   // 相邻测速点最小间隔
    QStringList categoryKeywords = {            // 自动勾选的分类词
        QStringLiteral("单字"),
        QStringLiteral("散文"),
        QStringLiteral("小说"),
        QStringLiteral("古文"),
        QStringLiteral("新闻"),
        QStringLiteral("政论"),
        QStringLiteral("名言"),
        QStringLiteral("笑话"),
        QStringLiteral("短信"),
        QStringLiteral("诗词"),
        QStringLiteral("现代文"),
        QStringLiteral("文言文")
    };
    bool    fillEvenly = true;                  // 不足时按等分补充
};

class SpeedPointFinder
{
public:
    /// 自动寻找候选测速点
    static QVector<SpeedPointCandidate> find(const QString& text,
                                             const SpeedPointFinderConfig& cfg);

    /// 按等分补充到指定数量
    static QVector<int> fillEvenly(int textLength, int count,
                                   const QVector<int>& existing,
                                   int minDistance);

private:
    SpeedPointFinder() = delete;
};