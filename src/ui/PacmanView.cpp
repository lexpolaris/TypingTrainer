// src/ui/PacmanView.cpp
#include "PacmanView.h"

#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "theme/ThemeManager.h"

#include <QPainter>
#include <QFontMetrics>

PacmanView::PacmanView(QWidget* parent) : WrappedTextView(parent) {}

// ---------------------------------------------------------------
// 位置变化：记录错误 + 交给基类滚动/重绘
// ---------------------------------------------------------------
void PacmanView::onPositionChanged(int index, bool correct)
{
    if (!correct) {
        // 打错：记录该位置
        markErrorAt(index);
    } else {
        // 同时清理所有 >= index 的标记（回退后重打）
        for (int i = m_errorIndices.size() - 1; i >= 0; --i) {
            if (m_errorIndices[i] >= index)
                m_errorIndices.removeAt(i);
        }
    }

    // 交给基类处理滚动与重绘
    WrappedTextView::onPositionChanged(index, correct);
}

void PacmanView::markErrorAt(int index)
{
    if (index < 0) return;
    if (!m_errorIndices.contains(index))
        m_errorIndices.append(index);
}

// ---------------------------------------------------------------
// 绘制单个视觉行
// ---------------------------------------------------------------
void PacmanView::drawVisualLine(QPainter& p,
                                const VisualLine& vl,
                                int baseY,
                                int idx,
                                const QString& text,
                                const QFontMetrics& fm)
{
    auto& th = ThemeManager::instance();

    int x = m_marginX;
    int y = baseY + m_marginY;

    for (int k = 0; k < vl.length; ++k) {
        const int gi = vl.startIndex + k;
        QChar ch = text.at(gi);
        int cw = fm.horizontalAdvance(ch);

        // ---------- 颜色决策 ----------
        QColor fg;
        QColor bg = Qt::transparent;

        if (gi < idx) {
            // 已打过：变灰（"被吃豆人吃掉"）
            fg = th.color(ThemeManager::Typed);
        } else if (gi == idx) {
            // 当前字：高亮背景 + 反色文字
            fg = th.color(ThemeManager::AccentText);
            bg = th.color(ThemeManager::Current);
        } else {
            // 未打：正常色
            fg = th.color(ThemeManager::Pending);
        }

        // 错误标记：之前在此位置打错过
        if (m_markErrors && m_errorIndices.contains(gi)) {
            fg = th.color(ThemeManager::Error);
            if (gi == idx) {
                // 当前字同时是错的，用错误色作背景
                bg = th.color(ThemeManager::Error);
                fg = th.color(ThemeManager::AccentText);
            }
        }

        // ---------- 绘制背景（当前字高亮） ----------
        if (bg.alpha() > 0) {
            p.fillRect(QRectF(x, y, cw, fm.height()), bg);
        }

        // ---------- 绘制字符 ----------
        p.setPen(fg);
        p.drawText(x, y + fm.ascent(), QString(ch));

        // ---------- 当前字下划线（增强可见性） ----------
        if (gi == idx) {
            QPen pen(th.color(ThemeManager::Accent), 2);
            p.setPen(pen);
            p.drawLine(QPointF(x, y + fm.height() - 1),
                       QPointF(x + cw, y + fm.height() - 1));
        }

        x += cw;
    }
}