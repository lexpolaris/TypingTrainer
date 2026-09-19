// src/ui/TypingView.cpp
#include "TypingView.h"
#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "core/CodeTable.h"
#include "theme/ThemeManager.h"

TypingView::TypingView(QWidget* parent) : QWidget(parent)
{
    m_font = font();
    m_font.setPointSize(18);
    setAutoFillBackground(true);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, qOverload<>(&QWidget::update));
}

void TypingView::setSession(TypingSession* s)
{
    if (m_session) m_session->disconnect(this);
    m_session = s;
    if (m_session) {
        connect(m_session, &TypingSession::positionChanged,
                this, &TypingView::onPositionChanged);
        connect(m_session, &TypingSession::positionChanged,
                this, [this](int idx, bool) {
            if (!m_session || !m_doc) return;
            QChar cur = (idx < m_doc->length()) ? m_doc->at(idx) : QChar();
            QChar nxt = (idx + 1 < m_doc->length()) ? m_doc->at(idx + 1) : QChar();
            emit codeHintRequested(cur, nxt);
        });
    }
    update();
}

void TypingView::setDocument(TextDocument* d)
{
    m_doc = d;
    update();
}

void TypingView::setCodeTable(CodeTable* t)
{
    m_codeTable = t;
}

void TypingView::setTypingFont(const QFont& f)
{
    m_font = f;
    update();
}
