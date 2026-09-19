#pragma once
#include <QObject>
#include <QString>
#include <QVector>

struct Paragraph
{
    int startIndex = 0;
    int length = 0;
    QString title;
};

class TextDocument : public QObject
{
    Q_OBJECT
public:
    explicit TextDocument(QObject* parent = nullptr) : QObject(parent) {}

    bool loadFromFile(const QString& path, QString* error = nullptr);
    bool loadFromString(const QString& text, const QString& name = {});

    const QString& text() const { return m_text; }
    int length() const { return static_cast<int>(m_text.length()); }
    QChar at(int i) const { return m_text.at(i); }
    bool isEmpty() const { return m_text.isEmpty(); }

    const QString& name() const { return m_name; }
    const QVector<Paragraph>& paragraphs() const { return m_paragraphs; }

private:
    void parseParagraphs();

    QString m_text;
    QString m_name;
    QVector<Paragraph> m_paragraphs;
};