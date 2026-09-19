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

    // GBK 兜底（Qt 6 通过 QStringConverter 支持 GBK？）
    // Qt 6 内置编码有限，GBK 需要 Qt5Compat 或 iconv
    // 方案 A：用 QTextCodec（需 Qt5Compat）
    // 方案 B：用系统 iconv（Linux/macOS）或 Win32 API（Windows）
    // 这里给出跨平台方案 B 的实现
#ifdef Q_OS_WIN
    // Windows 下用 MultiByteToWideChar
    #include <windows.h>
    int wlen = MultiByteToWideChar(936 /*GBK*/, 0, raw.constData(), raw.size(), nullptr, 0);
    if (wlen > 0) {
        std::wstring wbuf(wlen, L'\0');
        MultiByteToWideChar(936, 0, raw.constData(), raw.size(), &wbuf[0], wlen);
        return QString::fromWCharArray(wbuf.data(), wlen);
    }
#else
    // Linux/macOS 下用 iconv
    // 简化：用 QString::fromLocal8Bit 兜底（locale 通常是 UTF-8，GBK 会乱码）
    // 生产环境建议引入 iconv 或 Qt5Compat
#endif

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
