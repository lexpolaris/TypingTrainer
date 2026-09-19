// src/app/ConfigManager.cpp
#include "ConfigManager.h"
#include "utils/AppPaths.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <functional>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

ConfigManager::ConfigManager(QObject* parent) : QObject(parent)
{
    load();
}

void ConfigManager::load()
{
    QFile f(AppPaths::configFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        // 默认配置
        m_root = QJsonObject{
            {"general", QJsonObject{
                {"themeMode", "system"},
                {"fontFamily", ""},
                {"fontPointSize", 16}
            }},
            {"typing", QJsonObject{
                {"currentCodeTable", "wubi86"},
                {"showCodeHint", true}
            }},
            {"recent", QJsonObject{
                {"lastTextPath", ""}
            }}
        };
        return;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Config parse error:" << err.errorString();
        return;
    }
    m_root = doc.object();
}

void ConfigManager::save()
{
    QFile f(AppPaths::configFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Cannot save config:" << f.errorString();
        return;
    }
    f.write(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
}

QVariant ConfigManager::get(const QString& key, const QVariant& defaultValue) const
{
    QStringList parts = key.split('.', Qt::SkipEmptyParts);
    QJsonValue cur = m_root;
    for (const QString& p : parts) {
        if (!cur.isObject()) return defaultValue;
        cur = cur.toObject().value(p);
    }
    if (cur.isUndefined()) return defaultValue;
    return cur.toVariant();
}

void ConfigManager::set(const QString& key, const QVariant& value)
{
    QStringList parts = key.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;

    // 逐层创建
    QJsonObject* cur = &m_root;
    for (int i = 0; i < parts.size() - 1; ++i) {
        QJsonValue v = cur->value(parts[i]);
        if (!v.isObject()) {
            cur->insert(parts[i], QJsonObject{});
        }
        // 注意：这里需要引用，不能拷贝
        // 简化写法：重新构建
    }
    // 用更简单的方式：递归 set
    std::function<void(QJsonObject&, const QStringList&, const QVariant&)> rec =
        [&](QJsonObject& obj, const QStringList& keys, const QVariant& val) {
        if (keys.size() == 1) {
            obj.insert(keys[0], QJsonValue::fromVariant(val));
            return;
        }
        QJsonObject child = obj.value(keys[0]).toObject();
        rec(child, keys.mid(1), val);
        obj.insert(keys[0], child);
    };
    rec(m_root, parts, value);
}

QString ConfigManager::lastTextPath() const
{
    return get("recent.lastTextPath").toString();
}

int ConfigManager::fontPointSize() const
{
    return get("general.fontPointSize", 16).toInt();
}

QString ConfigManager::fontFamily() const
{
    return get("general.fontFamily").toString();
}

QString ConfigManager::themeMode() const
{
    return get("general.themeMode", "system").toString();
}

QString ConfigManager::currentCodeTable() const
{
    return get("typing.currentCodeTable", "wubi86").toString();
}
