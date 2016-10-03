#ifndef PLAINTEXTLOG_H
#define PLAINTEXTLOG_H

#include "logblockcustomdata.h"
#include "searchhighlighter.h"

#include <QPlainTextEdit>
#include <QObject>
#include <QGraphicsScene>

class PlainTextLog : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextLog(QWidget *parent = 0);

    void attachSideMarkScene(QGraphicsScene *sideMarkScene);

    LogBlockCustomData *highlightBlock(const QTextBlock &block);

public slots:
    void appendTextNoNewline(const QString &text);
    void setSearchPhrase(const QString &phrase, bool caseSensitive);
    void findNext();
    void findPrev();

protected:
    void resizeEvent(QResizeEvent *e);

private:
    int getLastBlockBottom();
    void resizeMark(QGraphicsRectItem *item, const QTextBlock &block);
    void find(bool backward);

    SearchHighlighter *m_highlighter;
    QGraphicsScene *m_sideMarkScene;
};

#endif // PLAINTEXTLOG_H
