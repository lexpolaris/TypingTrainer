#pragma once

#include "TypingView.h"

#include <QVector>

class TwoLineView : public TypingView
{
    Q_OBJECT
public:
    explicit TwoLineView(QWidget* parent = nullptr);
    QString modeName() const override { return tr("双行对照模式"); }

    void onPositionChanged(int index, bool correct) override;

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    struct VisualLine
    {
        int startIndex = 0;   // 在全文中的起始字符索引
        int length = 0;       // 本行字符数
        int y = 0;            // 本行顶部 y 坐标（相对于内容区）
    };

    // 按当前字体/宽度把文本折成视觉行
    void relayout();

    // 让 currentIndex 所在的视觉行可见
    void ensureCursorVisible(int currentIndex);

    // 根据当前滚动偏移，取第 i 个视觉行的绝对 y（含 -scrollOffset）
    int lineY(int visualLineIdx) const;

    QVector<VisualLine> m_lines;
    int m_marginX = 24;
    int m_marginY = 24;
    int m_lineGap = 12;       // 上行与下行的间距
    int m_lineSpacing = 8;    // 视觉行之间的额外间距
    int m_scrollOffset = 0;   // 内容纵向滚动偏移（像素）

    // 缓存，避免每帧重算
    int m_cachedWidth = -1;
    QFont m_cachedFont;
};