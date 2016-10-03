#include "plaintextlog.h"
#include "searchhighlighter.h"

#include <QDebug>

SearchHighlighter::SearchHighlighter(PlainTextLog *textLog) :
    QSyntaxHighlighter(textLog->document())
{
    m_textLog = textLog;
}

void SearchHighlighter::setSearchPhrase(const QString &phrase, bool caseSensitive)
{
    m_searchPhrase = phrase;
    m_isCaseSensitive = caseSensitive;
    rehighlight();
}

void SearchHighlighter::highlightBlock(const QString &text)
{
    QTextBlockUserData *data = Q_NULLPTR;

    if (!m_searchPhrase.isEmpty())
    {
        QTextCharFormat myClassFormat;
        myClassFormat.setFontWeight(QFont::Bold);
        myClassFormat.setBackground(QColor(0xFEF935));
        myClassFormat.setForeground(Qt::black);

        int index = text.indexOf(m_searchPhrase, 0, m_isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
        if (index >= 0)
        {
            data = m_textLog->highlightBlock(this->currentBlock());

            while (index >= 0) {
                int length = m_searchPhrase.length();
                setFormat(index, length, myClassFormat);
                index = text.indexOf(m_searchPhrase, index + length, m_isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
            }
        }

    }

    setCurrentBlockUserData(data);
}

QString SearchHighlighter::searchPhrase() const
{
    return m_searchPhrase;
}

bool SearchHighlighter::isCaseSensitive() const
{
    return m_isCaseSensitive;
}
