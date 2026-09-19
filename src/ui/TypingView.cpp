// src/ui/TypingView.cpp
#include "TypingView.h"

#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "core/CodeTable.h"
#include "theme/ThemeManager.h"

#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QFocusEvent>

TypingView::TypingView(QWidget* parent) : QWidget(parent)
{
    m_font = font();
    m_font.setPointSize(18);
    setAutoFillBackground(true);

    // 关键：强焦点策略，允许点击/快捷键/Tab 获取焦点
    setFocusPolicy(Qt::StrongFocus);
    // 允许输入法（中文用户）
    setAttribute(Qt::WA_InputMethodEnabled, true);

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

// ---------------------------------------------------------------
// 键盘输入
// ---------------------------------------------------------------
void TypingView::keyPressEvent(QKeyEvent* e)
{
    if (!m_session) {
        QWidget::keyPressEvent(e);
        return;
    }

    // Backspace
    if (e->key() == Qt::Key_Backspace) {
        m_session->backspace();
        e->accept();
        return;
    }

    // Esc：暂停 / 恢复
    if (e->key() == Qt::Key_Escape) {
        if (m_session->state() == TypingSession::Running)
            m_session->pause();
        else if (m_session->state() == TypingSession::Paused)
            m_session->resume();
        e->accept();
        return;
    }

    // 忽略纯修饰键
    switch (e->key()) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_ScrollLock:
        QWidget::keyPressEvent(e);
        return;
    default: break;
    }

    // Ctrl/Alt 组合键交回父窗口（保留菜单快捷键）
    if (e->modifiers().testFlag(Qt::ControlModifier) ||
        e->modifiers().testFlag(Qt::AltModifier)) {
        QWidget::keyPressEvent(e);
        return;
    }

    // 可打印字符
    const QString text = e->text();
    if (!text.isEmpty()) {
        for (QChar ch : text) {
            if (ch.isPrint()) {
                handleKeyInput(ch);
            }
        }
        e->accept();
        return;
    }

    QWidget::keyPressEvent(e);
}

void TypingView::inputMethodEvent(QInputMethodEvent* e)
{
    if (!m_session) {
        QWidget::inputMethodEvent(e);
        return;
    }

    // 中文输入法最终提交的字符串
    if (!e->commitString().isEmpty()) {
        for (QChar ch : e->commitString()) {
            if (ch.isPrint()) handleKeyInput(ch);
        }
    }
    // 预编辑文本由输入法自行处理（不送入 session）
    e->accept();
}

void TypingView::handleKeyInput(QChar ch)
{
    if (!m_session) return;
    m_session->inputCharacter(ch);
}

// ---------------------------------------------------------------
// 焦点管理
// ---------------------------------------------------------------
void TypingView::mousePressEvent(QMouseEvent* e)
{
    // 点击视图时重新获取焦点（防止焦点跑到菜单栏后回不来）
    setFocus(Qt::MouseFocusReason);
    QWidget::mousePressEvent(e);
}

void TypingView::focusInEvent(QFocusEvent* e)
{
    QWidget::focusInEvent(e);
    update();  // 让视图知道有焦点（未来可绘制光标闪烁）
}

void TypingView::focusOutEvent(QFocusEvent* e)
{
    QWidget::focusOutEvent(e);
    update();
}