#include "TextShuffler.h"

#include <QRandomGenerator>
#include <vector>
#include <algorithm>

static bool isKeepChar(QChar c)
{
    const ushort u = c.unicode();
    // 汉字（CJK 基本区 + 扩展 A）
    if ((u >= 0x4E00 && u <= 0x9FFF) || (u >= 0x3400 && u <= 0x4DBF))
        return true;
    // 字母、数字
    if (c.isLetterOrNumber())
        return true;
    return false;
}

QString TextShuffler::removePunctuation(const QString& text)
{
    QString result;
    result.reserve(text.size());
    for (QChar c : text) {
        if (isKeepChar(c))
            result.append(c);
    }
    return result;
}

QString TextShuffler::shuffle(const QString& text, bool removePunct)
{
    const QString base = removePunct ? removePunctuation(text) : text;

    std::vector<QChar> chars;
    chars.reserve(base.size());
    for (QChar c : base)
        chars.push_back(c);

    std::shuffle(chars.begin(), chars.end(), *QRandomGenerator::global());

    QString result;
    result.reserve(chars.size());
    for (QChar c : chars)
        result.append(c);
    return result;
}