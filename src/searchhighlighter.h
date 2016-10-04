#ifndef SEARCHHIGHLIGHTER_H
#define SEARCHHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QObject>

class PlainTextLog;

class SearchHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SearchHighlighter(PlainTextLog *textLog);

    void setSearchPhrase(const QString &phrase, bool caseSensitive);

    void highlightBlock(const QString &text);

    QString searchPhrase() const;
    bool isCaseSensitive() const;

private:
    QString m_searchPhrase;
    bool m_isCaseSensitive;

    PlainTextLog *m_textLog;
};

#endif // SEARCHHIGHLIGHTER_H
