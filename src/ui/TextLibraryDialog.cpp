#include "TextLibraryDialog.h"

#include "utils/AppPaths.h"
#include "utils/TextLoader.h"
#include "theme/ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QFile>
#include <QStandardPaths>

TextLibraryDialog::TextLibraryDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("文本库"));
    resize(860, 560);
    setupUi();
    loadTextList();
}

void TextLibraryDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // ---- 顶部：搜索 + 刷新 ----
    auto* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel(tr("搜索:"), this));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("按文件名过滤..."));
    m_searchEdit->setClearButtonEnabled(true);
    topBar->addWidget(m_searchEdit, 1);
    auto* btnRefresh = new QPushButton(tr("刷新"), this);
    topBar->addWidget(btnRefresh);
    root->addLayout(topBar);

    // ---- 中部：列表 + 预览 ----
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_listWidget = new QListWidget(splitter);
    m_listWidget->setAlternatingRowColors(true);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    splitter->addWidget(m_listWidget);

    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    m_preview = new QTextEdit(rightPanel);
    m_preview->setReadOnly(true);
    m_preview->setPlaceholderText(tr("选择左侧文本以预览..."));
    rightLayout->addWidget(m_preview, 1);

    m_statusLabel = new QLabel(rightPanel);
    m_statusLabel->setWordWrap(true);
    rightLayout->addWidget(m_statusLabel);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    root->addWidget(splitter, 1);

    // ---- 底部：操作按钮 ----
    auto* bottomBar = new QHBoxLayout();
    m_btnImport = new QPushButton(tr("导入文本..."), this);
    m_btnRename = new QPushButton(tr("重命名"), this);
    m_btnDelete = new QPushButton(tr("删除"), this);
    m_btnOpen   = new QPushButton(tr("打开"), this);
    m_btnOpen->setDefault(true);

    bottomBar->addWidget(m_btnImport);
    bottomBar->addWidget(m_btnRename);
    bottomBar->addWidget(m_btnDelete);
    bottomBar->addStretch();
    bottomBar->addWidget(new QPushButton(tr("取消"), this));
    bottomBar->addWidget(m_btnOpen);
    root->addLayout(bottomBar);

    // ---- 连接 ----
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &TextLibraryDialog::onSearchChanged);
    connect(btnRefresh, &QPushButton::clicked,
            this, &TextLibraryDialog::onRefresh);
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &TextLibraryDialog::onSelectionChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &TextLibraryDialog::onItemDoubleClicked);
    connect(m_btnImport, &QPushButton::clicked,
            this, &TextLibraryDialog::onImport);
    connect(m_btnDelete, &QPushButton::clicked,
            this, &TextLibraryDialog::onDelete);
    connect(m_btnRename, &QPushButton::clicked,
            this, &TextLibraryDialog::onRename);
    connect(m_btnOpen, &QPushButton::clicked,
            this, &TextLibraryDialog::onOpen);

    // "取消"
    auto* cancelBtn = qobject_cast<QPushButton*>(
        bottomBar->itemAt(bottomBar->count() - 2)->widget());
    if (cancelBtn)
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    // 初始状态
    m_btnOpen->setEnabled(false);
    m_btnDelete->setEnabled(false);
    m_btnRename->setEnabled(false);
}

QString TextLibraryDialog::userTextDir() const
{
    return AppPaths::textDir();
}

