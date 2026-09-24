// src/ui/controllers/TextLoaderController.h
#pragma once

#include <QObject>
#include <QString>

class TextDocument;

/// 文本加载控制器
///
/// 职责：从文件/资源/内置欢迎内容加载文本，并执行
///   1. 过滤（TextFilter）
///   2. 乱序（TextShuffler，可选）
///   3. 载入 TextDocument
///   4. 依据 openMode 计算起始位置
///
/// 加载完成后通过 textReady 通知宿主，由宿主设置测速点模式、
/// 倒计时并真正启动会话（这些依赖视图/会话类型，属于宿主职责）。
///
/// 拥有 TextDocument（宿主可通过 document() 获取）。
class TextLoaderController : public QObject
{
    Q_OBJECT
public:
    explicit TextLoaderController(QObject* parent = nullptr);

    TextDocument* document() const { return m_doc; }

    /// 是否处于乱序模式
    bool shuffleMode() const { return m_shuffleMode; }
    void setShuffleMode(bool on) { m_shuffleMode = on; }

    /// 当前文档的稳定键（文件绝对路径 / 资源路径；欢迎页为空）
    QString docKey() const { return m_docKey; }

    /// 当前文档显示名
    QString docName() const { return m_docName; }

public slots:
    /// 打开本地文件（含错误弹窗由宿主负责 → 通过 loadFailed 通知）
    void loadFile(const QString& path);

    /// 打开 Qt 资源文本
    void loadResource(const QString& resPath);

    /// 加载内置欢迎内容
    void loadWelcome();

    /// 切换乱序模式后重载当前文本
    void reload();

signals:
    /// 文本已就绪（过滤/乱序/载入完成），宿主据此启动会话
    /// @param content   处理后的最终文本
    /// @param startIndex 起始位置（由 openMode 决定）
    void textReady(const QString& content, int startIndex);

    /// 加载失败（文件读取错误等），宿主负责提示
    void loadFailed(const QString& message);

    /// 最近一次成功加载的文本路径（用于持久化 lastTextPath）
    void textPathChanged(const QString& path);

private:
    /// 共享的处理流程
    void processAndEmit(const QString& rawText, const QString& name,
                        const QString& key);

    TextDocument* m_doc = nullptr;

    bool    m_shuffleMode = false;
    QString m_originalText;   // 未乱序的原始文本
    QString m_docName;
    QString m_docKey;
};
