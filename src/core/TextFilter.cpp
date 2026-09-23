// src/core/TextFilter.cpp
#include "TextFilter.h"

bool TextFilter::isHan(QChar c)
{
    const ushort u = c.unicode();
    return (u >= 0x4E00 && u <= 0x9FFF) ||   // CJK 基本区
           (u >= 0x3400 && u <= 0x4DBF);      // CJK 扩展 A
}

QString TextFilter::apply(const QString& text, const FilterOptions& opt)
{
    // 全部关闭：原样返回
    if (!opt.filterHan && !opt.filterNonHan &&
        !opt.filterUpper && !opt.filterLower &&
        !opt.filterDigit && !opt.filterSpace &&
        !opt.upperToLower && !opt.lowerToUpper) {
        return text;
    }

    QString result;
    result.reserve(text.size());

    for (QChar c : text) {
        // ---------- 1. 删除 ----------
        if (opt.filterHan && isHan(c))
            continue;

        // filterNonHan 与 filterHan 互斥：
        // filterNonHan 表示"只保留汉字，删除其他"，
        // 因此这里判断 !isHan 而不是"是汉字"
        if (opt.filterNonHan && !isHan(c))
            continue;

        if (opt.filterUpper && c.isUpper())
            continue;

        if (opt.filterLower && c.isLower())
            continue;

        if (opt.filterDigit && c.isDigit())
            continue;

        if (opt.filterSpace && (c == QChar(' ') || c == QChar(0x3000)))
            continue;

        // ---------- 2. 转换 ----------
        if (opt.upperToLower && c.isUpper()) {
            result.append(c.toLower());
            continue;
        }
        if (opt.lowerToUpper && c.isLower()) {
            result.append(c.toUpper());
            continue;
        }

        result.append(c);
    }

    return result;
}