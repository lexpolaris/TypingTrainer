// src/ui/CodeHintPanel.h
#pragma once
#include <QWidget>

class CodeTable;
class QLabel;
class QListWidget;

class CodeHintPanel : public QWidget
{
    Q_OBJECT
public:
    explicit CodeHintPanel(QWidget* parent = nullptr);

    void setCodeTable(CodeTable* t) { m_table = t; }
    void showFor(QChar current, QChar next = {});
    void clear();

private:
    CodeTable* m_table = nullptr;
    QLabel* m_charLabel = nullptr;
    QLabel* m_codeLabel = nullptr;
    QListWidget* m_candidates = nullptr;
};
