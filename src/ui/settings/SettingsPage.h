// src/ui/settings/SettingsPage.h
#pragma once

#include <QWidget>
#include <QString>

/// 设置页基类
///
/// 每个 Tab 一个子类，只负责：
///   - 把 ConfigManager 的值映射到控件（loadFromConfig）
///   - 把控件的值写回 ConfigManager（saveToConfig）
///   - 恢复默认（resetToDefault）
///
/// 不负责：
///   - 保存 JSON（由 SettingsDialog::onAccept 统一调 ConfigManager::save）
///   - 关闭对话框（由 SettingsDialog 控制）
class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr) : QWidget(parent) {}
    ~SettingsPage() override = default;

    /// Tab 标题
    virtual QString title() const = 0;

    /// ConfigManager → 控件
    virtual void loadFromConfig() = 0;

    /// 控件 → ConfigManager（不调用 save，由外层统一 save）
    virtual void saveToConfig() = 0;

    /// 恢复本页默认值（同时刷新控件）
    virtual void resetToDefault() = 0;

    /// 是否实时预览（true 时，控件变化会触发 changed 信号，
    /// SettingsDialog 会据此转发给 MainWindow 做预览）
    virtual bool isLivePreview() const { return false; }

signals:
    /// 控件发生变化（仅 isLivePreview()==true 时需要 emit）
    void changed();

    /// 请求"取消时恢复"的快照被外部读取（可选，本方案不用）
};