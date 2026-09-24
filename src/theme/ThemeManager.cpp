// src/theme/ThemeManager.cpp
#include "ThemeManager.h"

#include <QApplication>
#include <QPalette>
#include <QStyleHints>
#include <QGuiApplication>
#include <QSettings>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    buildDefaultPalettes();

    // 监听系统主题变化（Qt 6.5+）
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme) {
        if (m_mode == System) {
            detectSystemMode();
            applyToApplication();
            emit themeChanged();
        }
    });
#endif

    detectSystemMode();
}

void ThemeManager::buildDefaultPalettes()
{
    // 亮色
    m_lightPalette = {
        {WindowBg,      QColor(0xF5, 0xF5, 0xF5)},
        {SurfaceBg,     QColor(0xFF, 0xFF, 0xFF)},
        {Border,        QColor(0xD0, 0xD0, 0xD0)},
        {TextPrimary,   QColor(0x21, 0x21, 0x21)},
        {TextSecondary, QColor(0x66, 0x66, 0x66)},
        {TextDisabled,  QColor(0x9E, 0x9E, 0x9E)},
        {Accent,        QColor(0x19, 0x76, 0xD2)},
        {AccentText,    QColor(0xFF, 0xFF, 0xFF)},
        {Typed,         QColor(0x9E, 0x9E, 0x9E)},
        {Current,       QColor(0x19, 0x76, 0xD2)},
        {Pending,       QColor(0x21, 0x21, 0x21)},
        {Error,         QColor(0xD3, 0x2F, 0x2F)},
        {Highlight,     QColor(0xFF, 0xF1, 0x76)},
        {SelectionBg,   QColor(0x19, 0x76, 0xD2)},
        {SelectionText, QColor(0xFF, 0xFF, 0xFF)},
    };

    // 暗色
    m_darkPalette = {
        {WindowBg,      QColor(0x1E, 0x1E, 0x1E)},
        {SurfaceBg,     QColor(0x2A, 0x2A, 0x2A)},
        {Border,        QColor(0x3C, 0x3C, 0x3C)},
        {TextPrimary,   QColor(0xE8, 0xE8, 0xE8)},
        {TextSecondary, QColor(0xB0, 0xB0, 0xB0)},
        {TextDisabled,  QColor(0x6E, 0x6E, 0x6E)},
        {Accent,        QColor(0x4F, 0x9E, 0xFF)},
        {AccentText,    QColor(0xFF, 0xFF, 0xFF)},
        {Typed,         QColor(0x5A, 0x5A, 0x5A)},
        {Current,       QColor(0x4F, 0x9E, 0xFF)},
        {Pending,       QColor(0xE8, 0xE8, 0xE8)},
        {Error,         QColor(0xFF, 0x6B, 0x6B)},
        {Highlight,     QColor(0x5C, 0x4A, 0x00)},
        {SelectionBg,   QColor(0x4F, 0x9E, 0xFF)},
        {SelectionText, QColor(0xFF, 0xFF, 0xFF)},
    };
}

