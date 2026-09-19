#include "PacmanView.h"

#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "theme/ThemeManager.h"

#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>

PacmanView::PacmanView(QWidget* parent) : TypingView(parent) {}

void PacmanView::resizeEvent(QResizeEvent* e)
{
    TypingView::resizeEvent(e);
    relayout();
    update();
}

void PacmanView::onPositionChanged(int index, bool correct)
{
    // 打错时记录错误位置
    if (!correct) {
        markErrorAt(index);
    } else {
        // 打对推进时，如果该位置之前被标记为错误，清除标记
        m_errorIndices.removeAll(index);
    }

    ensureCursorVisible(index);
    update();
}

void PacmanView::markErrorAt(int index)
{
    if (index < 0) return;
    if (!m_errorIndices.contains(index))
        m_errorIndices.append(index);
}

// ---------------------------------------------------------------
// 软折行布局（与 TwoLineView 一致，但只算一行高度）
// ---------------------------------------------------------------
void PacmanView::relayout()
{
    m_lines.clear();
    if (!m_doc) return;

    const int availWidth = width() - 2 * m_marginX;
    if (availWidth <= 0) return;

    QFontMetrics fm(m_font);
    const int lineHeight = fm.height() + m_lineSpacing;

    const QString& text = m_doc->text();
    int i = 0;
    int y = 0;

    while (i < text.length()) {
        // 原文硬换行
        if (text.at(i) == '\n') {
            ++i;
            y += lineHeight;
            continue;
        }

        int lineStart = i;
        int w = 0;
        while (i < text.length() && text.at(i) != '\n') {
            int cw = fm.horizontalAdvance(text.at(i));
            if (w + cw > availWidth && i > lineStart) break;
            w += cw;
            ++i;
        }
        if (i == lineStart) ++i;

        VisualLine vl;
        vl.startIndex = lineStart;
        vl.length = i - lineStart;
        vl.y = y;
        m_lines.append(vl);

        y += lineHeight;
    }

    m_cachedWidth = width();
    m_cachedFont = m_font;
}

// ---------------------------------------------------------------
// 自动滚动
// ---------------------------------------------------------------
void PacmanView::ensureCursorVisible(int currentIndex)
{
    if (m_lines.isEmpty()) return;

    int targetLine = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& vl = m_lines[i];
        if (currentIndex >= vl.startIndex &&
            currentIndex < vl.startIndex + vl.length) {
            targetLine = i;
            break;
        }
    }
    if (targetLine < 0 && !m_lines.isEmpty()) {
        const auto& last = m_lines.last();
        if (currentIndex >= last.startIndex)
            targetLine = m_lines.size() - 1;
    }
    if (targetLine < 0) return;

    QFontMetrics fm(m_font);
    const int lineHeight = fm.height() + m_lineSpacing;
    const int contentTop = m_lines[targetLine].y;
    const int contentBottom = contentTop + lineHeight;
    const int viewTop = m_scrollOffset;
    const int viewBottom = m_scrollOffset + height();

    if (contentTop < viewTop)
        m_scrollOffset = contentTop;
    else if (contentBottom > viewBottom)
        m_scrollOffset = contentBottom - height();

    if (m_scrollOffset < 0) m_scrollOffset = 0;
}

// ---------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------
void PacmanView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    auto& th = ThemeManager::instance();
    p.fillRect(rect(), th.color(ThemeManager::WindowBg));

    if (!m_doc || !m_session) return;

    if (m_cachedWidth != width() || m_cachedFont != m_font)
        relayout();

    p.setFont(m_font);
    QFontMetrics fm(m_font);
    const int idx = m_session->currentIndex();
    const QString& text = m_doc->text();
    const int lineHeight = fm.height() + m_lineSpacing;

    for (int li = 0; li < m_lines.size(); ++li) {
        const VisualLine& vl = m_lines[li];
        int baseY = vl.y - m_scrollOffset;
        if (baseY + lineHeight < 0) continue;
        if (baseY > height()) break;

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
                // 已经打过的字：变灰（"被吃豆人吃掉"）
                fg = th.color(ThemeManager::Typed);
            } else if (gi == idx) {
                // 当前字：高亮
                fg = th.color(ThemeManager::AccentText);
                bg = th.color(ThemeManager::Current);
            } else {
                // 未打的字：正常
                fg = th.color(ThemeManager::Pending);
            }

            // 错误标记：若之前在这个位置打错过，标红
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

            // ---------- 当前字下划线（可选，增强可见性） ----------
            if (gi == idx) {
                QPen pen(th.color(ThemeManager::Accent), 2);
                p.setPen(pen);
                p.drawLine(QPointF(x, y + fm.height() - 1),
                           QPointF(x + cw, y + fm.height() - 1));
            }

            x += cw;
        }
    }

    // ---------- 滚动条 ----------
    if (m_lines.size() > 1) {
        const int totalHeight = m_lines.last().y + lineHeight;
        if (totalHeight > height()) {
            const int barW = 4;
            const int barX = width() - barW - 2;
            const int trackH = height() - 4;
            const int thumbH = qMax(20, trackH * height() / totalHeight);
            const int thumbY = 2 + (trackH - thumbH) * m_scrollOffset
                                   / qMax(1, totalHeight - height());
            p.fillRect(QRect(barX, 2, barW, trackH),
                       th.color(ThemeManager::Border));
            p.fillRect(QRect(barX, thumbY, barW, thumbH),
                       th.color(ThemeManager::TextSecondary));
        }
    }
}