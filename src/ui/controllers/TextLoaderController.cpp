// src/ui/controllers/TextLoaderController.cpp
#include "TextLoaderController.h"

#include "core/TextDocument.h"
#include "core/TextFilter.h"
#include "core/TextShuffler.h"
#include "app/ConfigManager.h"
#include "utils/TextLoader.h"

#include <QFileInfo>
#include <QRandomGenerator>

TextLoaderController::TextLoaderController(QObject* parent) : QObject(parent)
{
    m_doc = new TextDocument(this);
}

void TextLoaderController::loadFile(const QString& path)
{
    QString err;
    QString text = TextLoader::loadFile(path, &err);
    if (text.isEmpty() && !err.isEmpty()) {
        emit loadFailed(err);
        return;
    }
    const QString key = QFileInfo(path).absoluteFilePath();
    processAndEmit(text, QFileInfo(path).fileName(), key);
}

void TextLoaderController::loadResource(const QString& resPath)
{
    QString err;
    QString text = TextLoader::loadResource(resPath, &err);
    if (text.isEmpty()) {
        emit loadFailed(err);
        return;
    }
    processAndEmit(text, QFileInfo(resPath).fileName(), resPath);
}

void TextLoaderController::loadWelcome()
{
    static const QString kWelcome =
        QStringLiteral("欢迎使用爱不释手打字练习软件");
    processAndEmit(kWelcome, tr("欢迎"), QString());
}

void TextLoaderController::reload()
{
    if (m_originalText.isEmpty()) return;
    processAndEmit(m_originalText, m_docName, m_docKey);
}

void TextLoaderController::processAndEmit(const QString& rawText,
                                          const QString& name,
                                          const QString& key)
{
    m_originalText = rawText;
    m_docName = name;
    m_docKey = key;

    // ---- 1. 过滤 ----
    FilterOptions filterOpt = ConfigManager::instance().filterOptions();
    QString filtered = TextFilter::apply(rawText, filterOpt);

    // ---- 2. 乱序 ----
    QString content = m_shuffleMode
                          ? TextShuffler::shuffle(filtered)
                          : filtered;

    // ---- 3. 载入文档 ----
    m_doc->loadFromString(content, name);

    // ---- 4. 计算起始位置（依据 openMode） ----
    auto& cfg = ConfigManager::instance();
    const int om = cfg.openMode();   // 0 从头 1 随机 2 断点
    int startIndex = 0;
    if (om == 1) {
        const int total = content.length();
        if (total > 100)
            startIndex = QRandomGenerator::global()->bounded(total - 100);
        else
            startIndex = 0;
    } else if (om == 2) {
        startIndex = cfg.lastReadPosition(name);
        if (startIndex < 0 || startIndex >= content.length())
            startIndex = 0;
    }

    // ---- 5. 通知宿主 ----
    if (!key.isEmpty())
        emit textPathChanged(key);
    emit textReady(content, startIndex);
}
