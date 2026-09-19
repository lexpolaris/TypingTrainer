// src/core/CodeTable.cpp
#include "CodeTable.h"
#include "utils/TextLoader.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

// ---------------------------------------------------------------
// 判断字符是否为汉字（CJK 基本区 + 扩展 A）
// ---------------------------------------------------------------
static bool isHanChar(QChar c)
{
    const ushort u = c.unicode();
    // CJK 基本区：4E00-9FFF
    // CJK 扩展 A：3400-4DBF（部分字符）
    return (u >= 0x4E00 && u <= 0x9FFF) ||
           (u >= 0x3400 && u <= 0x4DBF);
}

// ---------------------------------------------------------------
// 加载
// ---------------------------------------------------------------
bool CodeTable::loadFromFile(const QString& path, QString* error)
{
    QString content = TextLoader::loadFile(path, error);
    if (content.isEmpty()) return false;
    m_name = QFileInfo(path).baseName();
    return loadFromString(content, Auto);
}

bool CodeTable::loadFromString(const QString& content, Format fmt)
{
    m_charToCodes.clear();
    m_codeToChars.clear();

    if (fmt == Auto) {
        // 逐行探测
        const auto probeLines = content.split('\n', Qt::SkipEmptyParts);
        fmt = CharFirst;
        for (const QString& raw : probeLines) {
            const QString line = raw.trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            if (isHanChar(line.at(0))) { fmt = CharFirst; break; }

            // 第一个 token 是否纯 ASCII
            int sep = -1;
            for (int i = 0; i < line.size(); ++i) {
                QChar c = line.at(i);
                if (c.isSpace() || c == '\t' || c == ',') { sep = i; break; }
            }
            if (sep > 0) {
                const QString token = line.left(sep);
                bool ascii = true;
                for (QChar c : token)
                    if (c.unicode() > 0x7F) { ascii = false; break; }
                if (ascii) { fmt = CodeFirst; break; }
            }
        }
    }

    const auto lines = content.split('\n');
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        if (fmt == CharFirst) {
            // 汉字<Tab>编码1 编码2 ...
            QChar han = line.at(0);
            if (!isHanChar(han)) continue;

            // Tab 或空白分隔
            int sep = -1;
            for (int i = 1; i < line.size(); ++i) {
                QChar c = line.at(i);
                if (c == '\t' || c.isSpace() || c == ',') { sep = i; break; }
            }
            if (sep <= 0) continue;

            QString codesStr = line.mid(sep + 1).trimmed();
            if (codesStr.isEmpty()) continue;

            // 编码可能用空格/逗号/斜杠分隔
            QStringList codes = codesStr.split(
                QRegularExpression("[\\s,/]+"), Qt::SkipEmptyParts);

            QStringList validCodes;
            for (const QString& code : codes) {
                if (code.isEmpty()) continue;
                // 编码只允许 ASCII 字母数字
                bool ascii = true;
                for (QChar c : code)
                    if (c.unicode() > 0x7F) { ascii = false; break; }
                if (!ascii) continue;

                validCodes.append(code);
                // 反向映射
                if (!m_codeToChars[code].contains(han))
                    m_codeToChars[code].append(han);
            }
            if (!validCodes.isEmpty())
                m_charToCodes[han].append(validCodes);
        } else {
            // 兼容旧格式：编码<Tab>汉字1 汉字2 ...
            int sep = -1;
            for (int i = 0; i < line.size(); ++i) {
                QChar c = line.at(i);
                if (c.isSpace() || c == '\t' || c == ',') { sep = i; break; }
            }
            if (sep <= 0) continue;

            const QString code = line.left(sep).trimmed();
            const QString charsStr = line.mid(sep + 1).trimmed();
            if (code.isEmpty() || charsStr.isEmpty()) continue;

            QString hanChars;
            for (QChar c : charsStr) {
                if (isHanChar(c)) hanChars.append(c);
            }
            if (hanChars.isEmpty()) continue;

            m_codeToChars[code].append(hanChars);
            for (QChar c : hanChars)
                if (!m_charToCodes[c].contains(code))
                    m_charToCodes[c].append(code);
        }
    }

    return !m_charToCodes.isEmpty();
}

// ---------------------------------------------------------------
// 查询
// ---------------------------------------------------------------
QStringList CodeTable::codesFor(QChar ch) const
{
    return m_charToCodes.value(ch);
}

QStringList CodeTable::charsFor(const QString& code) const
{
    return m_codeToChars.value(code);
}