void ThemeManager::detectSystemMode()
{
    bool dark = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    dark = (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
#else
    // 回退：根据系统调色板亮度判断
    const QPalette& pal = QApplication::palette();
    dark = pal.color(QPalette::Window).lightness() < 128;
#endif
    m_isDark = dark;
}

void ThemeManager::setMode(Mode m)
{
    if (m_mode == m) return;
    m_mode = m;
    if (m == System) detectSystemMode();
    else m_isDark = (m == Dark);
    applyToApplication();
    emit themeChanged();
}

bool ThemeManager::isDark() const
{
    return m_isDark;
}

QColor ThemeManager::color(Role role) const
{
    // 1. 自定义优先
    auto it = m_customColors.find(static_cast<int>(role));
    if (it != m_customColors.end())
        if (it.value().isValid())
            return it.value();
    // 2. 默认调色板
    const auto& pal = m_isDark ? m_darkPalette : m_lightPalette;
    return pal.value(static_cast<int>(role), Qt::magenta);
}

void ThemeManager::setCustomColor(Role role, const QColor& color)
{
    m_customColors.insert(static_cast<int>(role), color);
    emit themeChanged();
}

void ThemeManager::clearCustomColor(Role role)
{
    m_customColors.remove(static_cast<int>(role));
    emit themeChanged();
}

void ThemeManager::clearAllCustomColors()
{
    m_customColors.clear();
    emit themeChanged();
}

bool ThemeManager::hasCustomColor(Role role) const
{
    return m_customColors.contains(static_cast<int>(role));
}

void ThemeManager::applySystemPalette()
{
    // 让 Qt 使用系统原生调色板（Windows/macOS 原生风格）
    QApplication::setStyle(QApplication::style());
}

void ThemeManager::applyToApplication()
{
    // 1. 设置调色板（影响所有未显式设色的控件）
    QPalette pal;
    pal.setColor(QPalette::Window,          color(WindowBg));
    pal.setColor(QPalette::WindowText,      color(TextPrimary));
    pal.setColor(QPalette::Base,            color(SurfaceBg));
    pal.setColor(QPalette::AlternateBase,   color(WindowBg));
    pal.setColor(QPalette::Text,            color(TextPrimary));
    pal.setColor(QPalette::Button,          color(SurfaceBg));
    pal.setColor(QPalette::ButtonText,      color(TextPrimary));
    pal.setColor(QPalette::Highlight,       color(SelectionBg));
    pal.setColor(QPalette::HighlightedText, color(SelectionText));
    pal.setColor(QPalette::ToolTipBase,     color(SurfaceBg));
    pal.setColor(QPalette::ToolTipText,     color(TextPrimary));
    pal.setColor(QPalette::Disabled, QPalette::Text,       color(TextDisabled));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, color(TextDisabled));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, color(TextDisabled));

    qApp->setPalette(pal);

    // 2. 应用 QSS（控制更精细的样式）
    qApp->setStyleSheet(styleSheet());
}

QString ThemeManager::styleSheet() const
{
    auto c = [this](Role r) { return color(r).name(); };

    return QString(R"(
        QMainWindow, QDialog {
            background-color: %1;
            color: %2;
        }
        QMenuBar, QMenu, QToolBar {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
        }
        QMenuBar::item:selected, QMenu::item:selected {
            background-color: %5;
            color: %6;
        }
        QStatusBar {
            background-color: %3;
            color: %7;
            border-top: 1px solid %4;
        }
        QSplitter::handle {
            background-color: %4;
        }
        QPlainTextEdit, QTextEdit, QLineEdit {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
            selection-background-color: %5;
            selection-color: %6;
        }
        QPushButton {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
            padding: 4px 12px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: %5;
            color: %6;
        }
        QPushButton:pressed {
            background-color: %8;
        }
        QLabel {
            color: %2;
            background: transparent;
        }
        QGroupBox {
            border: 1px solid %4;
            border-radius: 3px;
            margin-top: 8px;
            color: %2;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 3px;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            background: %3;
            border: none;
        }
        QScrollBar::handle {
            background: %4;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::handle:hover {
            background: %5;
        }
    )")
        .arg(c(WindowBg))       // 1
        .arg(c(TextPrimary))    // 2
        .arg(c(SurfaceBg))      // 3
        .arg(c(Border))         // 4
        .arg(c(Accent))         // 5
        .arg(c(AccentText))     // 6
        .arg(c(TextSecondary))  // 7
        .arg(c(Highlight));     // 8
}

QString ThemeManager::roleName(Role role)
{
    switch (role) {
    case WindowBg:      return QObject::tr("窗口背景");
    case SurfaceBg:     return QObject::tr("面板背景");
    case Border:        return QObject::tr("边框");
    case TextPrimary:   return QObject::tr("主要文字");
    case TextSecondary: return QObject::tr("次要文字");
    case TextDisabled:  return QObject::tr("禁用文字");
    case Accent:        return QObject::tr("强调色");
    case AccentText:    return QObject::tr("强调文字");
    case Typed:         return QObject::tr("已打字符");
    case Current:       return QObject::tr("当前字符");
    case Pending:       return QObject::tr("未打字符");
    case Error:         return QObject::tr("错误字符");
    case Highlight:     return QObject::tr("高亮背景");
    case SelectionBg:   return QObject::tr("选中背景");
    case SelectionText: return QObject::tr("选中文字");
    }
    return {};
}

QVector<ThemeManager::Role> ThemeManager::allRoles()
{
    return {
        WindowBg, SurfaceBg, Border,
        TextPrimary, TextSecondary, TextDisabled,
        Accent, AccentText,
        Typed, Current, Pending, Error,
    };
}

void ThemeManager::setCustomColors(const QHash<int, QColor>& colors)
{
    m_customColors = colors;
    applyToApplication();
    emit themeChanged();
}