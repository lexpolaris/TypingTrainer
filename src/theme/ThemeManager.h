// src/theme/ThemeManager.h
#pragma once

#include <QObject>
#include <QColor>
#include <QHash>
#include <QString>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum Mode {
        System,     // 跟随系统
        Light,      // 强制亮色
        Dark        // 强制暗色
    };

    // 颜色角色（可扩展）
    enum Role {
        WindowBg,        // 窗口背景
        SurfaceBg,       // 面板/卡片背景
        Border,          // 边框
        TextPrimary,     // 主要文字
        TextSecondary,   // 次要文字
        TextDisabled,    // 禁用文字
        Accent,          // 强调色（按钮、选中）
        AccentText,      // 强调色上的文字
        Typed,           // 已打过的字符
        Current,         // 当前字符
        Pending,         // 未打的字符
        Error,           // 错误字符
        Pacman,          // 吃豆人颜色
        Highlight,       // 高亮背景
        SelectionBg,     // 选中背景
        SelectionText    // 选中文字
    };
    Q_ENUM(Role)

    static ThemeManager& instance();

    // 模式
    Mode mode() const { return m_mode; }
    void setMode(Mode m);
    bool isDark() const;

    // 取色（核心接口）
    QColor color(Role role) const;

    // ---- 自定义覆盖 ----
    void setCustomColor(Role role, const QColor& color);
    void clearCustomColor(Role role);
    void clearAllCustomColors();
    bool hasCustomColor(Role role) const;

    /// 返回当前自定义颜色表（用于持久化）
    QHash<int, QColor> customColors() const { return m_customColors; }

    /// 从持久化数据恢复（启动时调用）
    void setCustomColors(const QHash<int, QColor>& colors);

    /// 角色的可读名称（UI 显示）
    static QString roleName(Role role);

    /// 角色列表（UI 遍历）
    static QVector<Role> allRoles();

    QString styleSheet() const;
    void applyToApplication();

signals:
    void themeChanged();

private:
    explicit ThemeManager(QObject* parent = nullptr);
    Q_DISABLE_COPY(ThemeManager)

    void detectSystemMode();
    void buildDefaultPalettes();
    void applySystemPalette();

    Mode m_mode = System;
    bool m_isDark = false;
    QHash<int, QColor> m_customColors;

    QHash<int, QColor> m_lightPalette;
    QHash<int, QColor> m_darkPalette;
};
