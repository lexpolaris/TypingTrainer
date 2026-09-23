// src/app/ConfigManager.h
#pragma once
#include <QObject>
#include <QJsonObject>
#include <QVariant>
#include <QString>
#include <QColor>
#include <QFont>

struct FilterOptions;

class ConfigManager : public QObject
{
    Q_OBJECT
public:
    static ConfigManager& instance();

    void load();
    void save();

    QVariant get(const QString& key, const QVariant& defaultValue = {}) const;
    void set(const QString& key, const QVariant& value);

    // ---- 主题 ----
    QString themeMode() const;
    QHash<int, QColor> customThemeColors() const;
    void setCustomThemeColors(const QHash<int, QColor>& colors);

    // ---- 字体 ----
    QFont typingFont() const;
    void setTypingFont(const QFont& f);
    int     fontPointSize() const;
    QString fontFamily() const;

    // ---- 其他 ----
    QString lastTextPath() const;
    QString currentCodeTable() const;

    // ---- 码表 ----
    /// 启动时自动加载的码表路径（空表示不自动加载）
    QString autoLoadCodeTablePath() const;
    void setAutoLoadCodeTablePath(const QString& path);

    // ---- 过滤 ----
    FilterOptions filterOptions() const;
    void setFilterOptions(const FilterOptions& opt);

private:
    explicit ConfigManager(QObject* parent = nullptr);
    Q_DISABLE_COPY(ConfigManager)

    QJsonObject m_root;
};
