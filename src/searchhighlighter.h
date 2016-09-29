#ifndef SEARCHHIGHLIGHTER_H
#define SEARCHHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QObject>

class SearchHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SearchHighlighter(QTextDocument *document);

    void setSearchPhrase(const QString &phrase, bool caseSensitive);

    void highlightBlock(const QString &text);

private:
    QString m_searchPhrase;
    bool m_isCaseSensitive;

    QTextDocument *m_document;
};

#endif // SEARCHHIGHLIGHTER_H
