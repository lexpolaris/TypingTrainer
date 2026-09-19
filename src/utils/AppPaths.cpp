// src/utils/AppPaths.cpp
#include "AppPaths.h"
#include <QStandardPaths>
#include <QDir>

QString AppPaths::configDir()
{
    QString p = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(p);
    return p;
}

QString AppPaths::dataDir()
{
    QString p = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(p);
    return p;
}

QString AppPaths::codeTableDir()
{
    QString p = dataDir() + "/codetables";
    QDir().mkpath(p);
    return p;
}

QString AppPaths::textDir()
{
    QString p = dataDir() + "/texts";
    QDir().mkpath(p);
    return p;
}

QString AppPaths::historyDbPath()
{
    return dataDir() + "/history.db";
}

QString AppPaths::configFilePath()
{
    return configDir() + "/config.json";
}

void AppPaths::ensureDirsExist()
{
    configDir();
    dataDir();
    codeTableDir();
    textDir();
}
