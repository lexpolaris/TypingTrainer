#pragma once
#include <QString>

/// 文本乱序工具
///
/// - removePunctuation: 去除中英文标点、空白、换行，仅保留汉字、字母、数字
/// - shuffle: 先去标点（可选），再按字符随机打乱顺序
class TextShuffler
{
public:
    static QString removePunctuation(const QString& text);
    static QString shuffle(const QString& text, bool removePunct = true);

private:
    TextShuffler() = delete;
};