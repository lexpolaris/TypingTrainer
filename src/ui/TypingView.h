// src/ui/TypingView.h
#pragma once

#include <QWidget>
#include <QFont>

class TypingSession;
class TextDocument;
class CodeTable;

class TypingView : public QWidget
{
    Q_OBJECT
public:
    explicit TypingView(QWidget* parent = nullptr);

    virtual void setSession(TypingSession* s);
    virtual void setDocument(TextDocument* d);
    virtual void setCodeTable(CodeTable* t);

    virtual QString modeName() const = 0;

    void setTypingFont(const QFont& f);
    QFont typingFont() const { return m_font; }

public slots:
    virtual void onPositionChanged(int index, bool correct)
    {
        Q_UNUSED(index);
        Q_UNUSED(correct);
        update();
    }

signals:
    void codeHintRequested(QChar current, QChar next);

protected:
    // ---------------- 键盘输入 ----------------
    void keyPressEvent(QKeyEvent* e) override;
    void inputMethodEvent(QInputMethodEvent* e) override;

    // ---------------- 焦点管理 ----------------
    void mousePressEvent(QMouseEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;

    // ---------------- 数据 ----------------
    TypingSession* m_session = nullptr;
    TextDocument*  m_doc = nullptr;
    CodeTable*     m_codeTable = nullptr;
    QFont          m_font;

private:
    void handleKeyInput(QChar ch);
};