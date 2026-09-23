// src/ui/MusicLibraryDialog.cpp
#include "MusicLibraryDialog.h"

#include "app/ConfigManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMessageBox>

MusicLibraryDialog::MusicLibraryDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("音乐库"));
    resize(720, 480);
    setupUi();

    // 从配置加载
    auto& cfg = ConfigManager::instance();
    m_folderEdit->setText(cfg.musicFolder());
    QStringList files = cfg.musicFiles();

    // 如果配置里没有文件，但配置了文件夹，扫描一次
    if (files.isEmpty() && !cfg.musicFolder().isEmpty()) {
        scanFolder(cfg.musicFolder());
    } else {
        for (const QString& f : files) {
            if (QFileInfo::exists(f)) {
                auto* item = new QListWidgetItem(
                    QFileInfo(f).fileName(), m_listWidget);
                item->setData(Qt::UserRole, f);
            }
        }
        refreshList();
    }
}

void MusicLibraryDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // ---- 顶部：文件夹选择 ----
    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel(tr("文件夹:"), this));
    m_folderEdit = new QLineEdit(this);
    m_folderEdit->setReadOnly(true);
    m_folderEdit->setPlaceholderText(tr("（未选择）"));
    topRow->addWidget(m_folderEdit, 1);

    m_chooseFolderBtn = new QPushButton(tr("选择..."), this);
    m_refreshBtn = new QPushButton(tr("扫描"), this);
    topRow->addWidget(m_chooseFolderBtn);
    topRow->addWidget(m_refreshBtn);
    root->addLayout(topRow);

    // ---- 中部：列表 + 操作按钮 ----
    auto* midRow = new QHBoxLayout();

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listWidget->setAlternatingRowColors(true);
    midRow->addWidget(m_listWidget, 1);

    auto* btnCol = new QVBoxLayout();
    m_addBtn    = new QPushButton(tr("添加文件..."), this);
    m_removeBtn = new QPushButton(tr("移除选中"), this);
    m_clearBtn  = new QPushButton(tr("清空列表"), this);
    m_upBtn     = new QPushButton(tr("上移"), this);
    m_downBtn   = new QPushButton(tr("下移"), this);
    btnCol->addWidget(m_addBtn);
    btnCol->addWidget(m_removeBtn);
    btnCol->addWidget(m_clearBtn);
    btnCol->addSpacing(12);
    btnCol->addWidget(m_upBtn);
    btnCol->addWidget(m_downBtn);
    btnCol->addStretch();
    midRow->addLayout(btnCol);

    root->addLayout(midRow, 1);

    // ---- 底部：状态 + 按钮 ----
    auto* bottomRow = new QHBoxLayout();
    m_statusLabel = new QLabel(this);
    bottomRow->addWidget(m_statusLabel, 1);

    auto* cancelBtn = new QPushButton(tr("取消"), this);
    auto* okBtn     = new QPushButton(tr("确定"), this);
    okBtn->setDefault(true);
    bottomRow->addWidget(cancelBtn);
    bottomRow->addWidget(okBtn);
    root->addLayout(bottomRow);

    // ---- 连接 ----
    connect(m_chooseFolderBtn, &QPushButton::clicked,
            this, &MusicLibraryDialog::onChooseFolder);
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &MusicLibraryDialog::onRefresh);
    connect(m_addBtn, &QPushButton::clicked,
            this, &MusicLibraryDialog::onAddFile);
    connect(m_removeBtn, &QPushButton::clicked,
            this, &MusicLibraryDialog::onRemoveSelected);
    connect(m_clearBtn, &QPushButton::clicked,
            this, &MusicLibraryDialog::onClearAll);
    connect(m_upBtn, &QPushButton::clicked,
            this, &MusicLibraryDialog::onMoveUp);
    connect(m_downBtn, &QPushButton::clicked,
            this, &MusicLibraryDialog::onMoveDown);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &MusicLibraryDialog::onItemDoubleClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, this, &MusicLibraryDialog::onAccept);
}

// ---------------------------------------------------------------
// 文件夹扫描
// ---------------------------------------------------------------
void MusicLibraryDialog::onChooseFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("选择音乐文件夹"),
        m_folderEdit->text().isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
            : m_folderEdit->text());
    if (folder.isEmpty()) return;

    m_folderEdit->setText(folder);
    scanFolder(folder);
}

