// src/ui/controllers/CodeTableController.cpp
#include "CodeTableController.h"

#include "core/CodeTable.h"
#include "app/ConfigManager.h"
#include "utils/AppPaths.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFile>

CodeTableController::CodeTableController(QObject* parent) : QObject(parent)
{
    m_table = new CodeTable();
}

CodeTableController::~CodeTableController()
{
    delete m_table;
}

bool CodeTableController::loadAutoTable()
{
    const QString autoTable = ConfigManager::instance().autoLoadCodeTablePath();
    if (autoTable.isEmpty()) return false;

    QString err;
    if (m_table->loadFromFile(autoTable, &err)) {
        emit codeTableChanged(m_table);
        emit statusMessage(tr("已自动加载码表: %1").arg(m_table->name()), 3000);
        return true;
    }
    qWarning() << "自动加载码表失败:" << autoTable << err;
    return false;
}

void CodeTableController::loadBuiltin(const QString& name)
{
    QString path = QString(":/tables/%1.txt").arg(name);
    QString err;
    if (!m_table->loadFromFile(path, &err)) {
        emit infoMessage(tr("码表"),
            tr("内置码表 %1 尚未提供。\n请通过\"导入码表\"加载。").arg(name));
        return;
    }
    emit codeTableChanged(m_table);
    ConfigManager::instance().setAutoLoadCodeTablePath(path);
    ConfigManager::instance().save();
    emit statusMessage(tr("已加载码表: %1").arg(m_table->name()), 3000);
}

void CodeTableController::importFromFile(QWidget* parent)
{
    QString path = QFileDialog::getOpenFileName(
        parent, tr("导入码表"), AppPaths::codeTableDir(),
        tr("码表 (*.txt *.mb);;所有文件 (*)"));
    if (path.isEmpty()) return;

    QString err;
    if (!m_table->loadFromFile(path, &err)) {
        emit infoMessage(tr("导入失败"), err);
        return;
    }

    // 若源文件不在用户码表目录，复制一份进去，便于设置对话框列表统一管理
    QFileInfo fi(path);
    const QString dest = AppPaths::codeTableDir() + "/" + fi.fileName();
    if (fi.absolutePath() != AppPaths::codeTableDir() && !QFile::exists(dest))
        QFile::copy(path, dest);

    emit codeTableChanged(m_table);
    ConfigManager::instance().setAutoLoadCodeTablePath(
        QFile::exists(dest) ? dest : path);
    ConfigManager::instance().save();
    emit statusMessage(tr("已导入码表: %1").arg(m_table->name()), 3000);
}