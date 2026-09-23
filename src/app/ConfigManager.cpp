// src/app/ConfigManager.cpp
#include "ConfigManager.h"
#include "utils/AppPaths.h"
#include "core/TextFilter.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QColor>
#include <QFont>
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
                {"fontPointSize", 16},
                {"fontBold", false},
                {"fontItalic", false}
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
    if (!doc.isObject()) {
        qWarning() << "Config root is not an object, using defaults";
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


QString ConfigManager::autoLoadCodeTablePath() const
{
    return get("typing.autoLoadCodeTablePath", "").toString();
}

void ConfigManager::setAutoLoadCodeTablePath(const QString& path)
{
    set("typing.autoLoadCodeTablePath", path);
}

QHash<int, QColor> ConfigManager::customThemeColors() const
{
    QHash<int, QColor> result;
    QJsonObject obj = get("theme.customColors").toJsonObject();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        bool ok = false;
        int role = it.key().toInt(&ok);
        if (!ok) continue;
        QColor c(it.value().toString());
        if (c.isValid()) result.insert(role, c);
    }
    return result;
}

void ConfigManager::setCustomThemeColors(const QHash<int, QColor>& colors)
{
    QJsonObject obj;
    for (auto it = colors.begin(); it != colors.end(); ++it) {
        obj.insert(QString::number(it.key()), it.value().name(QColor::HexArgb));
    }
    set("theme.customColors", obj);
}

QFont ConfigManager::typingFont() const
{
    QFont f;
    f.setFamily(get("general.fontFamily", "").toString());
    f.setPointSize(get("general.fontPointSize", 18).toInt());
    f.setBold(get("general.fontBold", false).toBool());
    f.setItalic(get("general.fontItalic", false).toBool());
    return f;
}

void ConfigManager::setTypingFont(const QFont& f)
{
    set("general.fontFamily", f.family());
    set("general.fontPointSize", f.pointSize());
    set("general.fontBold", f.bold());
    set("general.fontItalic", f.italic());
}

FilterOptions ConfigManager::filterOptions() const
{
    FilterOptions opt;
    opt.filterHan     = get("filter.filterHan",     false).toBool();
    opt.filterNonHan  = get("filter.filterNonHan",  false).toBool();
    opt.filterUpper   = get("filter.filterUpper",   false).toBool();
    opt.filterLower   = get("filter.filterLower",   false).toBool();
    opt.filterDigit   = get("filter.filterDigit",   false).toBool();
    opt.filterSpace   = get("filter.filterSpace",   false).toBool();
    opt.upperToLower  = get("filter.upperToLower",  false).toBool();
    opt.lowerToUpper  = get("filter.lowerToUpper",  false).toBool();
    return opt;
}

void ConfigManager::setFilterOptions(const FilterOptions& opt)
{
    set("filter.filterHan",     opt.filterHan);
    set("filter.filterNonHan",  opt.filterNonHan);
    set("filter.filterUpper",   opt.filterUpper);
    set("filter.filterLower",   opt.filterLower);
    set("filter.filterDigit",   opt.filterDigit);
    set("filter.filterSpace",   opt.filterSpace);
    set("filter.upperToLower",  opt.upperToLower);
    set("filter.lowerToUpper",  opt.lowerToUpper);
}

// ---- 启动 ----
bool ConfigManager::loadLastTextOnStartup() const
{
    return get("startup.loadLastText", true).toBool();
}
void ConfigManager::setLoadLastTextOnStartup(bool on)
{
    set("startup.loadLastText", on);
}

void ConfigManager::setLastTextPath(const QString& path)
{
    set("recent.lastTextPath", path);
}

bool ConfigManager::playBgMusicOnStartup() const
{
    return get("startup.playBgMusic", false).toBool();
}
void ConfigManager::setPlayBgMusicOnStartup(bool on)
{
    set("startup.playBgMusic", on);
}

QString ConfigManager::bgMusicPath() const
{
    return get("startup.bgMusicPath", "").toString();
}
void ConfigManager::setBgMusicPath(const QString& path)
{
    set("startup.bgMusicPath", path);
}

// ---- 练习 ----
int ConfigManager::openMode() const
{
    return get("practice.openMode", 0).toInt();
}
void ConfigManager::setOpenMode(int mode)
{
    set("practice.openMode", mode);
}

bool ConfigManager::countdownEnabled() const
{
    return get("practice.countdownEnabled", false).toBool();
}
void ConfigManager::setCountdownEnabled(bool on)
{
    set("practice.countdownEnabled", on);
}

int ConfigManager::countdownMinutes() const
{
    return get("practice.countdownMinutes", 5).toInt();
}
void ConfigManager::setCountdownMinutes(int minutes)
{
    set("practice.countdownMinutes", qBound(1, minutes, 60));
}

int ConfigManager::lastReadPosition(const QString& docName) const
{
    QJsonObject positions = get("recent.positions").toJsonObject();
    return positions.value(docName).toInt(0);
}

void ConfigManager::setLastReadPosition(const QString& docName, int pos)
{
    QJsonObject positions = get("recent.positions").toJsonObject();
    positions.insert(docName, pos);
    set("recent.positions", positions);
}