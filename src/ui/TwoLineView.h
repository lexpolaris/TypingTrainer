// src/ui/TwoLineView.h
#pragma once
#include "TypingView.h"

class TwoLineView : public TypingView
{
    Q_OBJECT
public:
    explicit TwoLineView(QWidget* parent = nullptr);
    QString modeName() const override { return tr("双行对照模式"); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    int m_marginX = 24;
    int m_marginY = 24;
    int m_lineGap = 12;
};
