#include "plaintextlog.h"

#include <QScrollBar>
#include <QDebug>
#include <QTextBlock>
#include <QCursor>

PlainTextLog::PlainTextLog(QWidget *parent) :
    QPlainTextEdit(parent),
    m_highlighter(new SearchHighlighter(this)),
    m_sideMarkScene(Q_NULLPTR)
{
}

void PlainTextLog::attachSideMarkScene(QGraphicsScene *sideMarkScene)
{
    m_sideMarkScene = sideMarkScene;
}

LogBlockCustomData *PlainTextLog::highlightBlock(const QTextBlock &block)
{
    if (!m_sideMarkScene)
    {
        return Q_NULLPTR;
    }

    QGraphicsRectItem *rect = new QGraphicsRectItem();
    rect->setPen(QPen(Qt::darkYellow));
    rect->setBrush(QBrush(Qt::yellow));
    rect->setToolTip(tr("%1 (Line: %2)").arg(block.text()).arg(block.firstLineNumber()));

    resizeMark(rect, block);

    m_sideMarkScene->addItem(rect);

    //TODO rect->setCursor(Qt::PointingHandCursor);

    LogBlockCustomData *lbData = new LogBlockCustomData(rect);

    return lbData;
}

void PlainTextLog::setSearchPhrase(const QString &phrase, bool caseSensitive)
{
    m_sideMarkScene->clear();

    m_highlighter->setSearchPhrase(phrase, caseSensitive);

    m_sideMarkScene->setSceneRect(0,0,0,height()); // sync scene coordinates
}

void PlainTextLog::find(bool backward)
{
    QTextCursor cur = textCursor();
    QTextBlock currentBlock = cur.block();

    for (int i = 0; i < document()->blockCount(); ++i) // protection against eternal cycle
    {
        if (currentBlock.userData() && (!backward || (backward && !cur.atBlockStart()))) // todo store a search selection cursor list in UserData
        {
            //LogBlockCustomData *lbData = dynamic_cast<LogBlockCustomData*>(uData);
            //Q_ASSERT(lbData);

            int offset;

            if (backward)
            {
                int from = (cur.positionInBlock() + cur.anchor() - cur.position()) - currentBlock.text().length() - 1;
                offset = currentBlock.text().lastIndexOf(m_highlighter->searchPhrase(), from, m_highlighter->isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
            }
            else
            {
                int from = cur.positionInBlock();
                offset = currentBlock.text().indexOf(m_highlighter->searchPhrase(), from, m_highlighter->isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
            }

            if (offset >= 0)
            {
                cur.movePosition(QTextCursor::StartOfBlock);

                offset += cur.position();

                cur.setPosition(offset);
                cur.setPosition(offset + m_highlighter->searchPhrase().length(), QTextCursor::KeepAnchor);

                setTextCursor(cur);

                return;
            }
        }

        if (backward)
        {
            currentBlock = currentBlock.previous();
        }
        else
        {
            currentBlock = currentBlock.next();
        }

        if (!currentBlock.isValid())
        {
            if (backward)
            {
                currentBlock = document()->lastBlock();
            }
            else
            {
                currentBlock = document()->firstBlock();
            }
        }

        cur = QTextCursor(currentBlock);

        if (backward)
        {
            cur.movePosition(QTextCursor::EndOfBlock);
        }
    }

    qDebug() << "Warning" << __FUNCTION__ << "nothing found" << "(backward=" << backward << ")";
}

void PlainTextLog::findNext()
{
    find(false);
}

void PlainTextLog::findPrev()
{
    find(true);
}

void PlainTextLog::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QTextBlock currentBlock = document()->begin();
    while (currentBlock.isValid())
    {
        QTextBlockUserData *uData = currentBlock.userData();

        if (uData)
        {
            LogBlockCustomData *lbData = dynamic_cast<LogBlockCustomData*>(uData);
            Q_ASSERT(lbData);

            resizeMark(lbData->m_item, currentBlock);
        }
        currentBlock = currentBlock.next();
    }

    m_sideMarkScene->setSceneRect(0,0,0,height()); // sync scene coordinates
}

int PlainTextLog::getLastBlockBottom()
{
    const QTextBlock &lastBlock = document()->lastBlock();
    const QTextBlock &firstBlock = document()->firstBlock();
    return blockBoundingGeometry(lastBlock).bottom() - blockBoundingGeometry(firstBlock).top();
}

void PlainTextLog::resizeMark(QGraphicsRectItem *item, const QTextBlock &block)
{
    Q_ASSERT(item);

    const int marksToWidgetBorder = 1;
    const int markWidth = 8;
    const int markHeight = 4;

    int rootFrameHeight = getLastBlockBottom();
    int maxLine = document()->lineCount();
    int documentYProjection = qMin(height() - 2*marksToWidgetBorder, rootFrameHeight) - markHeight;

    //qDebug() << rootFrameHeight << maxLine << documentYProjection << height();

    qreal blockLine = block.firstLineNumber() + ((qreal) block.lineCount()) / 2;
    int markYOffset = qRound(((qreal) blockLine * documentYProjection) / maxLine);

    item->setRect(-markWidth/2, marksToWidgetBorder + markYOffset, markWidth, markHeight);

    //qDebug() << markYOffset << block.text();
}

//
// http://stackoverflow.com/a/13560475
//
void PlainTextLog::appendTextNoNewline(const QString &text)
{
    // scroll -- take old scrollbar value
    QScrollBar *p_scroll_bar = this->verticalScrollBar();
    bool bool_at_bottom = (p_scroll_bar->value() == p_scroll_bar->maximum());

    QTextCursor text_cursor = QTextCursor(this->document());
    text_cursor.beginEditBlock();
    {
        text_cursor.movePosition(QTextCursor::End);

        QStringList string_list = text.split('\n');

        for (int i = 0; i < string_list.size(); i++){
            text_cursor.insertText(string_list.at(i));
            if ((i + 1) < string_list.size()){
                text_cursor.insertBlock();
            }
        }
    }
    text_cursor.endEditBlock();

    if (bool_at_bottom)
    {
        p_scroll_bar->setValue(p_scroll_bar->maximum());
    }
}
