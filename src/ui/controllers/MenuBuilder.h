// src/ui/controllers/MenuBuilder.h
#pragma once

#include <QObject>
#include <QHash>
#include <QColor>

class QMainWindow;
class QAction;
class QActionGroup;

/// 菜单栏构建器
///
/// 职责：构建 MainWindow 的整个菜单栏（文件/模式/跟打/码表/测速/主题/音乐）。
/// 不持有任何业务对象，所有用户操作通过信号对外广播，由宿主连接。
///
/// 主题菜单的勾选状态通过 setThemeMode(int) 由宿主同步。
class MenuBuilder : public QObject
{
    Q_OBJECT
public:
    /// 主题模式（与 ThemeManager::Mode 数值一致）
    enum ThemeMode { ThemeSystem = 0, ThemeLight = 1, ThemeDark = 2 };

    explicit MenuBuilder(QMainWindow* window, QObject* parent = nullptr);

    /// 构建菜单栏
    void build();

    /// 同步主题菜单勾选状态
    void setThemeMode(ThemeMode mode);

signals:
    // ---- 文件 ----
    void openTextRequested();
    void openTextLibraryRequested();
    void openSettingsRequested();

    // ---- 模式 ----
    void modePacmanRequested();
    void modeTwoLineRequested();

    // ---- 跟打 ----
    void retryRequested();
    void shuffleToggled(bool on);
    void showMistakesRequested();
    void showHistoryRequested();

    // ---- 码表 ----
    void builtinCodeTableRequested(const QString& name);
    void importCodeTableRequested();

    // ---- 测速 ----
    void speedPointSettingsRequested();
    void showSpeedChartRequested();

    // ---- 主题 ----
    void themeModeRequested(ThemeMode mode);
    void customCurrentColorRequested();
    void clearCustomColorsRequested();

    // ---- 音乐 ----
    void openMusicLibraryRequested();
    void playPauseMusicRequested();
    void nextMusicRequested();
    void prevMusicRequested();

private:
    QMainWindow* m_window = nullptr;

    QAction* m_actThemeSystem = nullptr;
    QAction* m_actThemeLight = nullptr;
    QAction* m_actThemeDark = nullptr;
};
