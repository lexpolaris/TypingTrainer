#include "TextDocument.h"
#include "utils/TextLoader.h"
#include <QFileInfo>

bool TextDocument::loadFromFile(const QString& path, QString* error)
{
    QString t = TextLoader::loadFile(path, error);
    if (t.isEmpty() && error && !error->isEmpty()) return false;
    m_name = QFileInfo(path).fileName();
    m_text = t;
    parseParagraphs();
    emit contentChanged();
    return true;
}

bool TextDocument::loadFromString(const QString& text, const QString& name)
{
    m_text = text;
    m_name = name;
    parseParagraphs();
    emit contentChanged();
    return true;
}

void TextDocument::parseParagraphs()
{
    m_paragraphs.clear();
    int start = 0;
    int i = 0;
    while (i < m_text.length()) {
        if (m_text.at(i) == '\n') {
            int end = i;
            if (end > start) {
                m_paragraphs.append({start, end - start, {}});
            }
            while (i < m_text.length() && m_text.at(i) == '\n') ++i;
            start = i;
        } else {
            ++i;
        }
    }
    if (start < m_text.length()) {
        m_paragraphs.append({start, static_cast<int>(m_text.length() - start), {}});
    }
}