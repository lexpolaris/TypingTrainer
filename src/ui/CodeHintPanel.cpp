// src/ui/CodeHintPanel.cpp
#include "CodeHintPanel.h"

#include "core/CodeTable.h"
#include "theme/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>

CodeHintPanel::CodeHintPanel(QWidget* parent) : QWidget(parent)
{
    // 浮动：不参与布局
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setFocusPolicy(Qt::NoFocus);

    // 内容
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(12);

    m_charLabel = new QLabel(this);
    QFont f = m_charLabel->font();
    f.setPointSize(f.pointSize() + 8);
    f.setBold(true);
    m_charLabel->setFont(f);
    m_charLabel->setMinimumWidth(48);
    m_charLabel->setAlignment(Qt::AlignCenter);

    m_codeLabel = new QLabel(this);
    QFont cf = m_codeLabel->font();
    cf.setPointSize(cf.pointSize() + 2);
    m_codeLabel->setFont(cf);
    m_codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_candidateLabel = new QLabel(this);
    m_candidateLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    layout->addWidget(m_charLabel);
    layout->addWidget(m_codeLabel);
    layout->addWidget(m_candidateLabel);

    // 阴影
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(16);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 120));
    setGraphicsEffect(shadow);

    // 自动隐藏
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(2000);
    connect(m_hideTimer, &QTimer::timeout, this, &CodeHintPanel::clear);

    // 主题切换重绘
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, qOverload<>(&QWidget::update));

    hide();
}

void CodeHintPanel::showFor(QChar current, QChar)
{
    if (current.isNull()) { clear(); return; }

    m_current = current;
    m_charLabel->setText(QString(current));

    if (m_table && !m_table->isEmpty()) {
        const QString word = QString(current);
        QStringList codes = m_table->codesFor(word);
        m_codeLabel->setText(codes.isEmpty()
                                 ? tr("（无编码）")
                                 : codes.join(" / "));

        // 候选字：取第一个编码的同码字
        QString candidateText;
        if (!codes.isEmpty()) {
            QStringList words = m_table->wordsFor(codes.first());
            if (words.size() > 1) {
                candidateText = tr("同码: %1").arg(words.join(" "));
            }
        }
        m_candidateLabel->setText(candidateText);
    } else {
        m_codeLabel->setText(tr("（未加载码表）"));
        m_candidateLabel->clear();
    }

    adjustSize();
    repositionToParent();
    show();
    raise();

    m_hideTimer->start();
}

void CodeHintPanel::clear()
{
    m_hideTimer->stop();
    m_current = QChar();
    hide();
}

void CodeHintPanel::repositionToParent()
{
    QWidget* p = parentWidget();
    if (!p) return;
    // 若父窗口隐藏或最小化，直接返回
    if (!p->isVisible()) return;

    // 定位到父窗口底部中央，向上偏移 60px
    const int margin = 24;
    const int bottomOffset = 60;

    int x = p->width() / 2 - width() / 2;
    int y = p->height() - height() - bottomOffset;

    // 转换为全局坐标后移动（因为是顶层窗口）
    QPoint globalPos = p->mapToGlobal(QPoint(x, y));
    move(globalPos);
}

void CodeHintPanel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto& th = ThemeManager::instance();

    // 圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    p.fillPath(path, th.color(ThemeManager::SurfaceBg));
    p.setPen(QPen(th.color(ThemeManager::Border), 1));
    p.drawPath(path);

    // 左边缘强调色
    QRectF accent(0, 0, 4, height());
    QPainterPath accentPath;
    accentPath.addRoundedRect(accent, 2, 2);
    p.fillPath(accentPath, th.color(ThemeManager::Error));
}