#include "SpeedPointDialog.h"

#include "core/SpeedPointFinder.h"
#include "app/ConfigManager.h"
#include "theme/ThemeManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

SpeedPointDialog::SpeedPointDialog(const QString& text, QWidget* parent)
    : QDialog(parent), m_text(text)
{
    setWindowTitle(tr("测速点设置"));
    resize(520, 560);
    setupUi();
    loadConfig();
    runFinder();
}

void SpeedPointDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // ---- 参数区 ----
    auto* paramBox = new QGroupBox(tr("寻找参数"), this);
    auto* grid = new QGridLayout(paramBox);

    grid->addWidget(new QLabel(tr("预设:"), paramBox), 0, 0);
    m_presetCombo = new QComboBox(paramBox);
    m_presetCombo->addItem(tr("中文冒号 ："), QStringLiteral("："));
    m_presetCombo->addItem(tr("英文冒号 :"),  QStringLiteral(":"));
    m_presetCombo->addItem(tr("句号 。"),     QStringLiteral("。"));
    m_presetCombo->addItem(tr("换行 \\n"),    QStringLiteral("\\n"));
    m_presetCombo->addItem(tr("自定义"),      QString());
    grid->addWidget(m_presetCombo, 0, 1);

    grid->addWidget(new QLabel(tr("标记:"), paramBox), 1, 0);
    m_markerEdit = new QLineEdit(QStringLiteral("："), paramBox);
    m_markerEdit->setPlaceholderText(tr("支持正则表达式"));
    grid->addWidget(m_markerEdit, 1, 1);

    grid->addWidget(new QLabel(tr("提取前 N 字:"), paramBox), 2, 0);
    m_prefixSpin = new QSpinBox(paramBox);
    m_prefixSpin->setRange(1, 10);
    m_prefixSpin->setValue(2);
    grid->addWidget(m_prefixSpin, 2, 1);

    grid->addWidget(new QLabel(tr("最多测速点:"), paramBox), 3, 0);
    m_maxSpin = new QSpinBox(paramBox);
    m_maxSpin->setRange(1, 10);
    m_maxSpin->setValue(10);
    grid->addWidget(m_maxSpin, 3, 1);

    m_btnRefresh = new QPushButton(tr("重新获取"), paramBox);
    grid->addWidget(m_btnRefresh, 4, 0, 1, 2);

    root->addWidget(paramBox);

    // ---- 候选区 ----
    auto* listBox = new QGroupBox(tr("候选测速点"), this);
    auto* listLayout = new QVBoxLayout(listBox);

    m_candidateList = new QListWidget(listBox);
    m_candidateList->setSelectionMode(QAbstractItemView::NoSelection);
    m_candidateList->setAlternatingRowColors(true);
    listLayout->addWidget(m_candidateList);

    m_statusLabel = new QLabel(tr("已选 0 / 10"), listBox);
    listLayout->addWidget(m_statusLabel);

    root->addWidget(listBox, 1);

    // ---- 按钮区 ----
    auto* bottomBar = new QHBoxLayout();
    bottomBar->addStretch();
    auto* btnCancel = new QPushButton(tr("取消"), this);
    m_btnOk = new QPushButton(tr("确定"), this);
    m_btnOk->setDefault(true);
    bottomBar->addWidget(btnCancel);
    bottomBar->addWidget(m_btnOk);
    root->addLayout(bottomBar);

    // ---- 连接 ----
    connect(m_presetCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        QString val = m_presetCombo->itemData(idx).toString();
        if (!val.isEmpty()) m_markerEdit->setText(val);
    });
    connect(m_btnRefresh, &QPushButton::clicked,
            this, &SpeedPointDialog::onRefresh);
    connect(m_candidateList, &QListWidget::itemChanged,
            this, &SpeedPointDialog::onSelectionChanged);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnOk, &QPushButton::clicked,
            this, &SpeedPointDialog::onAccept);
}

void SpeedPointDialog::loadConfig()
{
    auto& cfg = ConfigManager::instance();
    m_markerEdit->setText(cfg.get("speedPoints.marker", "：").toString());
    m_prefixSpin->setValue(cfg.get("speedPoints.prefixLength", 2).toInt());
    m_maxSpin->setValue(cfg.get("speedPoints.maxPoints", 10).toInt());
}

void SpeedPointDialog::saveConfig()
{
    auto& cfg = ConfigManager::instance();
    cfg.set("speedPoints.marker", m_markerEdit->text());
    cfg.set("speedPoints.prefixLength", m_prefixSpin->value());
    cfg.set("speedPoints.maxPoints", m_maxSpin->value());
    cfg.save();
}

void SpeedPointDialog::onRefresh()
{
    runFinder();
}

void SpeedPointDialog::runFinder()
{
    m_candidateList->clear();

    SpeedPointFinderConfig cfg;
    cfg.marker = m_markerEdit->text();
    if (cfg.marker.isEmpty()) cfg.marker = "：";
    // 处理 \n 转义
    cfg.marker.replace("\\n", "\n");
    cfg.prefixLength = m_prefixSpin->value();
    cfg.maxPoints = m_maxSpin->value();

    QVector<SpeedPointCandidate> candidates =
        SpeedPointFinder::find(m_text, cfg);

    if (candidates.isEmpty()) {
        auto* item = new QListWidgetItem(
            tr("（未找到任何候选点，请调整标记或参数）"), m_candidateList);
        item->setFlags(Qt::NoItemFlags);
        m_statusLabel->setText(tr("已选 0 / %1").arg(cfg.maxPoints));
        return;
    }

    for (const auto& c : candidates) {
        QString display = QString("%1  @ %2")
            .arg(c.prefix, -6)
            .arg(c.index);
        auto* item = new QListWidgetItem(display, m_candidateList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(c.autoChecked ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, c.index);
        item->setToolTip(tr("位置 %1：%2").arg(c.index).arg(c.prefix));
    }

    m_statusLabel->setText(tr("已选 %1 / %2")
        .arg(m_candidateList->count()).arg(cfg.maxPoints));
}

void SpeedPointDialog::onSelectionChanged()
{
    int checked = 0;
    for (int i = 0; i < m_candidateList->count(); ++i) {
        if (m_candidateList->item(i)->checkState() == Qt::Checked)
            ++checked;
    }

    int maxPts = m_maxSpin->value();
    m_statusLabel->setText(tr("已选 %1 / %2").arg(checked).arg(maxPts));

    // 超出上限时禁用未勾选项
    bool overLimit = checked >= maxPts;
    for (int i = 0; i < m_candidateList->count(); ++i) {
        auto* it = m_candidateList->item(i);
        if (it->flags() == Qt::NoItemFlags) continue;
        bool isChecked = (it->checkState() == Qt::Checked);
        it->setFlags(isChecked || !overLimit
                     ? (it->flags() | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable)
                     : (it->flags() & ~Qt::ItemIsEnabled));
    }

    m_btnOk->setEnabled(checked > 0);
}

void SpeedPointDialog::onAccept()
{
    m_selectedPoints.clear();
    for (int i = 0; i < m_candidateList->count(); ++i) {
        auto* it = m_candidateList->item(i);
        if (it->checkState() == Qt::Checked) {
            m_selectedPoints.append(it->data(Qt::UserRole).toInt());
        }
    }
    std::sort(m_selectedPoints.begin(), m_selectedPoints.end());
    saveConfig();
    accept();
}