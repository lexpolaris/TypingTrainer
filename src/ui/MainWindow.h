// src/ui/MainWindow.h
#pragma once

#include <QMainWindow>

class TextDocument;
class TypingSession;
class CodeTable;
class TypingView;
class CodeHintPanel;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onOpenText();
    void onOpenTextLibrary();
    void onSwitchModePacman();
    void onSwitchModeTwoLine();
    void onImportCodeTable();
    void onSpeedPointSettings();
    void onShowSpeedChart();
    void onCodeHintRequested(QChar current, QChar next);
    void updateStats();
    void onOpenSettings();

private:
    void setupUi();
    void setupMenus();
    void setupStatusBar();
    void applyConfigToUi();
    void saveConfigFromUi();
    void loadBuiltinCodeTable(const QString& name);
    void loadText(const QString& path);
    void loadResourceText(const QString& resPath);
    void switchMode(bool pacman);
    void startSessionForText(const QString& targetText, const QString& name);
    void ensureViewFocus();

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

    void applyTypingFont(const QFont& f);
    QFont m_currentTypingFont;
};