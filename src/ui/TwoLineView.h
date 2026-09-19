// src/ui/TwoLineView.h
#pragma once

#include "WrappedTextView.h"

/// 双行对照模式：
///   - 上行：原文（已打变灰，当前高亮，未打正常）
///   - 下行：用户输入（已打显示原字符，当前高亮块，未打空白）
class TwoLineView : public WrappedTextView
{
    Q_OBJECT
public:
    explicit TwoLineView(QWidget* parent = nullptr);
    ~TwoLineView() override = default;

    QString modeName() const override { return tr("双行对照模式"); }

protected:
    int visualLineHeight(const QFontMetrics& fm) const override
    {
        // 上行 + 下行 + 行间距
        return fm.height() * 2 + m_lineGap + m_lineSpacing;
    }

    void drawVisualLine(QPainter& p,
                        const VisualLine& vl,
                        int baseY,
                        int idx,
                        const QString& text,
                        const QFontMetrics& fm) override;

private:
    int m_lineGap = 12;   // 上行与下行的间距
};