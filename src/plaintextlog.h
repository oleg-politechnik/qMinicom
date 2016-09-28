#ifndef PLAINTEXTLOG_H
#define PLAINTEXTLOG_H

#include <QPlainTextEdit>

class PlainTextLog : public QPlainTextEdit
{
public:
    explicit PlainTextLog(QWidget *parent = Q_NULLPTR);

//protected:
//    void contextMenuEvent(QContextMenuEvent *event);

public slots:
    void appendTextNoNewline(const QString &text);
};

#endif // PLAINTEXTLOG_H
