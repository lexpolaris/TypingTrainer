// src/ui/MainWindow.h
#pragma once

#include <QMainWindow>
#include <QFont>

class QLabel;

class TextDocument;
class TypingSession;
class CodeTable;
class TypingView;
class CodeHintPanel;

class MenuBuilder;
class MusicController;
class TextLoaderController;
class CodeTableController;
class SpeedController;
class ThemeController;
class HistoryController;

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
    void onCodeHintRequested(QChar current, QChar next);
    void updateStats();
    void onRetry();                     // F3
    void onToggleShuffle(bool on);
    void onShowMistakes();              // 错字列表
    void onOpenMusicLibrary();

private:
    void setupUi();
    void setupControllers();
    void setupStatusBar();
    void connectControllers();
    void loadBuiltinCodeTable(const QString& name);
    void loadText(const QString& path);
    void loadResourceText(const QString& resPath);
    void loadWelcomeText();
    void switchMode(bool pacman);
    void startSessionForText(const QString& targetText, const QString& name);
    void ensureViewFocus();
    void onTextReady(const QString& content, int startIndex);

    int  currentParagraphIndex() const;
    void jumpToParagraph(int index);

    QString docName() const;

protected:
    void closeEvent(QCloseEvent* e) override;

    // 数据
    TypingSession* m_session = nullptr;
    TextDocument*  m_doc = nullptr;
    CodeTable*     m_codeTable = nullptr;

    // 控制器
    MenuBuilder*          m_menuBuilder = nullptr;
    MusicController*      m_music = nullptr;
    TextLoaderController* m_textLoader = nullptr;
    CodeTableController*  m_codeTables = nullptr;
    SpeedController*      m_speed = nullptr;
    ThemeController*      m_theme = nullptr;
    HistoryController*    m_history = nullptr;

    // UI
    TypingView*    m_view = nullptr;
    CodeHintPanel* m_codeHint = nullptr;
    QLabel*        m_statusSpeed = nullptr;
    QLabel*        m_statusKey = nullptr;
    QLabel*        m_statusCode = nullptr;
    QLabel*        m_statusProgress = nullptr;
    QLabel*        m_statusStats = nullptr;
    QLabel*        m_statusMistakes = nullptr;
};
