#ifndef PLAINTEXTLOG_H
#define PLAINTEXTLOG_H

#include "logblockcustomdata.h"
#include "searchhighlighter.h"

#include <QPlainTextEdit>
#include <QObject>
#include <QGraphicsScene>
#include <QRegExpValidator>
#include <QTextDecoder>

class PlainTextLog : public QPlainTextEdit
{
    Q_OBJECT

public:
    enum VT100EscapeCode
    {
        VT100_EC_ESC,
        VT100_EC_UP,
        VT100_EC_DOWN,
        VT100_EC_RIGHT,
        VT100_EC_LEFT,
        VT100_EC_FN_START,
        VT100_EC_F1 = VT100_EC_FN_START,
        VT100_EC_F2,
        VT100_EC_F3,
        VT100_EC_F4,
        VT100_EC_F5,
        VT100_EC_F6,
        VT100_EC_F7,
        VT100_EC_F8,
        VT100_EC_F9,
        VT100_EC_F10,
        VT100_EC_FN_END = VT100_EC_F10,
        VT100_EC_DA_VT102
    };

    enum AnsiColor
    {
        ANSI_BLACK,
        ANSI_RED,
        ANSI_GREEN,
        ANSI_YELLOW,
        ANSI_BLUE,
        ANSI_MAGENTA,
        ANSI_CYAN,
        ANSI_WHITE
    };

    explicit PlainTextLog(QWidget *parent = 0);

    const int terminalScreenWidth = 80;
    const int terminalScreenHeight = 24;

    void setSideMarkScene(QGraphicsScene *sideMarkScene);
    LogBlockCustomData *highlightBlock(const QTextBlock &block);

    void setContextMenuTextCursor(const QTextCursor &cur);

signals:
    void sendBytes(const QByteArray &bytes);

public slots:
    void appendBytes(const QByteArray &bytes);
    //void appendTextNoNewline(const QString &text);
    void setSearchPhrase(const QString &phrase, bool caseSensitive);
    void findNext();
    void findPrev();
    void find(bool backward);
    void clear();
    void clearToCurrentContextMenuLine();

protected:
    void resizeEvent(QResizeEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    int getLastBlockBottom();
    void resizeMark(QGraphicsRectItem *item, const QTextBlock &block);
    void sendVT100EscSeq(VT100EscapeCode code);
    void moveCaretToTheStartOfTheScreen();
    void moveCaretToTheRightBy(int chars);
    void moveCaretToTheLeftBy(int chars);
    void moveCaretToTheFirstColumn();
    void moveCaretUpwardsBy(int lines);
    void moveCaretDownwardsBy(int lines);
    void insertTextAtCaret(const QString &text);
    void caretBackspace();
    void setCaretBrightness(bool bright);
    void setCaretInvertedColors(bool invert);
    void setCaretUnderline(bool underline);
    void setCaretForeground(AnsiColor ansiColor);
    void setCaretBackround(AnsiColor ansiColor);
    bool setScrollingRegion(int top, int bottom);
    void resetCaretAttributes();
    void updateCaretAttributes();
    void resetTextDecoder();
    QRgb ansiColorToRgb(AnsiColor ansiColor, bool isBright);

    SearchHighlighter *m_highlighter;
    QGraphicsScene *m_sideMarkScene;
    QString m_escSeq;
    QTextDecoder *m_decoder;
    QTextCursor m_caret;
    QTextCursor m_contextMenuTextCursor;
    QTextCharFormat m_tcfm;
    AnsiColor m_caretFgColor;
    AnsiColor m_caretBgColor;
    bool m_caretBrightness;
    bool m_caretUnderline;
    bool m_caretInvertedColors;
    bool m_cursorMode;
    bool m_cursorRelativeCoordinates;
    int m_screenTopLine;
    QRegularExpressionValidator *m_vt100escReV;

    int m_screenScrollingRegionTop;
    int m_screenScrollingRegionBottom;
};

#endif // PLAINTEXTLOG_H
