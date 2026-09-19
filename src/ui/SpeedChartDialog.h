#pragma once

#include <QDialog>
#include <QVector>
#include <QString>

#include "core/TypingSession.h"

class QTableWidget;
class QTextEdit;
class QLabel;

#ifdef HAVE_QTCHARTS
#include <QChartView>
#endif

struct SegmentStat
{
    int     index = 0;
    QString description;
    int     start = 0;
    int     end = 0;
    int     length = 0;
    double  timeSeconds = 0;
    double  speedCPM = 0;
    double  keystrokePerSec = 0;
    double  codeLength = 0;
    int     backspaces = 0;
};

class SpeedChartDialog : public QDialog
{
    Q_OBJECT
public:
    SpeedChartDialog(const TypingSession* session,
                     const QString& targetText,
                     QWidget* parent = nullptr);

signals:
    void requestRetry(const QString& segmentText);

private slots:
    void onRowChanged(int row, int column);
    void onCopyImage();
    void onSaveImage();
    void onRetry();

private:
    void setupUi();
    QVector<SegmentStat> computeSegments() const;
    void fillTable(const QVector<SegmentStat>& segs);
    void buildChart(const QVector<SegmentStat>& segs);
    void updatePreview(int row);
    QPixmap renderToPixmap() const;

    const TypingSession* m_session = nullptr;
    QString m_targetText;
    QVector<SegmentStat> m_segments;

    QLabel*        m_titleLabel = nullptr;
    QTableWidget*  m_table = nullptr;
    QTextEdit*     m_preview = nullptr;
    QLabel*        m_summaryLabel = nullptr;

#ifdef HAVE_QTCHARTS
    QChartView* m_chartView = nullptr;
#endif
};