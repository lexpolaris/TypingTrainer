// src/ui/WrappedTextView.h
#pragma once

#include "TypingView.h"

#include <QFontMetrics>
#include <QVector>

/// 折行文本视图基类
///
/// 负责：
///   - 软折行布局（按控件宽度贪心填充，遇 '\n' 硬换行）
///   - 光标可见性滚动
///   - 视口裁剪绘制
///   - 简易滚动条
///
/// 子类需实现：
///   - visualLineHeight(fm)     每视觉行占用的像素高度
///   - drawVisualLine(...)      如何绘制一个视觉行
///
/// 可选重写：
///   - onPositionChanged(...)   处理位置变化副作用
class WrappedTextView : public TypingView
{
    Q_OBJECT
public:
    explicit WrappedTextView(QWidget* parent = nullptr);
    ~WrappedTextView() override = default;

    void onPositionChanged(int index, bool correct) override;

protected:
    // ---------------- 视觉行数据结构 ----------------
    struct VisualLine
    {
        int startIndex = 0;   // 在全文中的起始字符索引
        int length = 0;       // 本行字符数
        int y = 0;            // 本行顶部 y 坐标（相对于内容坐标系）
    };

    // ---------------- 子类必须实现的钩子 ----------------

    /// 每个视觉行占用的像素高度
    /// 注意：此函数必须只依赖 fm 与成员配置，不依赖滚动状态，
    ///       否则缓存会失效导致布局错乱。
    virtual int visualLineHeight(const QFontMetrics& fm) const = 0;

    /// 绘制单个视觉行
    /// @param p        画笔
    /// @param vl       视觉行（startIndex / length / y 为内容坐标）
    /// @param baseY    该视觉行在控件坐标中的顶部 y（已减 scrollOffset）
    /// @param idx      当前光标所在字符的全局索引
    /// @param text     原文
    /// @param fm       字体度量
    virtual void drawVisualLine(QPainter& p,
                                const VisualLine& vl,
                                int baseY,
                                int idx,
                                const QString& text,
                                const QFontMetrics& fm) = 0;

    // ---------------- 供子类使用的工具 ----------------

    /// 强制重新布局（子类修改 margin / spacing 后调用）
    void invalidateLayout() override;

    /// 请求滚动到指定字符，使其可见
    void ensureCursorVisible(int currentIndex);

    /// 访问所有视觉行
    const QVector<VisualLine>& lines() const { return m_lines; }

    /// 由视觉行布局计算当前光标的控件内矩形
    QRect currentCursorRect() const override;

    /// 当前滚动偏移（像素）
    int scrollOffset() const { return m_scrollOffset; }

    /// 内容总高度（用于滚动条）
    int totalContentHeight() const;

    // ---------------- 布局参数（子类可改） ----------------
    int m_marginX = 24;       // 左右边距
    int m_marginY = 24;       // 顶边距
    int m_lineSpacing = 8;    // 视觉行之间的额外间距

    // ---------------- Qt 事件 ----------------
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void relayout();

    QVector<VisualLine> m_lines;
    int m_scrollOffset = 0;

    int   m_cachedWidth = -1;
    QFont m_cachedFont;
};