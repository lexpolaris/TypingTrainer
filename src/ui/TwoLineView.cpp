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

    // 拿用户输入记录
    const QHash<int, QChar>& userInput =
        m_session ? m_session->userInput() : QHash<int, QChar>();

    int x = m_marginX;
    for (int k = 0; k < vl.length; ++k) {
        const int gi = vl.startIndex + k;
        const QChar ch = text.at(gi);
        const int cw = fm.horizontalAdvance(ch);

        // =====================================================
        // 上行：原文
        // =====================================================
        // 已打过：不显示（透明）
        // 当前：高亮
        // 未打：正常
        if (gi < idx) {
            // 上行已打过 → 不画
        } else if (gi == idx) {
            p.setPen(th.color(ThemeManager::Current));
            p.drawText(x, y1 + fm.ascent(), QString(ch));
        } else {
            p.setPen(th.color(ThemeManager::Pending));
            p.drawText(x, y1 + fm.ascent(), QString(ch));
        }

        // =====================================================
        // 下行：用户实际输入
        // =====================================================
        if (gi < idx) {
            // 已打过：显示用户输入的字符（黑色，与上行未打一致）
            const QChar userCh = userInput.value(gi, QChar());
            if (!userCh.isNull()) {
                const bool wrong = (userCh != ch);
                p.setPen(wrong ? th.color(ThemeManager::Error)
                               : th.color(ThemeManager::Pending));
                p.drawText(x, y2 + fm.ascent(), QString(userCh));
            }
        } else if (gi == idx) {
            // 当前位置：高亮块 + 用户输入的字符（如有）
            const QChar userCh = userInput.value(gi, QChar());
            const bool hasInput = !userCh.isNull();
            const bool wrong = hasInput && (userCh != ch);

            QRectF block(x, y2, cw, fm.height());
            p.fillRect(block, wrong ? th.color(ThemeManager::Error)
                                    : th.color(ThemeManager::Current));

            p.setPen(th.color(ThemeManager::AccentText));
            p.drawText(x, y2 + fm.ascent(),
                       QString(hasInput ? userCh : ch));

            QPen pen(th.color(ThemeManager::Accent), 2);
            p.setPen(pen);
            p.drawLine(QPointF(x, y2 + fm.height()),
                       QPointF(x + cw, y2 + fm.height()));
        }
        // 未打过：下行不画

        x += cw;
    }
}