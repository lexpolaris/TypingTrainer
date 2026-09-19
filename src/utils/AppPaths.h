// src/utils/AppPaths.h
#pragma once
#include <QString>

class AppPaths
{
public:
    static QString configDir();
    static QString dataDir();
    static QString codeTableDir();
    static QString textDir();
    static QString historyDbPath();
    static QString configFilePath();

    static void ensureDirsExist();

private:
    AppPaths() = delete;
};
