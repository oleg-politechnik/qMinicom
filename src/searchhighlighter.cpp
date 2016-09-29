#include "searchhighlighter.h"

SearchHighlighter::SearchHighlighter(QTextDocument *document) :
    QSyntaxHighlighter(document)
{
    m_document = document;
}

void SearchHighlighter::setSearchPhrase(const QString &phrase, bool caseSensitive)
{
    m_searchPhrase = phrase;
    m_isCaseSensitive = caseSensitive;

    rehighlight();
}

void SearchHighlighter::highlightBlock(const QString &text)
{
    if (!m_searchPhrase.isEmpty())
    {
        QTextCharFormat myClassFormat;
        myClassFormat.setFontWeight(QFont::Bold);
        myClassFormat.setBackground(QColor(0xFEF935));
        myClassFormat.setForeground(Qt::black);

        int index = text.indexOf(m_searchPhrase, 0, m_isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
        while (index >= 0) {
            int length = m_searchPhrase.length();
            setFormat(index, length, myClassFormat);
            index = text.indexOf(m_searchPhrase, index + length, m_isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
        }
    }
}