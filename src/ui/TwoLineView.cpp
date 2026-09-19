#include "TwoLineView.h"

#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "theme/ThemeManager.h"

#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QtMath>

TwoLineView::TwoLineView(QWidget* parent) : TypingView(parent) {}

void TwoLineView::resizeEvent(QResizeEvent* e)
{
    TypingView::resizeEvent(e);
    relayout();
    update();
}

void TwoLineView::onPositionChanged(int index, bool correct)
{
    Q_UNUSED(correct);
    ensureCursorVisible(index);
    update();
}

// ---------------------------------------------------------------
// 软折行布局算法
// ---------------------------------------------------------------
void TwoLineView::relayout()
{
    m_lines.clear();
    if (!m_doc) return;

    const int availWidth = width() - 2 * m_marginX;
    if (availWidth <= 0) return;

    QFontMetrics fm(m_font);
    const int lineHeight = fm.height() * 2 + m_lineGap + m_lineSpacing;

    const QString& text = m_doc->text();
    int i = 0;
    int y = 0;

    while (i < text.length()) {
        // 跳过行首换行符（原文里的硬换行）
        if (text.at(i) == '\n') {
            ++i;
            y += lineHeight;
            continue;
        }

        int lineStart = i;
        int w = 0;

        // 贪心填充：尽可能多塞字符直到超过可用宽度
        while (i < text.length() && text.at(i) != '\n') {
            int cw = fm.horizontalAdvance(text.at(i));
            if (w + cw > availWidth && i > lineStart) {
                break;  // 本行已满，换行
            }
            w += cw;
            ++i;
        }

        // 防止空行进死循环
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
// 滚动：确保当前字符所在视觉行可见
// ---------------------------------------------------------------
void TwoLineView::ensureCursorVisible(int currentIndex)
{
    if (m_lines.isEmpty()) return;

    // 找到包含 currentIndex 的视觉行
    int targetLine = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& vl = m_lines[i];
        if (currentIndex >= vl.startIndex &&
            currentIndex < vl.startIndex + vl.length) {
            targetLine = i;
            break;
        }
    }
    // 光标在最后一行末尾
    if (targetLine < 0 && !m_lines.isEmpty()) {
        const auto& last = m_lines.last();
        if (currentIndex >= last.startIndex)
            targetLine = m_lines.size() - 1;
    }
    if (targetLine < 0) return;

    QFontMetrics fm(m_font);
    const int lineHeight = fm.height() * 2 + m_lineGap + m_lineSpacing;
    const int contentTop = m_lines[targetLine].y;
    const int contentBottom = contentTop + lineHeight;
    const int viewTop = m_scrollOffset;
    const int viewBottom = m_scrollOffset + height();

    if (contentTop < viewTop) {
        // 向上滚动
        m_scrollOffset = contentTop;
    } else if (contentBottom > viewBottom) {
        // 向下滚动，让本行底部贴合视图底部
        m_scrollOffset = contentBottom - height();
    }
    if (m_scrollOffset < 0) m_scrollOffset = 0;
}

int TwoLineView::lineY(int visualLineIdx) const
{
    if (visualLineIdx < 0 || visualLineIdx >= m_lines.size()) return 0;
    return m_lines[visualLineIdx].y - m_scrollOffset;
}

// ---------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------
void TwoLineView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    auto& th = ThemeManager::instance();
    p.fillRect(rect(), th.color(ThemeManager::WindowBg));

    if (!m_doc || !m_session) return;

    // 惰性重排：字体或宽度变化时重算
    if (m_cachedWidth != width() || m_cachedFont != m_font) {
        relayout();
    }

    p.setFont(m_font);
    QFontMetrics fm(m_font);

    const int idx = m_session->currentIndex();
    const QString& text = m_doc->text();
    const int lineHeight = fm.height() * 2 + m_lineGap + m_lineSpacing;

    // 只绘制视口内的视觉行（性能优化）
    for (int li = 0; li < m_lines.size(); ++li) {
        const VisualLine& vl = m_lines[li];
        int baseY = vl.y - m_scrollOffset;
        if (baseY + lineHeight < 0) continue;          // 视口上方
        if (baseY > height()) break;                   // 视口下方

        const int y1 = baseY + m_marginY;              // 上行（原文）
        const int y2 = y1 + fm.height() + m_lineGap;   // 下行（输入）

        int x = m_marginX;
        for (int k = 0; k < vl.length; ++k) {
            const int gi = vl.startIndex + k;
            QChar ch = text.at(gi);
            int cw = fm.horizontalAdvance(ch);

            // ---- 上行：原文 ----
            QColor color;
            if (gi < idx)       color = th.color(ThemeManager::Typed);
            else if (gi == idx) color = th.color(ThemeManager::Current);
            else                color = th.color(ThemeManager::Pending);
            p.setPen(color);
            p.drawText(x, y1 + fm.ascent(), QString(ch));

            // ---- 下行：输入 ----
            if (gi < idx) {
                // 已打部分：显示原字符（与原文一致）
                p.setPen(th.color(ThemeManager::Typed));
                p.drawText(x, y2 + fm.ascent(), QString(ch));
            } else if (gi == idx) {
                // 当前字符：高亮块 + 反色文字
                QRectF block(x, y2, cw, fm.height());
                p.fillRect(block, th.color(ThemeManager::Current));
                p.setPen(th.color(ThemeManager::AccentText));
                p.drawText(x, y2 + fm.ascent(), QString(ch));

                // 光标下划线（可选）
                QPen pen(th.color(ThemeManager::Accent), 2);
                p.setPen(pen);
                p.drawLine(QPointF(x, y2 + fm.height()),
                           QPointF(x + cw, y2 + fm.height()));
            }
            // 未打部分：下行不画

            x += cw;
        }
    }

    // 绘制滚动条（简易）
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