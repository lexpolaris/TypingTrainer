// src/utils/TextLoader.cpp
#include "TextLoader.h"

#include <QFile>
#include <QStringConverter>
#include <QStringDecoder>

QString TextLoader::decode(const QByteArray& raw)
{
    if (raw.isEmpty()) return {};

    // BOM 检测
    if (raw.startsWith("\xEF\xBB\xBF"))
        return QString::fromUtf8(raw.mid(3));
    if (raw.startsWith("\xFF\xFE")) {
        // UTF-16 LE
        QStringDecoder dec(QStringDecoder::Utf16LE);
        return dec.decode(raw.mid(2));
    }
    if (raw.startsWith("\xFE\xFF")) {
        // UTF-16 BE
        QStringDecoder dec(QStringDecoder::Utf16BE);
        return dec.decode(raw.mid(2));
    }

    // 先尝试 UTF-8（Qt 6 的 fromUtf8 会替换无效字符，用解码器严格判断）
    {
        QStringDecoder dec(QStringDecoder::Utf8);
        QString s = dec.decode(raw);
        if (!dec.hasError())
            return s;
    }

    // 用本地编码兜底（locale 通常为 UTF-8，GBK 仍可能乱码）
    {
        QString s = QString::fromLocal8Bit(raw);
        // 若本地编码转换后无替换字符，认为可用
        if (!s.contains(QChar(0xFFFD)))
            return s;
    }

    // 最后兜底
    return QString::fromLatin1(raw);
}

QString TextLoader::loadFile(const QString& path, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = f.errorString();
        return {};
    }
    return decode(f.readAll());
}

QString TextLoader::loadResource(const QString& resPath, QString* error)
{
    QFile f(resPath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("无法打开资源: %1").arg(resPath);
        return {};
    }
    return decode(f.readAll());
}

bool TextLoader::saveFile(const QString& path, const QString& text, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = f.errorString();
        return false;
    }
    f.write(text.toUtf8());
    return true;
}
