// src/ui/controllers/ThemeController.h
#pragma once

#include <QObject>
#include <QFont>
#include <QColor>
#include <QHash>

class QWidget;

/// 主题控制器
///
/// 职责：主题模式（跟随系统/亮/暗）、自定义颜色、打字字体。
/// 与 ThemeManager / ConfigManager 交互，并把字体应用于视图。
class ThemeController : public QObject
{
    Q_OBJECT
public:
    /// 与 ThemeManager::Mode 数值一致
    enum Mode { System = 0, Light = 1, Dark = 2 };

    explicit ThemeController(QObject* parent = nullptr);

    QFont typingFont() const { return m_typingFont; }

    /// 启动时把配置应用到 UI。hostDefaultFont 作为字体缺省来源（等价于原 QWidget::font()）
    void applyConfigToUi(const QFont& hostDefaultFont);

    /// 应用打字字体（更新内部记录并 emit fontChanged）
    void applyTypingFont(const QFont& f);

public slots:
    /// 打开设置对话框（实时预览）；parent 作为对话框父窗口
    void openSettings(QWidget* parent);

    /// 切换主题模式（0 系统 / 1 亮 / 2 暗）
    void setThemeMode(int mode);

    /// 保存主题模式到配置
    void saveThemeModeToConfig();

signals:
    /// 字体变化（宿主连到 view->setTypingFont）
    void fontChanged(const QFont& f);

    /// 主题模式变化（宿主同步菜单勾选）
    void themeModeChanged(int mode);

private:
    QFont m_typingFont;
};