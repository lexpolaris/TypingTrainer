// src/core/CodeTable.h
#pragma once

#include <QString>
#include <QStringList>
#include <QHash>

class CodeTable
{
public:
    /// 码表格式
    enum Format {
        Auto,           // 自动识别（按行首是否为汉字判断）
        CharFirst,      // 汉字<Tab>编码1 编码2 ...（本次格式）
        CodeFirst       // 编码<Tab>汉字（兼容旧格式）
    };

    bool loadFromFile(const QString& path, QString* error = nullptr);
    bool loadFromString(const QString& content, Format fmt = Auto);

    QString name() const { return m_name; }
    void setName(const QString& n) { m_name = n; }
    bool isEmpty() const { return m_charToCodes.isEmpty(); }

    /// 单字查询编码（返回该字的所有编码）
    QStringList codesFor(QChar ch) const;

    /// 编码查询候选字
    QStringList charsFor(const QString& code) const;

    /// 统计
    int charCount() const { return m_charToCodes.size(); }
    int codeCount() const { return m_codeToChars.size(); }

private:
    QString m_name;
    QHash<QChar, QStringList> m_charToCodes;   // 字 -> 编码列表
    QHash<QString, QStringList> m_codeToChars; // 编码 -> 候选字
};