// src/core/CodeTable.h
#pragma once
#include <QString>
#include <QStringList>
#include <QHash>

class CodeTable
{
public:
    bool loadFromFile(const QString& path, QString* error = nullptr);
    bool loadFromString(const QString& content);

    QString name() const { return m_name; }
    void setName(const QString& n) { m_name = n; }
    bool isEmpty() const { return m_charToCodes.isEmpty(); }

    QStringList codesFor(QChar ch) const;
    QStringList charsFor(const QString& code) const;

private:
    QString m_name;
    QHash<QChar, QStringList> m_charToCodes;
    QHash<QString, QStringList> m_codeToChars;
};