// ---------------------------------------------------------------
// 加载列表
// ---------------------------------------------------------------
void TextLibraryDialog::loadTextList()
{
    m_listWidget->clear();

    // ---- 1. 内置文本（Qt 资源 :/texts/） ----
    auto* builtinHeader = new QListWidgetItem(tr("▸ 内置文本"), m_listWidget);
    builtinHeader->setFlags(Qt::NoItemFlags);
    builtinHeader->setForeground(ThemeManager::instance()
                                     .color(ThemeManager::TextSecondary));

    QDir resDir(":/texts");
    const auto resFiles = resDir.entryInfoList(QStringList() << "*.txt",
                                               QDir::Files, QDir::Name);
    for (const QFileInfo& fi : resFiles) {
        addTextItem(fi.fileName(), ":/texts/" + fi.fileName(), true);
    }
    if (resFiles.isEmpty()) {
        auto* empty = new QListWidgetItem(tr("   （无内置文本）"), m_listWidget);
        empty->setFlags(Qt::NoItemFlags);
    }

    // ---- 2. 用户文本（AppPaths::textDir()） ----
    auto* userHeader = new QListWidgetItem(tr("▸ 用户文本"), m_listWidget);
    userHeader->setFlags(Qt::NoItemFlags);
    userHeader->setForeground(ThemeManager::instance()
                                  .color(ThemeManager::TextSecondary));

    QDir userDir(userTextDir());
    const auto userFiles = userDir.entryInfoList(QStringList() << "*.txt",
                                                 QDir::Files,
                                                 QDir::Time);  // 按时间倒序
    for (const QFileInfo& fi : userFiles) {
        addTextItem(fi.fileName(), fi.absoluteFilePath(), false);
    }
    if (userFiles.isEmpty()) {
        auto* empty = new QListWidgetItem(tr("   （无用户文本，点击\"导入文本\"添加）"),
                                          m_listWidget);
        empty->setFlags(Qt::NoItemFlags);
    }

    m_statusLabel->setText(tr("内置 %1 个，用户 %2 个")
        .arg(resFiles.size()).arg(userFiles.size()));
}

void TextLibraryDialog::addTextItem(const QString& displayName,
                                    const QString& fullPath,
                                    bool isResource)
{
    auto* item = new QListWidgetItem(displayName, m_listWidget);
    item->setData(Qt::UserRole, fullPath);
    item->setData(Qt::UserRole + 1, isResource);

    // 用户文本附加字数与修改时间
    if (!isResource) {
        QFileInfo fi(fullPath);
        QString preview = TextLoader::loadFile(fullPath);
        int len = preview.length();
        item->setText(QString("%1  (%2 字 · %3)")
            .arg(displayName)
            .arg(len)
            .arg(fi.lastModified().toString("yyyy-MM-dd HH:mm")));
    }
    item->setToolTip(fullPath);
}

// ---------------------------------------------------------------
// 搜索过滤
// ---------------------------------------------------------------
void TextLibraryDialog::onSearchChanged(const QString& keyword)
{
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto* item = m_listWidget->item(i);
        // 跳过不可选中的分组标题/空提示
        if (item->flags() == Qt::NoItemFlags) {
            item->setHidden(false);
            continue;
        }
        const bool match = keyword.isEmpty() ||
                           item->text().contains(keyword, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
}

// ---------------------------------------------------------------
// 选中变化 → 更新预览
// ---------------------------------------------------------------
void TextLibraryDialog::onSelectionChanged()
{
    auto* item = m_listWidget->currentItem();
    if (!item || item->flags() == Qt::NoItemFlags) {
        m_preview->clear();
        m_statusLabel->clear();
        m_selectedPath.clear();
        m_selectedIsResource = false;
        m_btnOpen->setEnabled(false);
        m_btnDelete->setEnabled(false);
        m_btnRename->setEnabled(false);
        return;
    }

    m_selectedPath = item->data(Qt::UserRole).toString();
    m_selectedIsResource = item->data(Qt::UserRole + 1).toBool();

    updatePreview(m_selectedPath, m_selectedIsResource);

    m_btnOpen->setEnabled(true);
    m_btnDelete->setEnabled(!m_selectedIsResource);
    m_btnRename->setEnabled(!m_selectedIsResource);
}

void TextLibraryDialog::updatePreview(const QString& path, bool isResource)
{
    QString text = isResource ? TextLoader::loadResource(path)
                              : TextLoader::loadFile(path);
    if (text.isEmpty()) {
        m_preview->setPlainText(tr("（无法加载或文件为空）"));
        m_statusLabel->setText(tr("加载失败: %1").arg(path));
        return;
    }

    const int previewLen = qMin(500, text.length());
    m_preview->setPlainText(text.left(previewLen));

    // 统计
    int charCount = text.length();
    // 按 100 字/分估算跟打时间
    double estMinutes = charCount / 100.0;
    QString est = estMinutes < 1
                    ? tr("%1 秒").arg(int(estMinutes * 60))
                    : tr("%1 分钟").arg(estMinutes, 0, 'f', 1);

    m_statusLabel->setText(tr("字数: %1    预计跟打: %2（按 100 字/分）")
        .arg(charCount).arg(est));
}

// ---------------------------------------------------------------
// 双击 = 打开
// ---------------------------------------------------------------
void TextLibraryDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item || item->flags() == Qt::NoItemFlags) return;
    onSelectionChanged();
    onOpen();
}

