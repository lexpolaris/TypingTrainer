// src/ui/controllers/SpeedController.cpp
#include "SpeedController.h"

#include "core/TypingSession.h"
#include "core/TextDocument.h"
#include "ui/SpeedPointDialog.h"
#include "ui/SpeedChartDialog.h"

#include <QMessageBox>

SpeedController::SpeedController(QObject* parent) : QObject(parent)
{
}

void SpeedController::openSpeedPointSettings(QWidget* parent,
                                             TextDocument* doc,
                                             TypingSession* session)
{
    if (!doc || doc->isEmpty()) {
        emit infoMessage(tr("测速点"), tr("请先加载文本。"));
        return;
    }

    SpeedPointDialog dlg(doc->text(), parent);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QVector<int> pts = dlg.selectedPoints();
    // 若会话已开始，重设测速点需要重置会话统计
    if (session->currentIndex() > session->initialStartIndex()) {
        auto ret = QMessageBox::question(parent, tr("测速点"),
            tr("重设测速点会重置当前统计，是否继续？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
        session->startFrom(doc->text(), session->initialStartIndex());
    }
    session->setSpeedPoints(pts);
    emit statusMessage(tr("已设置 %1 个测速点").arg(pts.size()), 3000);
}

void SpeedController::openSpeedChart(QWidget* parent,
                                     TextDocument* doc,
                                     TypingSession* session)
{
    if (!session || session->currentIndex() == 0) {
        emit infoMessage(tr("测速结果"), tr("尚未开始跟打。"));
        return;
    }

    SpeedChartDialog dlg(session, doc->text(), parent);
    connect(&dlg, &SpeedChartDialog::requestRetry,
            this, &SpeedController::retrySegmentRequested);
    dlg.exec();
}