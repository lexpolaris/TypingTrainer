// src/ui/TwoLineView.cpp
#include "TwoLineView.h"
#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "theme/ThemeManager.h"

#include <QPainter>
#include <QFontMetrics>

TwoLineView::TwoLineView(QWidget* parent) : TypingView(parent) {}

void TwoLineView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    auto& th = ThemeManager::instance();
    p.fillRect(rect(), th.color(ThemeManager::WindowBg));

    if (!m_doc || !m_session) return;

    p.setFont(m_font);
    QFontMetrics fm(m_font);
    int idx = m_session->currentIndex();
    const QString& text = m_doc->text();

    // 可视窗口：以光标为中心
    int visibleChars = qMax(10, (width() - 2 * m_marginX) / fm.horizontalAdvance('M'));
    int start = qMax(0, idx - visibleChars / 3);
    int end = qMin(text.length(), start + visibleChars);

    int y1 = m_marginY;
    int y2 = y1 + fm.height() + m_lineGap;

    int x = m_marginX;
    for (int i = start; i < end; ++i) {
        QChar expected = text.at(i);
        int w = fm.horizontalAdvance(expected);

        // 上行：原文
        QColor color;
        if (i < idx)       color = th.color(ThemeManager::Typed);
        else if (i == idx) color = th.color(ThemeManager::Current);
        else               color = th.color(ThemeManager::Pending);
        p.setPen(color);
        p.drawText(x, y1 + fm.ascent(), QString(expected));

        // 下行：输入
        if (i < idx) {
            p.setPen(th.color(ThemeManager::Typed));
            p.drawText(x, y2 + fm.ascent(), QString(expected));
        } else if (i == idx) {
            p.fillRect(x, y2, w, fm.height(), th.color(ThemeManager::Current));
            p.setPen(th.color(ThemeManager::AccentText));
            p.drawText(x, y2 + fm.ascent(), QString(expected));
        }

        x += w;
    }
}
