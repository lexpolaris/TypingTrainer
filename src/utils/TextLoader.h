// src/utils/TextLoader.h
#pragma once
#include <QString>
#include <QByteArray>

class TextLoader
{
public:
    /// 自动检测编码并解码（UTF-8 BOM / UTF-8 / GBK / Latin-1 兜底）
    static QString decode(const QByteArray& raw);

    /// 从文件加载（自动编码检测）
    static QString loadFile(const QString& path, QString* error = nullptr);

    /// 从 Qt 资源加载
    static QString loadResource(const QString& resPath, QString* error = nullptr);

    /// 保存为 UTF-8
    static bool saveFile(const QString& path, const QString& text, QString* error = nullptr);

private:
    TextLoader() = delete;
};
