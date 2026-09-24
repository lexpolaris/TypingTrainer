// src/ui/HistoryDialog.h
#pragma once

#include <QDialog>
#include <QVector>

#include "core/HistoryDb.h"

class QTableWidget;
class QLabel;
class QPushButton;
class QLineEdit;

class HistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HistoryDialog(QWidget* parent = nullptr);

private slots:
    void onRefresh();
    void onClearAll();
    void onExport();
    void onDeleteSelected();
    void onFilterChanged(const QString& keyword);
    void onSelectionChanged();

private:
    void setupUi();
    void fillTable(const QVector<HistoryEntry>& entries);
    void updateSummary();

    QLineEdit*    m_filterEdit = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel*       m_summaryLabel = nullptr;
    QPushButton*  m_clearBtn = nullptr;
    QPushButton*  m_exportBtn = nullptr;
    QPushButton*  m_deleteBtn = nullptr;
    QPushButton*  m_closeBtn = nullptr;

    QVector<HistoryEntry> m_entries;   // 当前显示的数据
};