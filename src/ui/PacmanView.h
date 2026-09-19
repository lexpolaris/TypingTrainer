#pragma once

#include "TypingView.h"
#include <QVector>

class PacmanView : public TypingView
{
    Q_OBJECT
public:
    explicit PacmanView(QWidget* parent = nullptr);
    QString modeName() const override { return tr("吃豆人模式"); }

    void onPositionChanged(int index, bool correct) override;

    /// 是否将错误字符标红
    void setMarkErrors(bool on) { m_markErrors = on; update(); }
    bool markErrors() const { return m_markErrors; }

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    struct VisualLine
    {
        int startIndex = 0;
        int length = 0;
        int y = 0;
    };

    void relayout();
    void ensureCursorVisible(int currentIndex);
    void markErrorAt(int index);   // 记录错误位置

    QVector<VisualLine> m_lines;
    QVector<int>        m_errorIndices;   // 已打错的字符索引（用于标红）

    int m_marginX = 24;
    int m_marginY = 24;
    int m_lineSpacing = 8;
    int m_scrollOffset = 0;

    bool m_markErrors = true;

    int m_cachedWidth = -1;
    QFont m_cachedFont;
};