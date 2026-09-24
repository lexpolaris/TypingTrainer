#include "SpeedChartDialog.h"

#include "theme/ThemeManager.h"
#include "app/ConfigManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QPushButton>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QFileInfo>
#include <QtMath>
#include <algorithm>
#include <QDateTime>
#include <QDir>

#ifdef HAVE_QTCHARTS
#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QCategoryAxis>
#endif

SpeedChartDialog::SpeedChartDialog(const TypingSession* session,
                                   const QString& targetText,
                                   QWidget* parent)
    : QDialog(parent)
    , m_session(session)
    , m_targetText(targetText)
{
    setWindowTitle(tr("测速信息"));
    resize(960, 720);
    setupUi();

    m_segments = computeSegments();
    fillTable(m_segments);
    buildChart(m_segments);

    m_titleLabel->setText(tr("跟打测速信息 · 全文 %1 字 · %2 个测速点")
        .arg(m_targetText.length())
        .arg(m_segments.size()));
}

void SpeedChartDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // 标题
    m_titleLabel = new QLabel(this);
    QFont tf = m_titleLabel->font();
    tf.setPointSize(tf.pointSize() + 2);
    tf.setBold(true);
    m_titleLabel->setFont(tf);
    root->addWidget(m_titleLabel);

    // 中部：表格 + 预览
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_table = new QTableWidget(splitter);
    m_table->setColumnCount(10);
    m_table->setHorizontalHeaderLabels({
        tr("序"), tr("说明"), tr("起点"), tr("终点"), tr("字数"),
        tr("时间"), tr("速度"), tr("击键"), tr("码长"), tr("回改")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    splitter->addWidget(m_table);

    m_preview = new QTextEdit(splitter);
    m_preview->setReadOnly(true);
    m_preview->setPlaceholderText(tr("点击左侧表格查看对应段落原文..."));
    splitter->addWidget(m_preview);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    root->addWidget(splitter, 2);

    // 图表
    #ifdef HAVE_QTCHARTS
        m_chartView = new QChartView(this);
        m_chartView->setRenderHint(QPainter::Antialiasing);
        m_chartView->setMinimumHeight(200);
        root->addWidget(m_chartView, 1);
    #else
        auto* chartPlaceholder = new QLabel(
            tr("（未启用 QtCharts，图表不可用。重新编译时确保 Qt6::Charts 可用）"),
            this);
        chartPlaceholder->setAlignment(Qt::AlignCenter);
        root->addWidget(chartPlaceholder, 1);
    #endif

    // 底部：汇总 + 按钮
    auto* bottomBar = new QHBoxLayout();
    m_summaryLabel = new QLabel(this);
    bottomBar->addWidget(m_summaryLabel, 1);

    auto* btnRetry = new QPushButton(tr("重打当前"), this);
    auto* btnCopy = new QPushButton(tr("复制截图"), this);
    auto* btnSave = new QPushButton(tr("保存图片..."), this);
    auto* btnExit = new QPushButton(tr("退出"), this);
    bottomBar->addWidget(btnRetry);
    bottomBar->addWidget(btnCopy);
    bottomBar->addWidget(btnSave);
    bottomBar->addWidget(btnExit);
    root->addLayout(bottomBar);

    connect(m_table, &QTableWidget::cellClicked,
            this, &SpeedChartDialog::onRowChanged);
    connect(btnRetry, &QPushButton::clicked,
            this, &SpeedChartDialog::onRetry);
    connect(btnCopy, &QPushButton::clicked,
            this, &SpeedChartDialog::onCopyImage);
    connect(btnSave, &QPushButton::clicked,
            this, &SpeedChartDialog::onSaveImage);
    connect(btnExit, &QPushButton::clicked, this, &QDialog::accept);
}

// ---------------------------------------------------------------
// 分段统计（增量差分）
// ---------------------------------------------------------------
QVector<SegmentStat> SpeedChartDialog::computeSegments() const
{
    QVector<SegmentStat> result;
    if (!m_session) return result;

    const auto& snaps = m_session->snapshots();
    const int totalLen = m_targetText.length();

    auto makeStat = [&](int idx, const QString& desc,
                        int start, int end,
                        double t, int keys, int backs) -> SegmentStat {
        SegmentStat s;
        s.index = idx;
        s.description = desc;
        s.start = start;
        s.end = end;
        s.length = end - start;
        s.timeSeconds = t;
        s.speedCPM = (t > 0 && s.length > 0) ? s.length * 60.0 / t : 0;
        s.keystrokePerSec = (t > 0) ? keys / t : 0;
        s.codeLength = (s.length > 0) ? double(keys) / s.length : 0;
        s.backspaces = backs;
        return s;
    };

    // 第一段
    int prevPos = m_session->initialStartIndex();
    int prevKeys = 0;
    int prevBacks = 0;
    double prevTime = 0;

    for (int i = 0; i < snaps.size(); ++i) {
        const auto& s = snaps[i];
        int len = s.position - prevPos;
        if (len > 0) {
            double t = s.timeSeconds - prevTime;
            QString desc = m_targetText.mid(qMax(0, prevPos), qMin(2, len));
            result.append(makeStat(i + 1, desc, prevPos, s.position,
                                   t, s.keystrokes - prevKeys,
                                   s.backspaces - prevBacks));
        }
        prevPos = s.position;
        prevKeys = s.keystrokes;
        prevBacks = s.backspaces;
        prevTime = s.timeSeconds;
    }

    // 最后一段（到全文结束）
    if (prevPos < totalLen) {
        int len = totalLen - prevPos;
        double t = m_session->elapsedSeconds() - prevTime;
        if (t < 0) t = 0;
        QString desc = m_targetText.mid(prevPos, qMin(2, len));
        result.append(makeStat(snaps.size() + 1, desc, prevPos, totalLen,
                               t, m_session->totalKeystrokes() - prevKeys,
                               m_session->backspaceCount() - prevBacks));
    }

    return result;
}

// ---------------------------------------------------------------
// 填表
// ---------------------------------------------------------------
void SpeedChartDialog::fillTable(const QVector<SegmentStat>& segs)
{
    auto& th = ThemeManager::instance();
    const double fullSpeed = m_session->speedCPM();

    m_table->setRowCount(segs.size() + 1);  // +1 用于全文行

    for (int i = 0; i < segs.size(); ++i) {
        const auto& s = segs[i];
        auto setItem = [&](int col, const QString& text, bool rightAlign = true) {
            auto* item = new QTableWidgetItem(text);
            if (rightAlign) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            else            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_table->setItem(i, col, item);
        };
        setItem(0, QString::number(s.index), false);
        setItem(1, s.description, false);
        setItem(2, QString::number(s.start));
        setItem(3, QString::number(s.end));
        setItem(4, QString::number(s.length));
        setItem(5, QString::number(s.timeSeconds, 'f', 2));
        setItem(6, QString::number(s.speedCPM, 'f', 1));
        setItem(7, QString::number(s.keystrokePerSec, 'f', 1));
        setItem(8, QString::number(s.codeLength, 'f', 2));
        setItem(9, QString::number(s.backspaces));

        // 慢于全文平均的段标黄
        if (fullSpeed > 0 && s.speedCPM < fullSpeed) {
            for (int c = 0; c < 10; ++c) {
                if (auto* it = m_table->item(i, c)) {
                    it->setBackground(th.color(ThemeManager::Highlight));
                    it->setForeground(th.color(ThemeManager::TextPrimary));
                }
            }
        }
    }

    // 全文行
    int row = segs.size();
    auto setFull = [&](int col, const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        item->setBackground(th.color(ThemeManager::SurfaceBg));
        item->setForeground(th.color(ThemeManager::Accent));
        QFont f = item->font(); f.setBold(true); item->setFont(f);
        m_table->setItem(row, col, item);
    };
    setFull(0, tr("全"));
    setFull(1, tr("全文"));
    setFull(2, QString::number(0));
    setFull(3, QString::number(m_targetText.length()));
    setFull(4, QString::number(m_targetText.length()));
    setFull(5, QString::number(m_session->elapsedSeconds(), 'f', 2));
    setFull(6, QString::number(fullSpeed, 'f', 1));
    setFull(7, QString::number(m_session->keystrokePerSec(), 'f', 1));
    setFull(8, QString::number(m_session->codeLength(), 'f', 2));
    setFull(9, QString::number(m_session->backspaceCount()));

    m_table->resizeColumnsToContents();

    // 底部汇总
    m_summaryLabel->setText(tr("全文: %1 字 · 用时 %2 s · 速度 %3 字/分 · 击键 %4 · 码长 %5 · 错字 %6 · 回改 %7")
        .arg(m_targetText.length())
        .arg(m_session->elapsedSeconds(), 0, 'f', 2)
        .arg(fullSpeed, 0, 'f', 1)
        .arg(m_session->keystrokePerSec(), 0, 'f', 1)
        .arg(m_session->codeLength(), 0, 'f', 2)
        .arg(m_session->errorChars())
        .arg(m_session->backspaceCount()));
}

// ---------------------------------------------------------------
// 曲线
// ---------------------------------------------------------------
void SpeedChartDialog::buildChart(const QVector<SegmentStat>& segs)
{
#ifdef HAVE_QTCHARTS
    if (segs.isEmpty()) return;

    auto& th = ThemeManager::instance();

    auto* series = new QLineSeries();
    series->setName(tr("分段速度"));
    series->setPointsVisible(true);

    double minSpeed = 1e9, maxSpeed = 0;

    for (int i = 0; i < segs.size(); ++i) {
        double v = segs[i].speedCPM;
        series->append(i + 1, v);
        minSpeed = qMin(minSpeed, v);
        maxSpeed = qMax(maxSpeed, v);
    }

    // 全文平均线
    auto* avgSeries = new QLineSeries();
    avgSeries->setName(tr("全文平均"));
    double avg = m_session->speedCPM();
    if (!segs.isEmpty()) {
        avgSeries->append(1, avg);
        avgSeries->append(segs.size(), avg);
    }
    QPen avgPen(th.color(ThemeManager::TextSecondary), 1, Qt::DashLine);
    avgSeries->setPen(avgPen);

    auto* chart = new QChart();
    chart->addSeries(series);
    chart->addSeries(avgSeries);
    chart->setTitle(tr("分段速度曲线"));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setBackgroundBrush(th.color(ThemeManager::SurfaceBg));
    chart->setTitleBrush(QBrush(th.color(ThemeManager::TextPrimary)));

    auto* axisX = new QValueAxis();
    axisX->setTitleText(tr("段"));
    axisX->setLabelFormat("%d");
    axisX->setRange(1, qMax(2, segs.size()));
    axisX->setTickCount(qMin(segs.size() + 1, 11));
    axisX->setTitleBrush(QBrush(th.color(ThemeManager::TextSecondary)));
    axisX->setLabelsBrush(QBrush(th.color(ThemeManager::TextSecondary)));
    axisX->setGridLineColor(th.color(ThemeManager::Border));

    auto* axisY = new QValueAxis();
    axisY->setTitleText(tr("字/分"));
    int minY = int((minSpeed - 10) / 100) * 100;
    axisY->setRange(qMax(0, minY), maxSpeed + 20);
    axisY->setTitleBrush(QBrush(th.color(ThemeManager::TextSecondary)));
    axisY->setLabelsBrush(QBrush(th.color(ThemeManager::TextSecondary)));
    axisY->setGridLineColor(th.color(ThemeManager::Border));

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    avgSeries->attachAxis(axisX);
    avgSeries->attachAxis(axisY);

    m_chartView->setChart(chart);
#endif
}

// ---------------------------------------------------------------
// 段预览
// ---------------------------------------------------------------
void SpeedChartDialog::onRowChanged(int row, int)
{
    updatePreview(row);
}

void SpeedChartDialog::updatePreview(int row)
{
    if (row < 0 || row >= m_segments.size()) {
        if (row == m_segments.size()) {
            m_preview->setPlainText(m_targetText);  // 全文
        }
        return;
    }
    const auto& s = m_segments[row];
    if (s.start < 0 || s.end > m_targetText.length() || s.start >= s.end) return;
    m_preview->setPlainText(m_targetText.mid(s.start, s.end - s.start));
}

// ---------------------------------------------------------------
// 截图
// ---------------------------------------------------------------
QPixmap SpeedChartDialog::renderToPixmap() const
{
    const int W = width();
    const int H = height();
    QPixmap pm(W, H);
    pm.fill(ThemeManager::instance().color(ThemeManager::WindowBg));

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // ---- 标题 ----
    QFont tf = font();
    tf.setPointSize(tf.pointSize() + 2);
    tf.setBold(true);
    p.setFont(tf);
    p.setPen(ThemeManager::instance().color(ThemeManager::TextPrimary));
    const QString title = m_titleLabel->text();
    p.drawText(QRect(12, 8, W - 24, 30),
               Qt::AlignLeft | Qt::AlignVCenter, title);

    // ---- 表格 ----
    QPixmap tablePm = m_table->grab();
    const int tableY = 44;
    p.drawPixmap(12, tableY, tablePm);

    // ---- 预览 ----
    QPixmap previewPm = m_preview->grab();
    p.drawPixmap(12 + tablePm.width() + 8, tableY, previewPm);

    // ---- 图表 ----
#ifdef HAVE_QTCHARTS
    if (m_chartView) {
        QPixmap chartPm = m_chartView->grab();
        const int chartY = tableY + tablePm.height() + 12;
        p.drawPixmap(12, chartY, chartPm);
    }
#endif

    // ---- 右下角署名 ----
    QFont small = font();
    small.setPointSize(small.pointSize() - 1);
    p.setFont(small);
    p.setPen(ThemeManager::instance().color(ThemeManager::TextSecondary));
    p.drawText(QRect(12, H - 26, W - 24, 18),
               Qt::AlignRight | Qt::AlignVCenter,
               tr("打字练习 · %1")
                   .arg(QDateTime::currentDateTime()
                            .toString("yyyy-MM-dd HH:mm")));

    p.end();
    return pm;
}

void SpeedChartDialog::onCopyImage()
{
    QPixmap pm = renderToPixmap();
    QApplication::clipboard()->setPixmap(pm);
    QMessageBox::information(this, tr("复制截图"),
        tr("截图已复制到剪贴板 (%1 x %2)").arg(pm.width()).arg(pm.height()));
}

void SpeedChartDialog::onSaveImage()
{
    QString defaultName = QDir::homePath() + "/"
        + QString("typing_speed_%1.png")
              .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString path = QFileDialog::getSaveFileName(
        this, tr("保存测速截图"), defaultName,
        tr("PNG 图片 (*.png);;JPEG 图片 (*.jpg)"));
    if (path.isEmpty()) return;

    QPixmap pm = renderToPixmap();
    if (pm.save(path)) {
        QMessageBox::information(this, tr("保存成功"),
            tr("已保存到: %1").arg(path));
    } else {
        QMessageBox::warning(this, tr("保存失败"),
            tr("无法保存图片到: %1").arg(path));
    }
}

// ---------------------------------------------------------------
// 重打当前段
// ---------------------------------------------------------------
void SpeedChartDialog::onRetry()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_segments.size()) {
        QMessageBox::information(this, tr("重打当前"),
            tr("请先选择要重打的段落。"));
        return;
    }
    const auto& s = m_segments[row];
    QString segText = m_targetText.mid(s.start, s.end - s.start);

    auto ret = QMessageBox::question(this, tr("重打当前"),
        tr("将重置所有统计，并只跟打这一段（%1 字）。\n继续？").arg(segText.length()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    emit requestRetry(segText);
    accept();
}