// src/ui/MistakeDialog.h
#pragma once

#include <QDialog>
#include <QVector>

#include "core/TypingSession.h"

class QTableWidget;
class QLabel;
class QPushButton;

class MistakeDialog : public QDialog
{
    Q_OBJECT
public:
    MistakeDialog(const QVector<MistakeRecord>& mistakes,
                  const QString& targetText,
                  QWidget* parent = nullptr);

    /// 用户双击的位置（-1 表示未选择）
    int jumpPosition() const { return m_jumpPosition; }

signals:
    /// 请求跳转到指定位置
    void jumpRequested(int position);

private slots:
    void onItemDoubleClicked(int row, int col);
    void onJumpClicked();

private:
    void setupUi();
    void fillTable();
    void updatePreview(int row);
    QString contextAround(int pos, int radius = 8) const;

    QVector<MistakeRecord> m_mistakes;
    QString m_targetText;
    int m_jumpPosition = -1;

    QTableWidget* m_table = nullptr;
    QLabel*       m_previewLabel = nullptr;
    QLabel*       m_summaryLabel = nullptr;
    QPushButton*  m_jumpBtn = nullptr;
};