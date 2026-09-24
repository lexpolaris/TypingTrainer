// src/ui/HistoryDialog.cpp
#include "HistoryDialog.h"

#include "theme/ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTextStream>
#include <QFile>
#include <algorithm>

HistoryDialog::HistoryDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("历史成绩"));
    resize(900, 560);
    setupUi();
    onRefresh();
}

void HistoryDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // ---- 顶部：搜索 + 操作 ----
    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel(tr("搜索:"), this));
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("按文件名过滤..."));
    m_filterEdit->setClearButtonEnabled(true);
    topRow->addWidget(m_filterEdit, 1);

    m_clearBtn  = new QPushButton(tr("清空全部"), this);
    m_exportBtn = new QPushButton(tr("导出 CSV"), this);
    topRow->addWidget(m_clearBtn);
    topRow->addWidget(m_exportBtn);
    root->addLayout(topRow);

    // ---- 表格 ----
    m_table = new QTableWidget(this);
    m_table->setColumnCount(10);
    m_table->setHorizontalHeaderLabels({
        tr("时间"), tr("文件"), tr("字符"), tr("正确"), tr("错字"),
        tr("回改"), tr("耗时"), tr("速度"), tr("准确率"), tr("码长")
    });
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSortingEnabled(true);   // 允许点击表头排序
    root->addWidget(m_table, 1);

    // ---- 底部：汇总 + 按钮 ----
    auto* bottomRow = new QHBoxLayout();
    m_summaryLabel = new QLabel(this);
    bottomRow->addWidget(m_summaryLabel, 1);

    m_deleteBtn = new QPushButton(tr("删除选中"), this);
    m_deleteBtn->setEnabled(false);
    m_closeBtn  = new QPushButton(tr("关闭"), this);
    bottomRow->addWidget(m_deleteBtn);
    bottomRow->addWidget(m_closeBtn);
    root->addLayout(bottomRow);

    // ---- 连接 ----
    connect(m_filterEdit, &QLineEdit::textChanged,
            this, &HistoryDialog::onFilterChanged);
    connect(m_clearBtn, &QPushButton::clicked,
            this, &HistoryDialog::onClearAll);
    connect(m_exportBtn, &QPushButton::clicked,
            this, &HistoryDialog::onExport);
    connect(m_deleteBtn, &QPushButton::clicked,
            this, &HistoryDialog::onDeleteSelected);
    connect(m_closeBtn, &QPushButton::clicked,
            this, &QDialog::accept);
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &HistoryDialog::onSelectionChanged);
}

// ---------------------------------------------------------------
// 数据
// ---------------------------------------------------------------
void HistoryDialog::onRefresh()
{
    m_entries = HistoryDb::instance().query();
    fillTable(m_entries);
    updateSummary();
}

void HistoryDialog::fillTable(const QVector<HistoryEntry>& entries)
{
    // 排序时先禁用，避免填充过程中抖动
    m_table->setSortingEnabled(false);
    m_table->setRowCount(entries.size());

    auto makeItem = [](const QString& text,
                       Qt::Alignment align = Qt::AlignRight | Qt::AlignVCenter) {
        auto* it = new QTableWidgetItem(text);
        it->setTextAlignment(align);
        return it;
    };

    for (int i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];

        m_table->setItem(i, 0, makeItem(
            e.timestamp.toString("yyyy-MM-dd HH:mm"),
            Qt::AlignLeft | Qt::AlignVCenter));
        m_table->setItem(i, 1, makeItem(e.docName,
                                        Qt::AlignLeft | Qt::AlignVCenter));
        m_table->setItem(i, 2, makeItem(QString::number(e.charCount)));
        m_table->setItem(i, 3, makeItem(QString::number(e.correct)));
        m_table->setItem(i, 4, makeItem(QString::number(e.errors)));
        m_table->setItem(i, 5, makeItem(QString::number(e.backspaces)));
        m_table->setItem(i, 6, makeItem(QString::number(e.durationSeconds, 'f', 2) + " s"));
        m_table->setItem(i, 7, makeItem(QString::number(e.speedCPM, 'f', 1)));
        m_table->setItem(i, 8, makeItem(QString::number(e.accuracy, 'f', 1) + "%"));
        m_table->setItem(i, 9, makeItem(QString::number(e.codeLength, 'f', 2)));

        // 保存 id 到第一列
        m_table->item(i, 0)->setData(Qt::UserRole, e.id);

        // 高亮：准确率 < 90% 标红
        if (e.accuracy < 90.0) {
            auto& th = ThemeManager::instance();
            for (int c = 0; c < 10; ++c) {
                if (auto* it = m_table->item(i, c)) {
                    it->setBackground(th.color(ThemeManager::Error));
                    it->setForeground(th.color(ThemeManager::AccentText));
                }
            }
        }
    }

    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSortingEnabled(true);
}