void MusicLibraryDialog::onRefresh()
{
    const QString folder = m_folderEdit->text();
    if (folder.isEmpty()) return;
    scanFolder(folder);
}

void MusicLibraryDialog::scanFolder(const QString& folder)
{
    QDir dir(folder);
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("扫描失败"),
            tr("文件夹不存在: %1").arg(folder));
        return;
    }

    // 递归扫描？这里先不递归，只扫一层
    const QStringList filters = {
        "*.mp3", "*.wav", "*.ogg", "*.flac", "*.m4a"
    };
    const auto entries = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    // 保留已有的项（避免重复）
    QStringList existing;
    for (int i = 0; i < m_listWidget->count(); ++i)
        existing << m_listWidget->item(i)->data(Qt::UserRole).toString();

    int added = 0;
    for (const QFileInfo& fi : entries) {
        const QString path = fi.absoluteFilePath();
        if (existing.contains(path)) continue;

        auto* item = new QListWidgetItem(fi.fileName(), m_listWidget);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        ++added;
    }

    refreshList();
    m_statusLabel->setText(tr("扫描完成，新增 %1 个文件").arg(added));
}

// ---------------------------------------------------------------
// 列表操作
// ---------------------------------------------------------------
void MusicLibraryDialog::onAddFile()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("添加音乐"),
        m_folderEdit->text().isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
            : m_folderEdit->text(),
        tr("音频文件 (*.mp3 *.wav *.ogg *.flac *.m4a);;所有文件 (*)"));

    for (const QString& f : files) {
        // 去重
        bool exists = false;
        for (int i = 0; i < m_listWidget->count(); ++i) {
            if (m_listWidget->item(i)->data(Qt::UserRole).toString() == f) {
                exists = true;
                break;
            }
        }
        if (exists) continue;

        auto* item = new QListWidgetItem(QFileInfo(f).fileName(), m_listWidget);
        item->setData(Qt::UserRole, f);
        item->setToolTip(f);
    }
    refreshList();
}

void MusicLibraryDialog::onRemoveSelected()
{
    const auto selected = m_listWidget->selectedItems();
    for (auto* item : selected) {
        delete m_listWidget->takeItem(m_listWidget->row(item));
    }
    refreshList();
}

void MusicLibraryDialog::onClearAll()
{
    if (m_listWidget->count() == 0) return;
    const auto ret = QMessageBox::question(this, tr("清空列表"),
        tr("确定清空所有音乐吗？"), QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    m_listWidget->clear();
    refreshList();
}

void MusicLibraryDialog::onMoveUp()
{
    int row = m_listWidget->currentRow();
    if (row <= 0) return;
    auto* item = m_listWidget->takeItem(row);
    m_listWidget->insertItem(row - 1, item);
    m_listWidget->setCurrentRow(row - 1);
}

void MusicLibraryDialog::onMoveDown()
{
    int row = m_listWidget->currentRow();
    if (row < 0 || row >= m_listWidget->count() - 1) return;
    auto* item = m_listWidget->takeItem(row);
    m_listWidget->insertItem(row + 1, item);
    m_listWidget->setCurrentRow(row + 1);
}

void MusicLibraryDialog::onItemDoubleClicked()
{
    auto* item = m_listWidget->currentItem();
    if (!item) return;
    emit playRequested(item->data(Qt::UserRole).toString());
}

// ---------------------------------------------------------------
// 状态与结果
// ---------------------------------------------------------------
void MusicLibraryDialog::refreshList()
{
    updateStatus();
}

void MusicLibraryDialog::updateStatus()
{
    m_statusLabel->setText(tr("共 %1 首").arg(m_listWidget->count()));
}

QStringList MusicLibraryDialog::playlist() const
{
    QStringList result;
    for (int i = 0; i < m_listWidget->count(); ++i)
        result << m_listWidget->item(i)->data(Qt::UserRole).toString();
    return result;
}

void MusicLibraryDialog::onAccept()
{
    // 保存到配置
    auto& cfg = ConfigManager::instance();
    cfg.setMusicFolder(m_folderEdit->text());
    cfg.setMusicFiles(playlist());
    cfg.save();
    accept();
}