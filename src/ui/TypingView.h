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

    // 当前光标的控件内矩形（未映射到全局）；默认无效
    virtual QRect currentCursorRect() const { return {}; }

public slots:
    virtual void onPositionChanged(int index, bool correct)
    {
        Q_UNUSED(index);
        Q_UNUSED(correct);
        update();
    }

    /// 文本或字体变化时由子类重排；默认仅重绘
    virtual void invalidateLayout() { update(); }

signals:
    void codeHintRequested(QChar current, QChar next);
    void codeHintCleared();   // 打对时清除提示
    void requestRetry();           // F3

protected:
    // ---------------- 键盘输入 ----------------
    void keyPressEvent(QKeyEvent* e) override;
    void inputMethodEvent(QInputMethodEvent* e) override;

    // ---------------- 输入法光标位置 ----------------
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

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