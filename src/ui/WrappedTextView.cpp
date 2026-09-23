// src/ui/WrappedTextView.cpp
#include "WrappedTextView.h"

#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "theme/ThemeManager.h"

#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QtMath>

WrappedTextView::WrappedTextView(QWidget* parent) : TypingView(parent) {}

// ---------------------------------------------------------------
// 软折行布局
//   - 贪心填充：尽可能多塞字符直到超过可用宽度
//   - 原文中的 '\n' 视为硬换行，占用一个空视觉行高度
// ---------------------------------------------------------------
void WrappedTextView::relayout()
{
    m_lines.clear();
    if (!m_doc) return;

    const int availWidth = width() - 2 * m_marginX;
    if (availWidth <= 0) return;

    QFontMetrics fm(m_font);
    const int lineHeight = visualLineHeight(fm);

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

        // 贪心填充
        while (i < text.length() && text.at(i) != '\n') {
            int cw = fm.horizontalAdvance(text.at(i));
            if (w + cw > availWidth && i > lineStart) {
                break;  // 本行已满，软折行
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

void WrappedTextView::invalidateLayout()
{
    m_cachedWidth = -1;
    m_cachedFont = QFont();
    update();
}

// ---------------------------------------------------------------
// 滚动：确保当前字符所在视觉行可见
// ---------------------------------------------------------------
void WrappedTextView::ensureCursorVisible(int currentIndex)
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
    // 光标落在硬换行符上：定位到其后第一个视觉行（或最后一行）
    if (targetLine < 0) {
        for (int i = 0; i < m_lines.size(); ++i) {
            if (m_lines[i].startIndex >= currentIndex) {
                targetLine = i;
                break;
            }
        }
        if (targetLine < 0) targetLine = m_lines.size() - 1;
    }
    if (targetLine < 0) return;

    QFontMetrics fm(m_font);
    const int lineHeight = visualLineHeight(fm);
    const int contentTop = m_lines[targetLine].y;
    const int contentBottom = contentTop + lineHeight;

    // 上下各预留一个 marginY
    const int topPad = m_marginY;
    const int bottomPad = m_marginY;

    // 下方提前两行滚动：光标行 + 2 行缓冲始终可见
    const int lookAhead = lineHeight * 3;

    // 已经打过的行数（相对起点），用于控制"刚开打不滚动"
    const int cursorLineIndex = targetLine;
    const int kStartScrollAfter = 2;   // 打满 2 行后才启用提前滚动

    const int viewTop = m_scrollOffset + topPad;
    const int viewBottom = m_scrollOffset + height() - bottomPad;

    if (contentTop < viewTop) {
        // 向上滚动
        m_scrollOffset = contentTop - topPad;
    } else if (cursorLineIndex >= kStartScrollAfter
               && contentBottom + lookAhead > viewBottom) {
        // 当前行 + 下方两行缓冲超出视口底部 → 提前滚动
        m_scrollOffset = contentBottom + lookAhead - height() + bottomPad;
    }

    const int maxOffset = qMax(0, totalContentHeight() - height() + bottomPad);
    if (m_scrollOffset < 0) m_scrollOffset = 0;
    if (m_scrollOffset > maxOffset) m_scrollOffset = maxOffset;
}

int WrappedTextView::totalContentHeight() const
{
    if (m_lines.isEmpty()) return 0;
    QFontMetrics fm(m_font);
    return m_lines.last().y + visualLineHeight(fm) + m_marginY;
}

QRect WrappedTextView::currentCursorRect() const
{
    if (!m_session || !m_doc || m_lines.isEmpty())
        return {};

    const int idx = m_session->currentIndex();
    QFontMetrics fm(m_font);
    const int lineH = visualLineHeight(fm);

    // 找到包含 idx 的视觉行
    int lineIdx = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& vl = m_lines[i];
        if (idx >= vl.startIndex && idx < vl.startIndex + vl.length) {
            lineIdx = i;
            break;
        }
    }
    if (lineIdx < 0) lineIdx = m_lines.size() - 1;

    const auto& vl = m_lines[lineIdx];
    // 计算本行内光标 x 偏移
    const QString& text = m_doc->text();
    int x = m_marginX;
    for (int k = 0; k < vl.length; ++k) {
        const int gi = vl.startIndex + k;
        if (gi >= idx) break;
        x += fm.horizontalAdvance(text.at(gi));
    }
    const int y = vl.y - m_scrollOffset + m_marginY;
    const int cw = (idx < text.length())
                       ? fm.horizontalAdvance(text.at(idx))
                       : fm.horizontalAdvance(QChar(' '));
    return QRect(x, y, qMax(1, cw), lineH);
}
// ---------------------------------------------------------------
// 事件
// ---------------------------------------------------------------
void WrappedTextView::resizeEvent(QResizeEvent* e)
{
    TypingView::resizeEvent(e);
    relayout();
    update();
}

void WrappedTextView::onPositionChanged(int index, bool correct)
{
    Q_UNUSED(correct);
    ensureCursorVisible(index);
    update();
}

// ---------------------------------------------------------------
// 绘制主循环
// ---------------------------------------------------------------
void WrappedTextView::paintEvent(QPaintEvent*)
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
    const int lineHeight = visualLineHeight(fm);

    // 只绘制视口内的视觉行（性能优化）
    for (int li = 0; li < m_lines.size(); ++li) {
        const VisualLine& vl = m_lines[li];
        const int baseY = vl.y - m_scrollOffset;
        if (baseY + lineHeight < 0) continue;   // 完全在视口上方
        if (baseY > height()) break;            // 完全在视口下方

        drawVisualLine(p, vl, baseY, idx, text, fm);
    }

    // 简易滚动条
    if (m_lines.size() > 1) {
        const int totalHeight = totalContentHeight();
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