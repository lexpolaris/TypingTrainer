// src/ui/PacmanView.h
#pragma once

#include "WrappedTextView.h"

#include <QVector>

/// 吃豆人模式：打对一个字，该字立即变灰（被"吃掉"）
///   - 已打对：灰色
///   - 当前字：高亮背景 + 反色文字 + 下划线
///   - 未打：正常色
///   - 打错：标红（可关闭）
class PacmanView : public WrappedTextView
{
    Q_OBJECT
public:
    explicit PacmanView(QWidget* parent = nullptr);
    ~PacmanView() override = default;

    QString modeName() const override { return tr("吃豆人模式"); }

    void onPositionChanged(int index, bool correct) override;

    /// 是否将错误字符标红
    void setMarkErrors(bool on) { m_markErrors = on; update(); }
    bool markErrors() const { return m_markErrors; }

protected:
    int visualLineHeight(const QFontMetrics& fm) const override
    {
        // 单行高度 + 行间距
        return fm.height() + m_lineSpacing;
    }

    void drawVisualLine(QPainter& p,
                        const VisualLine& vl,
                        int baseY,
                        int idx,
                        const QString& text,
                        const QFontMetrics& fm) override;

private:
    void markErrorAt(int index);

    QVector<int> m_errorIndices;   // 已打错的字符索引（用于标红）
    bool m_markErrors = true;      // 是否标红错误字符
};