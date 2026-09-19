#include "SpeedPointFinder.h"

#include <QRegularExpression>
#include <QtMath>

QVector<SpeedPointCandidate> SpeedPointFinder::find(
    const QString& text,
    const SpeedPointFinderConfig& cfg)
{
    QVector<SpeedPointCandidate> result;
    if (text.isEmpty()) return result;

    // 1. 查找所有标记
    QRegularExpression re(cfg.marker);
    QRegularExpressionMatchIterator it = re.globalMatch(text);

    QVector<int> usedIndices;
    int lastAccepted = -cfg.minDistance;

    while (it.hasNext()) {
        auto m = it.next();
        int markerPos = m.capturedStart();

        // 测速点位置 = 标记前 N 字
        int prefixStart = qMax(0, markerPos - cfg.prefixLength);
        int prefixLen = markerPos - prefixStart;
        if (prefixLen <= 0) continue;

        // 提取前缀文本
        QString prefix = text.mid(prefixStart, prefixLen);

        // 判断是否命中分类词
        bool autoChecked = false;
        for (const QString& kw : cfg.categoryKeywords) {
            if (prefix.contains(kw)) {
                autoChecked = true;
                break;
            }
        }

        // 去重：与上一个接受的位置至少间隔 minDistance
        if (prefixStart - lastAccepted < cfg.minDistance)
            continue;

        SpeedPointCandidate c;
        c.index = prefixStart;
        c.prefix = prefix;
        c.autoChecked = autoChecked;
        result.append(c);
        usedIndices.append(prefixStart);
        lastAccepted = prefixStart;

        // 数量限制
        if (result.size() >= cfg.maxPoints) break;
    }

    // 2. 均衡补充
    if (cfg.fillEvenly && result.size() < cfg.maxPoints) {
        int need = cfg.maxPoints - result.size();
        QVector<int> extra = fillEvenly(text.length(), need, usedIndices,
                                        cfg.minDistance);
        for (int idx : extra) {
            SpeedPointCandidate c;
            c.index = idx;
            c.prefix = text.mid(qMax(0, idx - cfg.prefixLength),
                                qMin(cfg.prefixLength, idx));
            c.autoChecked = false;
            result.append(c);
        }
    }

    // 3. 按位置排序
    std::sort(result.begin(), result.end(),
              [](const SpeedPointCandidate& a, const SpeedPointCandidate& b) {
        return a.index < b.index;
    });

    return result;
}

QVector<int> SpeedPointFinder::fillEvenly(
    int textLength, int count,
    const QVector<int>& existing,
    int minDistance)
{
    QVector<int> result;
    if (textLength <= 0 || count <= 0) return result;

    // 把全文按 count+1 等分，取每个分割点
    for (int i = 1; i <= count; ++i) {
        int pos = textLength * i / (count + 1);
        pos = qBound(1, pos, textLength - 1);

        // 检查是否与已有的太近
        bool tooClose = false;
        for (int e : existing) {
            if (qAbs(e - pos) < minDistance) { tooClose = true; break; }
        }
        for (int r : result) {
            if (qAbs(r - pos) < minDistance) { tooClose = true; break; }
        }
        if (tooClose) continue;

        result.append(pos);
    }
    return result;
}