// src/ui/TwoLineView.cpp
#include "TwoLineView.h"

#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "theme/ThemeManager.h"

#include <QPainter>
#include <QFontMetrics>

TwoLineView::TwoLineView(QWidget* parent) : WrappedTextView(parent) {}

// ---------------------------------------------------------------
// 绘制单个视觉行
//   视觉行内分上下两行：
//     y1 —— 上行（原文）
//     y2 —— 下行（输入）
// ---------------------------------------------------------------
void TwoLineView::drawVisualLine(QPainter& p,
                                 const VisualLine& vl,
                                 int baseY,
                                 int idx,
                                 const QString& text,
                                 const QFontMetrics& fm)
{
    auto& th = ThemeManager::instance();

    const int y1 = baseY + m_marginY;                // 上行顶部
    const int y2 = y1 + fm.height() + m_lineGap;     // 下行顶部

    int x = m_marginX;
    for (int k = 0; k < vl.length; ++k) {
        const int gi = vl.startIndex + k;
        QChar ch = text.at(gi);
        int cw = fm.horizontalAdvance(ch);

        // ---------- 上行：原文 ----------
        QColor upColor;
        if (gi < idx)       upColor = th.color(ThemeManager::Typed);
        else if (gi == idx) upColor = th.color(ThemeManager::Current);
        else                upColor = th.color(ThemeManager::Pending);
        p.setPen(upColor);
        p.drawText(x, y1 + fm.ascent(), QString(ch));

        // ---------- 下行：输入 ----------
        if (gi < idx) {
            // 已打部分：显示原字符（用户输入与原文一致）
            p.setPen(th.color(ThemeManager::Typed));
            p.drawText(x, y2 + fm.ascent(), QString(ch));
        } else if (gi == idx) {
            // 当前字符：高亮块 + 反色文字 + 光标下划线
            QRectF block(x, y2, cw, fm.height());
            p.fillRect(block, th.color(ThemeManager::Current));
            p.setPen(th.color(ThemeManager::AccentText));
            p.drawText(x, y2 + fm.ascent(), QString(ch));

            QPen pen(th.color(ThemeManager::Accent), 2);
            p.setPen(pen);
            p.drawLine(QPointF(x, y2 + fm.height()),
                       QPointF(x + cw, y2 + fm.height()));
        }
        // 未打部分：下行不画

        x += cw;
    }
}