void HistoryDialog::updateSummary()
{
    if (m_entries.isEmpty()) {
        m_summaryLabel->setText(tr("暂无记录"));
        return;
    }

    double totalSpeed = 0, totalAcc = 0;
    int totalChars = 0;
    for (const auto& e : m_entries) {
        totalSpeed += e.speedCPM;
        totalAcc += e.accuracy;
        totalChars += e.charCount;
    }
    const int n = m_entries.size();

    m_summaryLabel->setText(
        tr("共 %1 条 · 平均速度 %2 字/分 · 平均准确率 %3% · 累计输入 %4 字")
            .arg(n)
            .arg(totalSpeed / n, 0, 'f', 1)
            .arg(totalAcc / n, 0, 'f', 1)
            .arg(totalChars));
}

// ---------------------------------------------------------------
// 过滤
// ---------------------------------------------------------------
void HistoryDialog::onFilterChanged(const QString& keyword)
{
    if (keyword.isEmpty()) {
        fillTable(m_entries);
        updateSummary();
        return;
    }

    QVector<HistoryEntry> filtered;
    for (const auto& e : m_entries) {
        if (e.docName.contains(keyword, Qt::CaseInsensitive))
            filtered.append(e);
    }
    fillTable(filtered);
}

// ---------------------------------------------------------------
// 清空
// ---------------------------------------------------------------
void HistoryDialog::onClearAll()
{
    if (m_entries.isEmpty()) return;

    const auto ret = QMessageBox::question(this, tr("清空历史"),
        tr("确定要删除所有历史记录吗？此操作不可恢复。"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (!HistoryDb::instance().clearAll()) {
        QMessageBox::warning(this, tr("清空失败"),
            HistoryDb::instance().lastError());
        return;
    }
    onRefresh();
}

// ---------------------------------------------------------------
// 删除选中
// ---------------------------------------------------------------
void HistoryDialog::onDeleteSelected()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    auto* idItem = m_table->item(row, 0);
    if (!idItem) return;
    const int id = idItem->data(Qt::UserRole).toInt();

    const auto ret = QMessageBox::question(this, tr("删除记录"),
        tr("确定删除这一条记录吗？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (!HistoryDb::instance().remove(id)) {
        QMessageBox::warning(this, tr("删除失败"),
            HistoryDb::instance().lastError());
        return;
    }
    onRefresh();
}

void HistoryDialog::onSelectionChanged()
{
    m_deleteBtn->setEnabled(m_table->currentRow() >= 0);
}

// ---------------------------------------------------------------
// 导出
// ---------------------------------------------------------------
void HistoryDialog::onExport()
{
    if (m_entries.isEmpty()) {
        QMessageBox::information(this, tr("导出"), tr("没有记录可导出。"));
        return;
    }

    const QString defaultName = QStandardPaths::writableLocation(
        QStandardPaths::DocumentsLocation) + "/typing_history.csv";
    const QString path = QFileDialog::getSaveFileName(
        this, tr("导出 CSV"), defaultName,
        tr("CSV 文件 (*.csv);;所有文件 (*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("导出失败"), f.errorString());
        return;
    }

    // UTF-8 BOM，方便 Excel 识别
    f.write("\xEF\xBB\xBF");

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out << "时间,文件,字符,正确,错字,回改,耗时(s),速度(字/分),准确率(%),码长\n";
    for (const auto& e : m_entries) {
        out << e.timestamp.toString("yyyy-MM-dd HH:mm:ss") << ','
            << '"' << e.docName << '"' << ','
            << e.charCount << ','
            << e.correct << ','
            << e.errors << ','
            << e.backspaces << ','
            << QString::number(e.durationSeconds, 'f', 2) << ','
            << QString::number(e.speedCPM, 'f', 1) << ','
            << QString::number(e.accuracy, 'f', 1) << ','
            << QString::number(e.codeLength, 'f', 2) << '\n';
    }

    f.close();
    QMessageBox::information(this, tr("导出成功"),
        tr("已导出 %1 条记录到:\n%2").arg(m_entries.size()).arg(path));
}