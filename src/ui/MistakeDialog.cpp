// src/ui/MistakeDialog.cpp
#include "MistakeDialog.h"

#include "theme/ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <algorithm>

MistakeDialog::MistakeDialog(const QVector<MistakeRecord>& mistakes,
                             const QString& targetText,
                             QWidget* parent)
    : QDialog(parent)
    , m_mistakes(mistakes)
    , m_targetText(targetText)
{
    setWindowTitle(tr("错字列表"));
    resize(640, 520);

    // 按位置排序
    std::sort(m_mistakes.begin(), m_mistakes.end(),
              [](const MistakeRecord& a, const MistakeRecord& b) {
                  return a.position < b.position;
              });

    setupUi();
    fillTable();
}

void MistakeDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // 顶部：汇总
    m_summaryLabel = new QLabel(this);
    QFont sf = m_summaryLabel->font();
    sf.setPointSize(sf.pointSize() + 1);
    sf.setBold(true);
    m_summaryLabel->setFont(sf);
    root->addWidget(m_summaryLabel);

    // 表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        tr("序"), tr("位置"), tr("期望字"), tr("输入"), tr("次数"), tr("上下文")
    });
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    root->addWidget(m_table, 1);

    // 预览
    m_previewLabel = new QLabel(this);
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setMinimumHeight(60);
    QFont pf = m_previewLabel->font();
    pf.setPointSize(pf.pointSize() + 2);
    m_previewLabel->setFont(pf);
    m_previewLabel->setStyleSheet(
        "padding: 8px; border: 1px solid palette(mid);");
    root->addWidget(m_previewLabel);

    // 底部按钮
    auto* bottom = new QHBoxLayout();
    bottom->addStretch();
    m_jumpBtn = new QPushButton(tr("跳转"), this);
    m_jumpBtn->setEnabled(false);
    auto* closeBtn = new QPushButton(tr("关闭"), this);
    bottom->addWidget(m_jumpBtn);
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);

    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &MistakeDialog::onItemDoubleClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        int row = m_table->currentRow();
        if (row >= 0) {
            m_jumpBtn->setEnabled(true);
            updatePreview(row);
        }
    });
    connect(m_jumpBtn, &QPushButton::clicked,
            this, &MistakeDialog::onJumpClicked);
    connect(closeBtn, &QPushButton::clicked,
            this, &QDialog::accept);
}

void MistakeDialog::fillTable()
{
    auto& th = ThemeManager::instance();

    m_table->setRowCount(m_mistakes.size());

    int totalErrors = 0;
    for (const auto& m : m_mistakes)
        totalErrors += m.count;

    m_summaryLabel->setText(
        tr("共 %1 个错字位置，累计 %2 次错误")
            .arg(m_mistakes.size()).arg(totalErrors));

    for (int i = 0; i < m_mistakes.size(); ++i) {
        const auto& m = m_mistakes[i];

        auto makeItem = [](const QString& text,
                           Qt::Alignment align = Qt::AlignCenter) {
            auto* it = new QTableWidgetItem(text);
            it->setTextAlignment(align | Qt::AlignVCenter);
            return it;
        };

        m_table->setItem(i, 0, makeItem(QString::number(i + 1)));
        m_table->setItem(i, 1, makeItem(QString::number(m.position)));
        m_table->setItem(i, 2, makeItem(QString(m.expected)));
        m_table->setItem(i, 3, makeItem(QString(m.actual)));
        m_table->setItem(i, 4, makeItem(QString::number(m.count)));
        m_table->setItem(i, 5, makeItem(contextAround(m.position),
                                        Qt::AlignLeft | Qt::AlignVCenter));

        // 次数高的标红
        if (m.count >= 3) {
            for (int c = 0; c < 6; ++c) {
                if (auto* it = m_table->item(i, c)) {
                    it->setBackground(th.color(ThemeManager::Error));
                    it->setForeground(th.color(ThemeManager::AccentText));
                }
            }
        }
    }

    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setStretchLastSection(true);
}

void MistakeDialog::updatePreview(int row)
{
    if (row < 0 || row >= m_mistakes.size()) return;
    const auto& m = m_mistakes[row];

    // 高亮显示上下文
    QString ctx = contextAround(m.position, 12);
    QString html = QString(
        "<span style='color:%1'>%2</span>"
        "<span style='background:#FFD54F; color:#000; font-weight:bold;'>%3</span>"
        "<span style='color:%1'>%4</span>")
        .arg("#888")
        .arg(m_targetText.mid(qMax(0, m.position - 12),
                              m.position - qMax(0, m.position - 12)).toHtmlEscaped())
        .arg(QString(m.expected).toHtmlEscaped())
        .arg(m_targetText.mid(m.position + 1, 12).toHtmlEscaped());

    m_previewLabel->setText(html);
}

QString MistakeDialog::contextAround(int pos, int radius) const
{
    int start = qMax(0, pos - radius);
    int end = qMin(m_targetText.length(), pos + radius + 1);
    QString ctx = m_targetText.mid(start, end - start);
    ctx.replace('\n', QChar(0x21B5));  // ↵
    return ctx;
}

void MistakeDialog::onItemDoubleClicked(int row, int)
{
    if (row < 0 || row >= m_mistakes.size()) return;
    m_jumpPosition = m_mistakes[row].position;
    emit jumpRequested(m_jumpPosition);
    accept();
}

void MistakeDialog::onJumpClicked()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_mistakes.size()) return;
    m_jumpPosition = m_mistakes[row].position;
    emit jumpRequested(m_jumpPosition);
    accept();
}