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

    /// 有词或有字都算非空
    bool isEmpty() const {
        return m_wordToCodes.isEmpty() && m_codeToWords.isEmpty();
    }

    /// 查询编码（word 长度 1 即单字，>1 即词语）
    QStringList codesFor(const QString& word) const;

    /// 编码查询候选词（返回的 QString 可能是单字或词组）
    QStringList wordsFor(const QString& code) const;

    /// 统计
    int wordCount() const { return m_wordToCodes.size(); }
    int codeCount() const { return m_codeToWords.size(); }

private:
    QString m_name;
    // 词/字 -> 编码列表
    QHash<QString, QStringList> m_wordToCodes;
    // 编码 -> 候选词/字（按加入顺序）
    QHash<QString, QStringList> m_codeToWords;
};