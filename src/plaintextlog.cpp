#include "plaintextlog.h"

#include <QScrollBar>
#include <QDebug>
#include <QTextBlock>
#include <QCursor>
#include <QPainter>
#include <QRegExpValidator>

PlainTextLog::PlainTextLog(QWidget *parent) :
    QPlainTextEdit(parent),
    m_highlighter(new SearchHighlighter(this)),
    m_sideMarkScene(Q_NULLPTR),
    m_caret(QTextCursor(document()->lastBlock()))
{
    this->setFocusPolicy(Qt::StrongFocus); // helps catching 'Control' key events on macOS
}

void PlainTextLog::setSideMarkScene(QGraphicsScene *sideMarkScene)
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

    //qDebug() << "Warning" << __FUNCTION__ << "nothing found" << "(backward=" << backward << ")";
}

void PlainTextLog::sendVT100EscSeq(PlainTextLog::VT100EscapeCode code)
{
    QByteArray ba;
    ba.append(0x1B); // ESC
    ba.append('[');

    switch(code)
    {
    case VT100_EC_UP:
        ba.append('A');
        break;

    case VT100_EC_DOWN:
        ba.append('B');
        break;

    case VT100_EC_RIGHT:
        ba.append('C');
        break;

    case VT100_EC_LEFT:
        ba.append('D');
        break;

    default:
        break;
    }
    
    emit sendBytes(ba);
}

void PlainTextLog::findNext()
{
    find(false);
}

void PlainTextLog::findPrev()
{
    find(true);
}

void PlainTextLog::caretBackspace()
{
    if (m_caret.atBlockStart())
    {
        qDebug() << "Warning: attempt to cross the text block start boundary";
        return;
    }

    m_caret.deletePreviousChar();
}

void PlainTextLog::clear()
{
    m_escSeq.clear();

    QPlainTextEdit::clear();
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

void PlainTextLog::keyPressEvent(QKeyEvent *e)
{
    QString t = e->text();

    //qDebug() << e->key() << e->modifiers() << t;

    const Qt::KeyboardModifier ctrlKeyModifier =
#ifdef Q_OS_MAC
    Qt::MetaModifier
#else
    Qt::ControlModifier
#endif
    ;

    if (e->modifiers() == ctrlKeyModifier)
    {
        // for some reason, Qt5@macOS doesn't trigger this function upon Ctrl+<key> sequences
        //
        // solution: use Cmd+<key>

        if (e->key() >= 'A' && e->key() <= 'Z')
        {
            if (e->matches(QKeySequence::Copy)
                    || e->matches(QKeySequence::Paste)
                    || e->matches(QKeySequence::Cut)
                    || e->matches(QKeySequence::Find)
                    || e->matches(QKeySequence::FindNext)
                    || e->matches(QKeySequence::FindPrevious)
                    || e->matches(QKeySequence::SelectAll))
            {
                qDebug() << QString("Not processing ^%1 (standard action)").arg(QChar(e->key()).toUpper());
            }
            else
            {
                // (0x41) -> ^A -> 01
                // ..................
                // (0x5A) -> ^Z -> 26

                qDebug() << QString("sending ctrl-seq: ^%1").arg((char) e->key()).toUpper();
                char code = e->key() - 0x40;

                emit sendBytes(QByteArray().append(code));

                return; // we've handled that
            }
        }
    }
    else if (t.length() == 1)
    {
        QChar ch = t.at(0);

        //qDebug() << "char:" << (int) ch.toLatin1() << ch.toLatin1();

        if (e->key() == Qt::Key_Backspace)
        {
            //TODO add an option to send Ctrl+H instead
            //qDebug() << "bksp";
        }

        emit sendBytes(QByteArray().append(ch.toLatin1()));

        return; // we've handled that
    }
    else
    {
        switch (e->key())
        {
        case Qt::Key_Up:
            sendVT100EscSeq(VT100_EC_UP);
            return;

        case Qt::Key_Down:
            sendVT100EscSeq(VT100_EC_DOWN);
            return;

        case Qt::Key_Right:
            sendVT100EscSeq(VT100_EC_RIGHT);
            return;

        case Qt::Key_Left:
            sendVT100EscSeq(VT100_EC_LEFT);
            return;

        default:
            break;
        }
    }

    QPlainTextEdit::keyPressEvent(e);
}

void PlainTextLog::paintEvent(QPaintEvent *e)
{
    QPlainTextEdit::paintEvent(e);

    if (m_caret.block().isVisible())
    {
        QPainter p(viewport());
        p.fillRect(this->cursorRect(m_caret), QBrush(this->palette().color(QPalette::Text)));
    }
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

void PlainTextLog::appendBytes(const QByteArray &bytes)
{
    QString text;

    for (int i = 0; i < bytes.size(); ++i)
    {
        unsigned char c = bytes.at(i);

        if (!m_escSeq.isEmpty())
        {
            m_escSeq += c;

            QRegExp re("\\^\\[\\[(?:(K)|(?:(\\d)(?:;(\\d+))*)*(m))");

            QRegExpValidator v(re);

            int pos = 0;
            QValidator::State st = v.validate(m_escSeq, pos);

            qDebug() << "Checking" << m_escSeq << "with" << v.regExp().pattern() << "->" << st;

            if (st == QRegExpValidator::Invalid)
            {
                text += m_escSeq;
                qDebug() << "Warning: unknown" << m_escSeq << "sequence";
                m_escSeq.clear();
            }
            else if (st == QRegExpValidator::Acceptable)
            {
                if (!re.exactMatch(m_escSeq))
                {
                    qDebug() << "Error: " << m_escSeq << " is accepted by QRegExpValidator(" << v.regExp().pattern() << "), but rejected by QRegExp::exactMatch";
                }
                else
                {
                    QStringList list = re.capturedTexts();

                    QMutableListIterator<QString> i(list);
                    while (i.hasNext())
                    {
                        QString &s = i.next();
                        if (s.isEmpty())
                        {
                            i.remove();
                        }
                    }

                    qDebug() << "Detected" << m_escSeq << "sequence, args:" << list;

                    QString &cmd = list.last();

                    if (cmd == "K")
                    {
                        // clear eol
                        m_caret.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                        qDebug() << "Removing" << m_caret.selectedText();
                        m_caret.removeSelectedText();
                    }
                    else if (cmd == "m")
                    {
                        qDebug() << "Warning: m-sequence is not supported yet";
                    }
                    else
                    {
                        qDebug() << "Error: unknown sequence";
                    }

                }
                m_escSeq.clear();
            }
            else
            {
                // partial match -- wait for more
            }
        }
        else if (c >= 0x20)
        {
            text += c;
        }
        else
        {
            switch (c)
            {
            case '\0':
                break;

            case 0x1B:
                if (!m_escSeq.isEmpty())
                {
                    text += m_escSeq;
                    qDebug() << "Warning: aborted" << m_escSeq << "sequence";
                    m_escSeq.clear();
                }

                m_escSeq += "^[";
                break;

            case '\a':
                //qDebug() << "system bell";
                break;

            case '\n':
                text += c;
                break;

            case '\t':
                text += c;
                break;

            case '\b':
                caretBackspace();
                break;

            case '\r':
                break;

            default:
                text += QString("\\u00%1").arg(c, 2, 16, QChar('0'));
                break;
            }
        }
    }

    appendTextNoNewline(text);
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
