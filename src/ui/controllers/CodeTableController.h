// src/ui/controllers/CodeTableController.h
#pragma once

#include <QObject>
#include <QString>

class QWidget;
class CodeTable;

/// 码表控制器
///
/// 职责：加载内置码表 / 导入外部码表，并持久化自动加载路径。
/// 拥有 CodeTable 对象。
class CodeTableController : public QObject
{
    Q_OBJECT
public:
    explicit CodeTableController(QObject* parent = nullptr);
    ~CodeTableController() override;

    CodeTable* codeTable() const { return m_table; }

    /// 启动时按配置自动加载码表（返回是否成功加载）
    bool loadAutoTable();

public slots:
    /// 加载内置码表（名字：wubi86 / wubi98 / zhengma ...）
    void loadBuiltin(const QString& name);

    /// 弹出文件对话框导入外部码表；parent 作为对话框父窗口
    void importFromFile(QWidget* parent);

signals:
    /// 码表已变更（宿主需把它设置到视图与提示面板）
    void codeTableChanged(CodeTable* table);

    /// 状态栏提示信息
    void statusMessage(const QString& message, int timeoutMs);

    /// 加载失败提示（内置缺失等）
    void infoMessage(const QString& title, const QString& message);

private:
    CodeTable* m_table = nullptr;
};