// src/core/CodeTable.cpp
#include "CodeTable.h"
#include "utils/TextLoader.h"

#include <QFile>
#include <QFileInfo>

bool CodeTable::loadFromFile(const QString& path, QString* error)
{
    QString content = TextLoader::loadFile(path, error);
    if (content.isEmpty()) return false;
    m_name = QFileInfo(path).baseName();
    return loadFromString(content);
}

bool CodeTable::loadFromString(const QString& content)
{
    m_charToCodes.clear();
    m_codeToChars.clear();

    const auto lines = content.split('\n');
    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        // 找第一个空白或逗号作为分隔
        int sep = -1;
        for (int i = 0; i < line.size(); ++i) {
            QChar c = line.at(i);
            if (c.isSpace() || c == ',' || c == '\t') { sep = i; break; }
        }
        if (sep <= 0) continue;

        QString code = line.left(sep).trimmed();
        QString chars = line.mid(sep + 1).trimmed();
        if (code.isEmpty() || chars.isEmpty()) continue;

        QString hanChars;
        for (QChar c : chars) {
            if (c.unicode() >= 0x4E00 && c.unicode() <= 0x9FFF)
                hanChars.append(c);
        }
        if (hanChars.isEmpty()) continue;

        m_codeToChars[code].append(hanChars);
        for (QChar c : hanChars)
            m_charToCodes[c].append(code);
    }
    return !m_charToCodes.isEmpty();
}

QStringList CodeTable::codesFor(QChar ch) const
{
    return m_charToCodes.value(ch);
}

QStringList CodeTable::charsFor(const QString& code) const
{
    return m_codeToChars.value(code);
}
