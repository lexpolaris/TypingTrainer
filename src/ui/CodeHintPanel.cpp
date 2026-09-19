// src/ui/CodeHintPanel.cpp
#include "CodeHintPanel.h"
#include "core/CodeTable.h"
#include "theme/ThemeManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>

CodeHintPanel::CodeHintPanel(QWidget* parent) : QWidget(parent)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(8, 4, 8, 4);

    m_charLabel = new QLabel(this);
    QFont f = m_charLabel->font();
    f.setPointSize(f.pointSize() + 6);
    f.setBold(true);
    m_charLabel->setFont(f);
    m_charLabel->setMinimumWidth(40);
    m_charLabel->setAlignment(Qt::AlignCenter);

    m_codeLabel = new QLabel(this);
    m_codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_candidates = new QListWidget(this);
    m_candidates->setFlow(QListView::LeftToRight);
    m_candidates->setMaximumHeight(28);
    m_candidates->setResizeMode(QListView::Adjust);

    root->addWidget(m_charLabel);
    root->addWidget(m_codeLabel, 1);
    root->addWidget(m_candidates, 2);
}

void CodeHintPanel::showFor(QChar current, QChar)
{
    if (!m_table || current.isNull()) { clear(); return; }

    m_charLabel->setText(QString(current));
    QStringList codes = m_table->codesFor(current);
    m_codeLabel->setText(codes.isEmpty() ? tr("（无编码）") : codes.join(" / "));

    m_candidates->clear();
    for (const QString& c : codes) {
        QStringList chars = m_table->charsFor(c);
        m_candidates->addItem(QString("%1: %2").arg(c, chars.join("")));
    }
}

void CodeHintPanel::clear()
{
    m_charLabel->clear();
    m_codeLabel->clear();
    m_candidates->clear();
}
