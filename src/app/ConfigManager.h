// src/app/ConfigManager.h
#pragma once
#include <QObject>
#include <QJsonObject>
#include <QVariant>
#include <QString>
#include <QColor>
#include <QFont>
#include <QStringList>

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

    // ---- 码表 ----
    /// 启动时自动加载的码表路径（空表示不自动加载）
    QString currentCodeTable() const;
    QString autoLoadCodeTablePath() const;
    void setAutoLoadCodeTablePath(const QString& path);

    // ---- 启动 ----
    bool    loadLastTextOnStartup() const;
    void    setLoadLastTextOnStartup(bool on);
    QString lastTextPath() const;
    void    setLastTextPath(const QString& path);

    bool    playBgMusicOnStartup() const;
    void    setPlayBgMusicOnStartup(bool on);
    QString     musicFolder() const;
    void        setMusicFolder(const QString& folder);
    QStringList musicFiles() const;
    void        setMusicFiles(const QStringList& files);
    int         musicCurrentIndex() const;
    void        setMusicCurrentIndex(int idx);
    int         musicVolume() const;         // 0..100
    void        setMusicVolume(int v);
    int         musicLoopMode() const;       // 0=NoLoop 1=LoopAll 2=LoopOne
    void        setMusicLoopMode(int mode);

    // ---- 练习 ----
    int     openMode() const;                        // 0从头 1随机 2断点
    void    setOpenMode(int mode);

    bool    countdownEnabled() const;
    void    setCountdownEnabled(bool on);
    int     countdownMinutes() const;
    void    setCountdownMinutes(int minutes);

    // ---- 断点续打 ----
    int lastReadPosition(const QString& docName) const;
    void setLastReadPosition(const QString& docName, int pos);

    // ---- 过滤 ----
    FilterOptions filterOptions() const;
    void setFilterOptions(const FilterOptions& opt);

private:
    explicit ConfigManager(QObject* parent = nullptr);
    Q_DISABLE_COPY(ConfigManager)

    QJsonObject m_root;
};
