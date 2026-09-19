// src/ui/MainWindow.h
#pragma once
#include <QMainWindow>

class TextDocument;
class TypingSession;
class CodeTable;
class TypingView;
class CodeHintPanel;
class QLabel;
class QComboBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void inputMethodEvent(QInputMethodEvent* e) override;

private slots:
    void onOpenText();
    void onSwitchMode(int index);
    void onLoadCodeTable(int index);
    void onImportCodeTable();
    void onThemeModeChanged(int index);
    void onCodeHintRequested(QChar current, QChar next);
    void updateStats();
    void onOpenTextLibrary();
    void onSpeedPointSettings();
    void onShowSpeedChart();

private:
    void setupUi();
    void setupMenus();
    void setupStatusBar();
    void applyConfigToUi();
    void saveConfigFromUi();
    void loadBuiltinCodeTable(const QString& name);
    void loadText(const QString& path);
    void loadResourceText(const QString& resPath);
    void startSessionForText(const QString& targetText, const QString& name);

    // 数据
    TextDocument*  m_doc = nullptr;
    TypingSession* m_session = nullptr;
    CodeTable*     m_codeTable = nullptr;

    // UI
    TypingView*    m_view = nullptr;
    CodeHintPanel* m_codeHint = nullptr;
    QLabel*        m_statusSpeed = nullptr;
    QLabel*        m_statusKey = nullptr;
    QLabel*        m_statusCode = nullptr;
    QLabel*        m_statusProgress = nullptr;
    QComboBox*     m_modeCombo = nullptr;
};
