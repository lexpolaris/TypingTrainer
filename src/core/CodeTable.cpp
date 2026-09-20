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
    return (u >= 0x4E00 && u <= 0x9FFF) ||
           (u >= 0x3400 && u <= 0x4DBF);
}

// ---------------------------------------------------------------
// 判断字符串是否全部为 ASCII 字母数字（编码合法性检查）
// ---------------------------------------------------------------
static bool isAsciiCode(const QString& s)
{
    if (s.isEmpty()) return false;
    for (QChar c : s)
        if (c.unicode() > 0x7F) return false;
    return true;
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
    m_wordToCodes.clear();
    m_codeToWords.clear();

    if (fmt == Auto) {
        const auto probeLines = content.split('\n', Qt::SkipEmptyParts);
        fmt = CharFirst;
        for (const QString& raw : probeLines) {
            const QString line = raw.trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            if (isHanChar(line.at(0))) { fmt = CharFirst; break; }

            // 第一个 token 是否纯 ASCII → CodeFirst
            int sep = -1;
            for (int i = 0; i < line.size(); ++i) {
                QChar c = line.at(i);
                if (c.isSpace() || c == '\t' || c == ',') { sep = i; break; }
            }
            if (sep > 0) {
                const QString token = line.left(sep);
                if (isAsciiCode(token)) { fmt = CodeFirst; break; }
            }
        }
    }

    const auto lines = content.split('\n');
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        if (fmt == CharFirst) {
            // 格式：汉字/词 <Tab 或空白> 编码1 编码2 ...
            // 取行首连续汉字作为"词"
            int wordEnd = 0;
            while (wordEnd < line.size() && isHanChar(line.at(wordEnd)))
                ++wordEnd;
            if (wordEnd == 0) continue;
            const QString word = line.left(wordEnd);

            // 定位分隔符（从词尾之后开始找）
            int sep = -1;
            for (int i = wordEnd; i < line.size(); ++i) {
                QChar c = line.at(i);
                if (c == '\t' || c.isSpace() || c == ',') { sep = i; break; }
            }
            if (sep <= 0) continue;

            QString codesStr = line.mid(sep + 1).trimmed();
            if (codesStr.isEmpty()) continue;

            QStringList codes = codesStr.split(
                QRegularExpression("[\\s,/]+"), Qt::SkipEmptyParts);

            QStringList validCodes;
            for (const QString& code : codes) {
                if (!isAsciiCode(code)) continue;
                validCodes.append(code);

                if (!m_codeToWords[code].contains(word))
                    m_codeToWords[code].append(word);
            }
            if (!validCodes.isEmpty()) {
                for (const QString& code : validCodes) {
                    if (!m_wordToCodes[word].contains(code))
                        m_wordToCodes[word].append(code);
                }
            }
        } else {
            // 格式：编码 <Tab 或空白> 词1 词2 ...
            int sep = -1;
            for (int i = 0; i < line.size(); ++i) {
                QChar c = line.at(i);
                if (c.isSpace() || c == '\t' || c == ',') { sep = i; break; }
            }
            if (sep <= 0) continue;

            const QString code = line.left(sep).trimmed();
            const QString wordsStr = line.mid(sep + 1).trimmed();
            if (!isAsciiCode(code) || wordsStr.isEmpty()) continue;

            // 按空白/逗号分词，每段作为独立候选
            QStringList words = wordsStr.split(
                QRegularExpression("[\\s,/]+"), Qt::SkipEmptyParts);

            bool any = false;
            for (const QString& w : words) {
                if (w.isEmpty()) continue;
                bool allHan = true;
                for (QChar c : w) {
                    if (!isHanChar(c)) { allHan = false; break; }
                }
                if (!allHan) continue;
                any = true;

                if (!m_codeToWords[code].contains(w))
                    m_codeToWords[code].append(w);
                if (!m_wordToCodes[w].contains(code))
                    m_wordToCodes[w].append(code);
            }
            if (!any) continue;
        }
    }

    return !isEmpty();
}

// ---------------------------------------------------------------
// 查询
// ---------------------------------------------------------------
QStringList CodeTable::codesFor(const QString& word) const
{
    return m_wordToCodes.value(word);
}

QStringList CodeTable::wordsFor(const QString& code) const
{
    return m_codeToWords.value(code);
}