// ---------------------------------------------------------------
// 导入
// ---------------------------------------------------------------
void TextLibraryDialog::onImport()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("导入文本"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        tr("文本文件 (*.txt);;所有文件 (*)"));

    if (paths.isEmpty()) return;

    int okCount = 0;
    QStringList failed;
    QDir destDir(userTextDir());

    for (const QString& src : paths) {
        QFileInfo fi(src);
        QString dest = destDir.absoluteFilePath(fi.fileName());

        // 重名时自动加序号
        if (QFile::exists(dest)) {
            QString base = fi.completeBaseName();
            QString ext = fi.suffix();
            int n = 1;
            do {
                dest = destDir.absoluteFilePath(
                    QString("%1 (%2).%3").arg(base).arg(n++).arg(ext));
            } while (QFile::exists(dest));
        }

        if (QFile::copy(src, dest)) {
            ++okCount;
        } else {
            failed << fi.fileName();
        }
    }

    loadTextList();

    if (!failed.isEmpty()) {
        QMessageBox::warning(this, tr("导入部分失败"),
            tr("成功 %1 个，失败 %2 个：\n%3")
                .arg(okCount).arg(failed.size())
                .arg(failed.join('\n')));
    } else {
        m_statusLabel->setText(tr("已导入 %1 个文件").arg(okCount));
    }
}

// ---------------------------------------------------------------
// 删除
// ---------------------------------------------------------------
void TextLibraryDialog::onDelete()
{
    if (m_selectedPath.isEmpty() || m_selectedIsResource) return;

    const auto ret = QMessageBox::question(this, tr("确认删除"),
        tr("确定要删除 \"%1\" 吗？此操作不可恢复。")
            .arg(QFileInfo(m_selectedPath).fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (QFile::remove(m_selectedPath)) {
        m_selectedPath.clear();
        m_preview->clear();
        loadTextList();
        m_statusLabel->setText(tr("已删除"));
    } else {
        QMessageBox::warning(this, tr("删除失败"),
            tr("无法删除文件: %1").arg(m_selectedPath));
    }
}

// ---------------------------------------------------------------
// 重命名
// ---------------------------------------------------------------
void TextLibraryDialog::onRename()
{
    if (m_selectedPath.isEmpty() || m_selectedIsResource) return;

    QFileInfo fi(m_selectedPath);
    bool ok = false;
    const QString newName = QInputDialog::getText(
        this, tr("重命名"),
        tr("新名称（不含扩展名）:"),
        QLineEdit::Normal, fi.completeBaseName(), &ok);
    if (!ok || newName.trimmed().isEmpty()) return;

    const QString newPath = fi.absolutePath() + "/"
                          + newName.trimmed() + "." + fi.suffix();

    if (QFile::exists(newPath)) {
        QMessageBox::warning(this, tr("重命名失败"),
            tr("目标文件已存在: %1").arg(newName));
        return;
    }
    if (!QFile::rename(m_selectedPath, newPath)) {
        QMessageBox::warning(this, tr("重命名失败"),
            tr("无法重命名文件: %1").arg(m_selectedPath));
        return;
    }

    m_selectedPath = newPath;
    loadTextList();
    m_statusLabel->setText(tr("已重命名为 %1").arg(newName));
}

// ---------------------------------------------------------------
// 打开 → 发信号给 MainWindow
// ---------------------------------------------------------------
void TextLibraryDialog::onOpen()
{
    if (m_selectedPath.isEmpty()) return;

    emit textChosen(m_selectedPath);
    accept();
}

// ---------------------------------------------------------------
// 刷新
// ---------------------------------------------------------------
void TextLibraryDialog::onRefresh()
{
    loadTextList();
    m_searchEdit->clear();
    m_preview->clear();
    m_selectedPath.clear();
    m_selectedIsResource = false;
    m_btnOpen->setEnabled(false);
    m_btnDelete->setEnabled(false);
    m_btnRename->setEnabled(false);
}