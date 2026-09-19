#pragma once

#include <QDialog>
#include <QVector>

class QLineEdit;
class QSpinBox;
class QListWidget;
class QLabel;
class QPushButton;
class QComboBox;
struct SpeedPointFinderConfig;

class SpeedPointDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SpeedPointDialog(const QString& text, QWidget* parent = nullptr);

    /// 用户选中的测速点位置（升序）
    QVector<int> selectedPoints() const { return m_selectedPoints; }

private slots:
    void onRefresh();
    void onSelectionChanged();
    void onAccept();

private:
    void setupUi();
    void loadConfig();
    void saveConfig();
    void runFinder();

    QString m_text;
    QVector<int> m_selectedPoints;

    // UI
    QComboBox*   m_presetCombo = nullptr;   // 预设标记
    QLineEdit*   m_markerEdit = nullptr;
    QSpinBox*    m_prefixSpin = nullptr;
    QSpinBox*    m_maxSpin = nullptr;
    QPushButton* m_btnRefresh = nullptr;
    QListWidget* m_candidateList = nullptr;
    QLabel*      m_statusLabel = nullptr;
    QPushButton* m_btnOk = nullptr;
};