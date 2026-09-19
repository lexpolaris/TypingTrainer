// src/app/ConfigManager.h
#pragma once
#include <QObject>
#include <QJsonObject>
#include <QVariant>
#include <QString>

class ConfigManager : public QObject
{
    Q_OBJECT
public:
    static ConfigManager& instance();

    void load();
    void save();

    QVariant get(const QString& key, const QVariant& defaultValue = {}) const;
    void set(const QString& key, const QVariant& value);

    // 便捷
    QString lastTextPath() const;
    int     fontPointSize() const;
    QString fontFamily() const;
    QString themeMode() const;         // "system" / "light" / "dark"
    QString currentCodeTable() const;  // "wubi86" / "wubi98" / "zhengma" / 用户路径

private:
    explicit ConfigManager(QObject* parent = nullptr);
    Q_DISABLE_COPY(ConfigManager)

    QJsonObject m_root;
};
