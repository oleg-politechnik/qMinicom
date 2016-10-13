#ifndef PLAINTEXTLOG_H
#define PLAINTEXTLOG_H

#include "logblockcustomdata.h"
#include "searchhighlighter.h"

#include <QPlainTextEdit>
#include <QObject>
#include <QGraphicsScene>
#include <QRegExpValidator>

class PlainTextLog : public QPlainTextEdit
{
    Q_OBJECT

public:
    enum VT100EscapeCode
    {
        VT100_EC_UP,
        VT100_EC_DOWN,
        VT100_EC_RIGHT,
        VT100_EC_LEFT
    };

    explicit PlainTextLog(QWidget *parent = 0);

    void setSideMarkScene(QGraphicsScene *sideMarkScene);
    LogBlockCustomData *highlightBlock(const QTextBlock &block);

signals:
    void sendBytes(const QByteArray &bytes);

public slots:
    void appendBytes(const QByteArray &bytes);
    void appendTextNoNewline(const QString &text);
    void setSearchPhrase(const QString &phrase, bool caseSensitive);
    void findNext();
    void findPrev();
    void caretBackspace();
    void clear();

protected:
    void resizeEvent(QResizeEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    int getLastBlockBottom();
    void resizeMark(QGraphicsRectItem *item, const QTextBlock &block);
    void find(bool backward);
    void sendVT100EscSeq(VT100EscapeCode code);

    SearchHighlighter *m_highlighter;
    QGraphicsScene *m_sideMarkScene;
    QString m_escSeq;
    QTextCursor m_caret;
};

#endif // PLAINTEXTLOG_H
