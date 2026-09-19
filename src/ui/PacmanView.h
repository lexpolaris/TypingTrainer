// src/ui/PacmanView.h
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

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    struct LineLayout { int startIndex; int length; int y; };
    QVector<LineLayout> m_lines;
    int m_marginX = 24;
    int m_marginY = 24;
    int m_lineSpacing = 8;

    void relayout();
    void drawPacman(QPainter& p, const QRectF& cell);
};
