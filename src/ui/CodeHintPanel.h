// src/ui/CodeHintPanel.h
#pragma once

#include <QWidget>

class CodeTable;
class QLabel;
class QTimer;

/// 编码提示条（浮动）
///
/// - 打错时在主窗口底部弹出，显示当前字的编码
/// - 2 秒后自动隐藏
/// - 不打字时完全不占空间
class CodeHintPanel : public QWidget
{
    Q_OBJECT
public:
    explicit CodeHintPanel(QWidget* parent = nullptr);

    void setCodeTable(CodeTable* t) { m_table = t; }

    /// 显示提示（打错时调用）
    void showFor(QChar current, QChar next = {});

    /// 立即隐藏
    void clear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void repositionToParent();

    CodeTable* m_table = nullptr;
    QLabel*    m_charLabel = nullptr;
    QLabel*    m_codeLabel = nullptr;
    QLabel*    m_candidateLabel = nullptr;
    QTimer*    m_hideTimer = nullptr;

    QChar m_current;
    bool  m_visible = false;
};