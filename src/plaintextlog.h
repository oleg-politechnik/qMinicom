#ifndef PLAINTEXTLOG_H
#define PLAINTEXTLOG_H

#include <QPlainTextEdit>
#include <QObject>

class PlainTextLog : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextLog(QWidget *parent = 0);

public slots:
    void appendTextNoNewline(const QString &text);
};

#endif // PLAINTEXTLOG_H
