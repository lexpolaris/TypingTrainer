// src/core/TextFilter.h
#pragma once

#include <QString>

/// 文本过滤选项
///
/// 与 ConfigManager 的 "filter" 分组对应。
/// 全部关闭时 apply() 原样返回。
struct FilterOptions
{
    // ---- 过滤（删除） ----
    bool filterHan      = false;  // 过滤汉字及全角标点
    bool filterNonHan   = false;  // 过滤非汉字字符（保留汉字）
    bool filterUpper    = false;  // 过滤大写字母
    bool filterLower    = false;  // 过滤小写字母
    bool filterDigit    = false;  // 过滤数字
    bool filterSpace    = false;  // 过滤空格（半角 + 全角）

    // ---- 转换 ----
    bool upperToLower   = false;  // 大写转小写
    bool lowerToUpper   = false;  // 小写转大写
};

/// 文本过滤器
///
/// 过滤规则顺序（与 UI 勾选顺序无关）：
///   1. 先按 filterXxx 删除不需要的字符
///   2. 再按 upperToLower / lowerToUpper 转换大小写
///
/// 若同时勾选 filterUpper 和 upperToLower，
/// 过滤器会先删除大写字母，转换步骤对大写无效——
/// 这是"过滤"与"转换"互斥的自然结果，与 UI 一致。
class TextFilter
{
public:
    /// 对文本应用过滤选项
    static QString apply(const QString& text, const FilterOptions& opt);

    /// 判断单个字符是否为汉字（CJK 基本区 + 扩展 A）
    static bool isHan(QChar c);

private:
    TextFilter() = delete;
};