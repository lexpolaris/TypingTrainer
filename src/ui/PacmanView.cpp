// src/ui/PacmanView.cpp
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
}

void PacmanView::onPositionChanged(int index, bool correct)
{
    Q_UNUSED(index); Q_UNUSED(correct);
    relayout();
    update();
}

void PacmanView::relayout()
{
    m_lines.clear();
    if (!m_doc) return;

    QFontMetrics fm(m_font);
    int availWidth = width() - 2 * m_marginX;
    int lineHeight = fm.height() + m_lineSpacing;
    int y = m_marginY;

    const QString& text = m_doc->text();
    int i = 0;
    while (i < text.length()) {
        int lineStart = i;
        int w = 0;
        while (i < text.length() && w + fm.horizontalAdvance(text.at(i)) <= availWidth) {
            w += fm.horizontalAdvance(text.at(i));
            ++i;
        }
        if (i == lineStart) ++i;  // 防止死循环
        m_lines.append({lineStart, i - lineStart, y});
        y += lineHeight;
    }
}

void PacmanView::paintEvent(QPaintEvent*)
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

    for (const auto& line : m_lines) {
        int x = m_marginX;
        for (int i = 0; i < line.length; ++i) {
            int gi = line.startIndex + i;
            QChar ch = text.at(gi);
            int w = fm.horizontalAdvance(ch);
            QRectF cell(x, line.y, w, fm.height());

            QColor color;
            if (gi < idx)       color = th.color(ThemeManager::Typed);
            else if (gi == idx) color = th.color(ThemeManager::Current);
            else                color = th.color(ThemeManager::Pending);

            p.setPen(color);
            p.drawText(cell, Qt::AlignCenter, QString(ch));

            if (gi == idx) drawPacman(p, cell);
            x += w;
        }
    }
}

void PacmanView::drawPacman(QPainter& p, const QRectF& cell)
{
    auto& th = ThemeManager::instance();
    p.setBrush(th.color(ThemeManager::Pacman));
    p.setPen(Qt::NoPen);
    double r = qMin(cell.width(), cell.height()) / 2.0;
    QRectF pie(cell.center().x() - r, cell.center().y() - r, 2*r, 2*r);
    p.drawPie(pie, 30 * 16, 300 * 16);  // 张口 60 度
}
