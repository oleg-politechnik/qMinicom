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
    m_decoder(Q_NULLPTR),
    m_caret(QTextCursor(document()->lastBlock()))
{
    const QString rx = "\\^\\[(?:\\[(?:(\\d*)(H)|(?:(\\d*)(?:;(\\d*))*)(m)|(?:(r))|(?:(\\d+);(\\d+)([rfH]))|(?:(\\d*)([ABCD]))|(?:([012]*)([JK]))|(?:\\?(\\d+)([hl]))|(?:([0]?)(c)))|([=>])|([()][0B])|([78M])|(#8))(.*)";

    m_vt100escReV = new QRegularExpressionValidator(QRegularExpression(rx), this);

    this->setFocusPolicy(Qt::StrongFocus); // helps catching 'Control' key events on macOS
    this->setUndoRedoEnabled(false);
    this->setLineWrapMode(QPlainTextEdit::NoWrap);
    this->setReadOnly(true);
    this->setTextInteractionFlags(Qt::TextSelectableByMouse);

    clear();

    QPalette palette = this->palette();
    palette.setColor(QPalette::Text, m_caret.blockCharFormat().foreground().color());
    palette.setColor(QPalette::Base, m_caret.blockCharFormat().background().color());
    setPalette(palette);

    // tests

    //appendBytes("\x1B[2J@\x1B[12B@\x1B[12C@\x1B[2J");
    //appendBytes("\x1B[2J@\x1B[D\x1B[12B@\x1B[D\x1B[12C@\x1B[D\x1B[6B$\x1B[D\x1B[0J\x1B[6A\x1B[0J");
    //appendBytes("\x1B[2J@\x1B[D\x1B[12B@\x1B[D\x1B[12C@\x1B[D\x1B[D\x1B[1J");
    //appendBytes("\x1B[H\x1B#8\x1B[2J");
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

void PlainTextLog::setContextMenuTextCursor(const QTextCursor &cur)
{
    m_contextMenuTextCursor = cur;
}

void PlainTextLog::setSearchPhrase(const QString &phrase, bool caseSensitive)
{
    if (m_sideMarkScene)
    {
        m_sideMarkScene->clear();
    }

    m_highlighter->setSearchPhrase(phrase, caseSensitive);

    if (m_sideMarkScene)
    {
        m_sideMarkScene->setSceneRect(0,0,0,height()); // sync scene coordinates
    }
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
    if (code >= VT100_EC_FN_START && code <= VT100_EC_FN_END && !m_cursorMode)
    {
        qDebug() << "Error:" << code << "is not supported for Cursor Mode = Cursor";
        return;
    }

    QByteArray ba;
    ba.append(0x1B); // <ESC>

    switch(code)
    {
    case VT100_EC_DA_VT102:
        ba.append('[');
        ba.append('?');
        ba.append('6');
        ba.append('c');
        break;

    case VT100_EC_ESC:
        ba.append('[');
        break;

    case VT100_EC_UP:
        ba.append(m_cursorMode ? 'O' : '[');
        ba.append('A');
        break;

    case VT100_EC_DOWN:
        ba.append(m_cursorMode ? 'O' : '[');
        ba.append('B');
        break;

    case VT100_EC_RIGHT:
        ba.append(m_cursorMode ? 'O' : '[');
        ba.append('C');
        break;

    case VT100_EC_LEFT:
        ba.append(m_cursorMode ? 'O' : '[');
        ba.append('D');
        break;

    case VT100_EC_F1:
        ba.append('O');
        ba.append('P');
        break;

    case VT100_EC_F2:
        ba.append('O');
        ba.append('Q');
        break;

    case VT100_EC_F3:
        ba.append('O');
        ba.append('R');
        break;

    case VT100_EC_F4:
        ba.append('O');
        ba.append('S');
        break;

    default:
        {
            qDebug() << "Error: unknown VT100EscapeCode" << code;
            return;
        }
        break;
    }

    emit sendBytes(ba);
}

void PlainTextLog::moveCaretToTheStartOfTheScreen()
{
    QTextBlock tb = document()->findBlockByNumber(m_cursorRelativeCoordinates ? m_screenScrollingRegionTop : m_screenTopLine);
    if (!tb.isValid())
    {
        if (m_cursorRelativeCoordinates)
        {
            qDebug() << "Error: m_screenScrollingRegionTop =" << m_screenScrollingRegionTop << "block count = " << document()->blockCount();
        }
        else
        {
            qDebug() << "Error: m_screenTopLine =" << m_screenTopLine << "block count = " << document()->blockCount();
        }
        tb = document()->lastBlock();
    }

    m_caret = QTextCursor(tb);
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
    moveCaretToTheLeftBy(1);
}

void PlainTextLog::setCaretBrightness(bool bright)
{
    m_caretBrightness = bright;
    updateCaretAttributes();
}

void PlainTextLog::setCaretInvertedColors(bool invert)
{
    m_caretInvertedColors = invert;
    updateCaretAttributes();
}

void PlainTextLog::setCaretUnderline(bool underline)
{
    m_caretUnderline = underline;
    updateCaretAttributes();
}

void PlainTextLog::setCaretForeground(PlainTextLog::AnsiColor ansiColor)
{
    m_caretFgColor = ansiColor;
    updateCaretAttributes();
}

void PlainTextLog::setCaretBackround(PlainTextLog::AnsiColor ansiColor)
{
    m_caretBgColor = ansiColor;
    updateCaretAttributes();
}

bool PlainTextLog::setScrollingRegion(int top, int bottom)
{
    if (bottom > top && top >= 0 && bottom < terminalScreenHeight)
    {
        m_screenScrollingRegionTop = top;
        m_screenScrollingRegionBottom = bottom;
        moveCaretToTheStartOfTheScreen();
        return true;
    }
    else
    {
        return false;
    }
}

void PlainTextLog::resetCaretAttributes()
{
    m_caretBrightness = false;
    m_caretUnderline = false;
    m_caretInvertedColors = false;
    m_caretFgColor = ANSI_WHITE;
    m_caretBgColor = ANSI_BLACK;
    updateCaretAttributes();
}

void PlainTextLog::updateCaretAttributes()
{
    m_tcfm.setFontUnderline(m_caretUnderline);
    if (m_caretInvertedColors)
    {
        m_tcfm.setForeground(QBrush(QColor(ansiColorToRgb(m_caretBgColor, m_caretBrightness))));
        m_tcfm.setBackground(QBrush(QColor(ansiColorToRgb(m_caretFgColor, false))));
    }
    else
    {
        m_tcfm.setForeground(QBrush(QColor(ansiColorToRgb(m_caretFgColor, m_caretBrightness))));
        m_tcfm.setBackground(QBrush(QColor(ansiColorToRgb(m_caretBgColor, false))));
    }
}

void PlainTextLog::resetTextDecoder()
{
    if (m_decoder)
    {
        delete m_decoder;
    }
    m_decoder = new QTextDecoder(QTextCodec::codecForName("UTF-8")); // everyone should use utf8, right?
}

QRgb PlainTextLog::ansiColorToRgb(PlainTextLog::AnsiColor ansiColor, bool isBright)
{
    QRgb rgb;

    switch (ansiColor)
    {
    case ANSI_BLACK:    rgb = isBright ? 0x686868 : 0x000000;      break;
    case ANSI_RED:      rgb = isBright ? 0xFF6F6B : 0xC91B00;      break;
    case ANSI_GREEN:    rgb = isBright ? 0x67F86F : 0x00C120;      break;
    case ANSI_YELLOW:   rgb = isBright ? 0xFFFA72 : 0xC7C327;      break;
    case ANSI_BLUE:     rgb = isBright ? 0x6A76FC : 0x0A2FC4;      break;
    case ANSI_MAGENTA:  rgb = isBright ? 0xFF7CFD : 0xC839C5;      break;
    case ANSI_CYAN:     rgb = isBright ? 0x68FDFE : 0x01C5C6;      break;
    case ANSI_WHITE:    rgb = isBright ? 0xFFFFFF : 0xC7C7C7;      break;
    }

    //qDebug() << __FUNCTION__ << ansiColor << isBright << "->" << rgb;

    return rgb;
}

void PlainTextLog::clear()
{
    m_escSeq.clear();

    resetTextDecoder();

    QPlainTextEdit::clear();

    m_screenTopLine = 0;
    m_cursorRelativeCoordinates = false;

    setScrollingRegion(0, terminalScreenHeight - 1);

    m_cursorMode = true;//XXX false;

    resetCaretAttributes();
}

void PlainTextLog::moveCaretToTheRightBy(int chars)
{
    Q_ASSERT(chars >= 0);

    int blockLength = m_caret.block().text().length();
    int pos = m_caret.positionInBlock();

    // qDebug() << "pos:" << pos << "chars:" << chars << "blockLength:" << blockLength;

    if (pos + chars <= blockLength)
    {
        m_caret.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, chars);
    }
    else
    {
        m_caret.movePosition(QTextCursor::EndOfBlock); // fixme force repaint caret

        int append = pos + chars - blockLength;

        //qDebug() << "appending symbols:" << append;

        insertTextAtCaret(QString(append, ' '));
    }
}

void PlainTextLog::moveCaretToTheLeftBy(int chars)
{
    Q_ASSERT(chars >= 0);

    if (m_caret.positionInBlock() < chars)
    {
        qDebug() << "Warning: attempt to cross the text block start boundary by" << chars - m_caret.positionInBlock() << "symbols";
        moveCaretToTheFirstColumn();
    }
    else
    {
        m_caret.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, chars);
    }
}

void PlainTextLog::moveCaretToTheFirstColumn()
{
    m_caret.movePosition(QTextCursor::StartOfBlock);
}

void PlainTextLog::moveCaretUpwardsBy(int lines)
{
    Q_ASSERT(lines >= 0);

    int col_was = m_caret.positionInBlock();

    if (m_caret.blockNumber() + (m_cursorRelativeCoordinates ? m_screenScrollingRegionTop : m_screenTopLine) < lines)
    {
        qDebug() << "Warning: attempt to cross the screen start boundary by" << lines - (m_caret.blockNumber() + (m_cursorRelativeCoordinates ? m_screenScrollingRegionTop : m_screenTopLine)) << "lines";
        qDebug() << "TODO implement sticky edge";
    }
    else
    {
        QTextBlock tb = document()->findBlockByNumber(m_caret.blockNumber() + (m_cursorRelativeCoordinates ? m_screenScrollingRegionTop : m_screenTopLine) - lines);
        Q_ASSERT(tb.isValid());
        m_caret = QTextCursor(tb);
        moveCaretToTheRightBy(col_was);
    }
}

void PlainTextLog::moveCaretDownwardsBy(int lines)
{
    Q_ASSERT(lines >= 0);

    // scroll -- take old scrollbar value
    QScrollBar *p_scroll_bar = this->verticalScrollBar();
    bool bool_at_bottom = (p_scroll_bar->value() == p_scroll_bar->maximum());

    // TODO check for terminalScreenHeight

    int blocks = document()->blockCount();
    int blockIx = m_caret.blockNumber();

    //qDebug() << blockIx << lines << blocks;

    int col_was = m_caret.positionInBlock();

    if (blockIx + lines >= blocks)
    {
        int append = blockIx + lines - blocks + 1;

        QTextCursor cur(document()->lastBlock());
        cur.movePosition(QTextCursor::EndOfBlock);

        for (int i = 0; i < append; ++i)
        {
            cur.insertBlock();
        }

        if (document()->blockCount() != blockIx + lines + 1)
        {
            qDebug() << "Error: appended" << append << "but got" << document()->blockCount() << "blockIx" << blockIx << lines;
        }
    }

    QTextBlock tb = document()->findBlockByNumber(blockIx + lines);
    if (!tb.isValid())
    {
        qDebug() << "Error:" << __FUNCTION__ << lines << "invalid block at" << blockIx + lines << "document()->blockCount() = " << document()->blockCount();
        tb = document()->lastBlock();
    }
    m_caret = QTextCursor(tb);
    moveCaretToTheRightBy(col_was);

    // scroll down if needed
    if (bool_at_bottom)
    {
        p_scroll_bar->setValue(p_scroll_bar->maximum());
    }
}

void PlainTextLog::clearToCurrentContextMenuLine()
{
    //qDebug() << "clearToCurrentLine" << textCursor().block().text() << textCursor().blockNumber();

    QTextCursor cur(document());

    m_contextMenuTextCursor.beginEditBlock();
    {
        cur.movePosition(QTextCursor::Start);
        cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, m_contextMenuTextCursor.block().position());
        cur.removeSelectedText();
    }
    m_contextMenuTextCursor.endEditBlock();
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

            if (lbData->m_item)
            {
                resizeMark(lbData->m_item, currentBlock);
            }
        }
        currentBlock = currentBlock.next();
    }

    if (m_sideMarkScene)
    {
        m_sideMarkScene->setSceneRect(0,0,0,height()); // sync scene coordinates
    }
}

void PlainTextLog::keyPressEvent(QKeyEvent *e)
{
    QString t = e->text();

    switch (e->key())
    {
    case Qt::Key_F1: sendVT100EscSeq(VT100_EC_F1); return;
    case Qt::Key_F2: sendVT100EscSeq(VT100_EC_F2); return;
    case Qt::Key_F3: sendVT100EscSeq(VT100_EC_F3); return;
    case Qt::Key_F4: sendVT100EscSeq(VT100_EC_F4); return;
    case Qt::Key_F5: sendVT100EscSeq(VT100_EC_F5); return;
    case Qt::Key_F6: sendVT100EscSeq(VT100_EC_F6); return;
    case Qt::Key_F7: sendVT100EscSeq(VT100_EC_F7); return;
    case Qt::Key_F8: sendVT100EscSeq(VT100_EC_F8); return;
    case Qt::Key_F9: sendVT100EscSeq(VT100_EC_F9); return;
    case Qt::Key_F10:sendVT100EscSeq(VT100_EC_F10); return;
    }

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

                //qDebug() << QString("sending ctrl-seq: ^%1").arg((char) e->key());
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
        p.fillRect(this->cursorRect(m_caret), m_tcfm.foreground());
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
    //
    // With a great help of: http://www.vt100.net/docs/vt102-ug/appendixc.html
    //                       http://www.vt100.net/docs/vt100-ug
    //

    QRect caretWasRect = this->cursorRect(m_caret);

    //qDebug() << bytes;

    for (int i = 0; i < bytes.size();)
    {
        unsigned char c = bytes.at(i);
        i++;

        if (!m_escSeq.isEmpty())
        {
            m_escSeq += c;

#if 0
            while (i < bytes.size() && bytes.at(i) > ' ' && bytes.at(i) < 0x7F) // only ascii symbols excluding ' ' (space)
            {
                //TODO handle non-ascii sequences properly

                m_escSeq += bytes.at(i);
                i++;
            }
#endif

            int pos = 0;
            QValidator::State st = m_vt100escReV->validate(m_escSeq, pos);

            //qDebug() << "Checking" << m_escSeq << "->" << st;

            if (st == QRegExpValidator::Invalid)
            {
                insertTextAtCaret(m_escSeq);
                qDebug() << QString("Warning: unknown VT100 sequence %1%2").arg(m_escSeq).arg(QString(bytes.right(bytes.size() - i).left(16)));
                m_escSeq.clear();
            }
            else if (st == QRegExpValidator::Acceptable)
            {
                //qDebug() << m_escSeq;

                QRegularExpressionMatch m = m_vt100escReV->regularExpression().match(m_escSeq);

                if (!m.isValid())
                {
                    qDebug() << "Error: " << m_escSeq << " is accepted by QRegExpValidator(" << m_vt100escReV->regularExpression().pattern() << "), but rejected by QRegExp::exactMatch";
                }
                else
                {
                    QStringList args = m.capturedTexts();

                    //qDebug() << args;

                    QString remainder = args.last();
                    Q_ASSERT(remainder.isEmpty()); // XXX
                    args.removeLast();

                    QMutableListIterator<QString> i(args);
                    while (i.hasNext())
                    {
                        QString &s = i.next();
                        if (s.isEmpty())
                        {
                            i.remove();
                        }
                    }

                    QString cmd = args.last();
                    args.removeLast();

                    if (args.at(0) == m_escSeq)
                    {
                        args.removeFirst();
                    }

                    // get rid of the junk at the end of the captured esc sequence
                    m_escSeq.remove(m_escSeq.length() - remainder.length(), remainder.length());

                    //qDebug() << "Detected" << cmd << "command, args:" << args;

                    if (cmd == "K")
                    {
                        if (args.isEmpty() || args.at(0) == "0")
                        {
                            // clear eol
                            m_caret.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                            m_caret.removeSelectedText();
                        }
                        else if (args.at(0) == "1")
                        {
                            int pos = m_caret.positionInBlock();
                            moveCaretToTheRightBy(pos);
                            insertTextAtCaret(QString(pos, ' '));
                        }
                        else
                        {
                            qDebug() << "Not supported:" << m_escSeq;
                        }
                    }
                    else if (cmd == "J")
                    {
                        if (args.isEmpty() || (args.size() == 1 && args.at(0) == "0"))
                        {
                            // clear eol
                            m_caret.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                            m_caret.removeSelectedText();

                            // clear all the lines below (XXX only down to the screen?)
                            QTextBlock bl = m_caret.block().next();
                            while (bl.isValid()) {
                                QTextCursor cur(bl);
                                cur.select(QTextCursor::BlockUnderCursor);
                                cur.removeSelectedText();
                                bl = bl.next();
                            }
                        }
                        else if (args.size() == 1 && args.at(0) == "1")
                        {
                            int col_was = m_caret.positionInBlock();
                            int row_was = m_caret.blockNumber() - m_screenTopLine;
                            Q_ASSERT(row_was >= 0);

                            //qDebug() << "was #" << col_was << row_was;

                            QTextBlock block = document()->findBlockByNumber(m_screenTopLine);
                            Q_ASSERT(block.isValid());

                            for (int line = 0; line < row_was; ++line)
                            {
                                Q_ASSERT(block.isValid());

                                QTextCursor cur(block);
                                cur.insertText(" ");
                                cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                                cur.removeSelectedText();

                                block = block.next();
                            }

                            int col_is = m_caret.positionInBlock();
                            int row_is = m_caret.blockNumber() - m_screenTopLine;
                            Q_ASSERT(col_is == col_was);
                            Q_ASSERT(row_is == row_was);

                            moveCaretToTheFirstColumn();

                            QString s;
                            for (int i = 0; i < col_was + 1; ++i) // erase including the cursor position
                            {
                                s += " ";
                            }
                            insertTextAtCaret(s);

                            moveCaretToTheLeftBy(1);
                        }
                        else if (args.size() == 1 && args.at(0) == "2")
                        {
                            int col_was = m_caret.positionInBlock();
                            int row_was = m_caret.blockNumber() - m_screenTopLine;
                            Q_ASSERT(row_was >= 0);

                            QTextBlock block = document()->findBlockByNumber(m_screenTopLine);
                            Q_ASSERT(block.isValid());

                            for (int line = 0; line < terminalScreenHeight; ++line)
                            {
                                if (!block.isValid())
                                {
                                    break;
                                }

                                QTextCursor cur(block);
                                cur.insertText(" ");
                                cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                                cur.removeSelectedText();

                                block = block.next();
                            }

                            moveCaretToTheFirstColumn();
                            moveCaretToTheRightBy(col_was);

                            int col_is = m_caret.positionInBlock();
                            int row_is = m_caret.blockNumber() - m_screenTopLine;
                            Q_ASSERT(col_is == col_was);
                            Q_ASSERT(row_is == row_was);
                        }
                        else
                        {
                            qDebug() << "Not supported:" << m_escSeq;
                        }
                    }
                    else if (cmd == "m")
                    {
                        if (args.isEmpty())
                        {
                            resetCaretAttributes();
                        }
                        else if (args.at(0) == "38")
                        {
                            // extended color table
                            qDebug() << "Not supported:" << m_escSeq;
                        }
                        else
                        {
                            foreach (QString param, args)
                            {
                                int p = param.toInt();

                                switch (p)
                                {
                                case 0:  resetCaretAttributes();            break;
                                case 1:  setCaretBrightness(true);          break;
                                case 4:  setCaretUnderline(true);           break;
                                case 7:  setCaretInvertedColors(true);      break;

                                case 30: setCaretForeground(ANSI_BLACK);    break;
                                case 31: setCaretForeground(ANSI_RED);      break;
                                case 32: setCaretForeground(ANSI_GREEN);    break;
                                case 33: setCaretForeground(ANSI_YELLOW);   break;
                                case 34: setCaretForeground(ANSI_BLUE);     break;
                                case 35: setCaretForeground(ANSI_MAGENTA);  break;
                                case 36: setCaretForeground(ANSI_CYAN);     break;
                                case 37: setCaretForeground(ANSI_WHITE);    break;

                                case 40: setCaretBackround(ANSI_BLACK);     break;
                                case 41: setCaretBackround(ANSI_RED);       break;
                                case 42: setCaretBackround(ANSI_GREEN);     break;
                                case 43: setCaretBackround(ANSI_YELLOW);    break;
                                case 44: setCaretBackround(ANSI_BLUE);      break;
                                case 45: setCaretBackround(ANSI_MAGENTA);   break;
                                case 46: setCaretBackround(ANSI_CYAN);      break;
                                case 47: setCaretBackround(ANSI_WHITE);     break;

                                case 2: //Dim
                                case 5: //Blink
                                case 8: //Hidden
                                default:
                                    qDebug() << "Not supported: m-sequence param" << p << "of" << m_escSeq << "escape sequence";
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        qDebug() << m_escSeq;

                        //qDebug() << "Detected" << m_escSeq << "sequence, args:" << list;

                        if (cmd == "=")
                        {
                            qDebug() << QString("Not supported: 'Application Keypad Mode' %1").arg(m_escSeq);
                        }
                        else if (cmd == ">")
                        {
                            qDebug() << QString("Not supported: 'Numeric Keypad Mode' %1").arg(m_escSeq);
                        }
                        else if (cmd == ")0")
                        {
                            qDebug() << QString("Not supported: 'Set G1 special chars. & line set' %1").arg(m_escSeq);
                        }
                        else if (cmd == "(B")
                        {
                            qDebug() << QString("Not supported: 'Set United States G0 character set' %1").arg(m_escSeq);
                        }
                        else if (cmd == "H" || cmd == "f")
                        {
                            if (args.isEmpty())
                            {
                                moveCaretToTheStartOfTheScreen();
                            }
                            else if (args.length() == 1)
                            {
                                qDebug() << QString("Not supported: '?fH' %1").arg(m_escSeq);
                            }
                            else
                            {
                                Q_ASSERT(args.length() == 2);

                                bool ok_row = false;
                                int row = args.at(0).toInt(&ok_row) - 1;

                                bool ok_col = false;
                                int column = args.at(1).toInt(&ok_col) - 1;

                                if (!ok_row || !ok_col || row < 0 || column < 0)
                                {
                                    qDebug() << "Error: Invalid params" << m_escSeq;
                                }
                                else
                                {
                                    //qDebug() << m_escSeq << "m_screenTopLine = " << m_screenTopLine;
                                    moveCaretToTheStartOfTheScreen();

                                    if (m_cursorRelativeCoordinates)
                                    {
                                        // relative positioning
                                        row += m_screenScrollingRegionTop;
                                        if (row > m_screenScrollingRegionBottom)
                                        {
                                            qDebug() << QString("Warning: trying to position caret outside (%1) of the scrolling region (%2,%3)")
                                                        .arg(row).arg(m_screenScrollingRegionTop).arg(m_screenScrollingRegionBottom);
                                            row = m_screenScrollingRegionBottom;
                                        }
                                    }

                                    moveCaretDownwardsBy(row);
                                    moveCaretToTheRightBy(column);

                                    if (m_caret.blockNumber() != row + m_screenTopLine || m_caret.positionInBlock() != column)
                                    {
                                        qDebug() << QString("Error: Cursor positioned to (%1,%2) instead of (%3,%4)")
                                                    .arg(m_caret.blockNumber()).arg(m_caret.positionInBlock())
                                                    .arg(row).arg(column);
                                    }

                                    //qDebug() << m_escSeq << QString("Cursor: row=%1, col=%2; Scroll region: top=%3, bottom=%4")
                                    //            .arg(m_caret.blockNumber() - m_screenTopLine).arg(m_caret.positionInBlock())
                                    //            .arg(m_screenScrollingRegionTop).arg(m_screenScrollingRegionBottom);
                                }
                            }
                        }
                        else if (cmd == "A")
                        {
                            int lines = 1;
                            if (!args.isEmpty())
                            {
                                lines = args.at(0).toInt(); // todo add check
                            }
                            moveCaretUpwardsBy(lines);
                        }
                        else if (cmd == "B")
                        {
                            int lines = 1;
                            if (!args.isEmpty())
                            {
                                lines = args.at(0).toInt(); // todo add check
                            }
                            moveCaretDownwardsBy(lines);
                        }
                        else if (cmd == "C")
                        {
                            int chars = 1;
                            if (!args.isEmpty())
                            {
                                chars = args.at(0).toInt(); // todo add check
                            }
                            moveCaretToTheRightBy(chars);
                        }
                        else if (cmd == "D")
                        {
                            int chars = 1;
                            if (!args.isEmpty())
                            {
                                chars = args.at(0).toInt(); // todo add check
                            }
                            moveCaretToTheLeftBy(chars);
                        }
                        else if (cmd == "M")
                        {
                            // Move the active position to the same horizontal position on the preceding line.
                            // If the active position is at the top margin, a scroll down is performed.

                            qDebug() << m_escSeq << QString("Cursor: row=%1, col=%2; Scroll region: top=%3, bottom=%4")
                                        .arg(m_caret.blockNumber() - m_screenTopLine).arg(m_caret.positionInBlock())
                                        .arg(m_screenScrollingRegionTop).arg(m_screenScrollingRegionBottom);

                            if (m_caret.blockNumber() - m_screenTopLine == m_screenScrollingRegionTop)
                            {
                                //
                            }
                            else
                            {
                                //
                            }
                        }
                        else if (cmd == "?")
                        {
                            qDebug() << QString("Not supported: '?' %1").arg(m_escSeq);
                        }
                        else if (cmd == "r")
                        {
                            if (args.length() == 2)
                            {
                                bool ok_start;
                                int start = args.at(0).toInt(&ok_start) - 1;

                                bool ok_end;
                                int end = args.at(1).toInt(&ok_end) - 1;

                                if (!ok_start || !ok_end || !setScrollingRegion(start, end))
                                {
                                    qDebug() << "Error: Invalid params" << m_escSeq;
                                }
                            }
                            else if (args.length() == 0)
                            {
                                setScrollingRegion(0, terminalScreenHeight - 1);
                            }
                            else
                            {
                                qDebug() << args.length();
                                Q_ASSERT(0);
                            }
                        }
                        else if (cmd == "c")
                        {
                            sendVT100EscSeq(VT100_EC_DA_VT102);
                        }
                        else if (cmd == "h")
                        {
                            Q_ASSERT(args.length() == 1);

                            bool ok;
                            int arg = args.at(0).toInt(&ok);

                            if (!ok)
                            {
                                qDebug() << "Error: Invalid params" << m_escSeq;
                            }
                            else
                            {
                                switch (arg)
                                {
                                case 1:
                                    m_cursorMode = true;
                                    break;

                                case 4:
                                    qDebug() << "Not supported: 'Set smooth scroll'";
                                    break;

                                case 6:
                                    // relative caret coordinates (independent of scrolling region)
                                    m_cursorRelativeCoordinates = true;
                                    break;

                                case 7:
                                    qDebug() << "Not supported: 'Set auto-wrap mode'";
                                    break;

                                case 40:
                                default:
                                    qDebug() << "Not supported:" << m_escSeq;
                                    break;
                                }
                            }
                        }
                        else if (cmd == "l")
                        {
                            Q_ASSERT(args.length() == 1);

                            bool ok;
                            int arg = args.at(0).toInt(&ok);

                            if (!ok)
                            {
                                qDebug() << "Error: Invalid params" << m_escSeq;
                            }
                            else
                            {
                                switch (arg)
                                {
                                case 1:
                                    m_cursorMode = false;
                                    break;

                                case 3:
                                    Q_ASSERT(terminalScreenWidth == 80); // todo implement reset to 80 columns
                                    break;

                                case 4:
                                    // select jump scroll (as opposed to smooth scroll)
                                    break;

                                case 5:
                                    // normal screen -- white on black;
                                    break;

                                case 6:
                                    // absolute caret coordinates (independent of scrolling region)
                                    m_cursorRelativeCoordinates = false;
                                    break;

                                case 7:
                                    qDebug() << "Not supported: 'Reset auto-wrap mode'";
                                    break;

                                case 8:
                                    // turn off auto repeat
                                    break;

                                case 45:
                                default:
                                    qDebug() << "Not supported:" << m_escSeq;
                                    break;
                                }
                            }
                        }
                        else if (cmd == "#8")
                        {
                            moveCaretToTheStartOfTheScreen();

                            for (int row = 0; ; ++row)
                            {
                                QString str;

                                for (int col = 0; col < terminalScreenWidth; ++col)
                                {
                                    str += 'E';
                                }

                                insertTextAtCaret(str);

                                if (row == terminalScreenHeight - 1)
                                    break;

                                moveCaretToTheFirstColumn(); // \r
                                moveCaretDownwardsBy(1); // \n
                            }
                        }
                        else
                        {
                            qDebug() << "Not supported:" << m_escSeq;
                        }
                    }
                    if (!remainder.isEmpty())
                    {
                        insertTextAtCaret(remainder);
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
            QString str;

            str += m_decoder->toUnicode((const char *) &c, 1);

            if (m_decoder->hasFailure())
            {
                qDebug() << "Error: m_decoder->hasFailure";
                resetTextDecoder();
            }

            while (i < bytes.size() && bytes.at(i) >= ' ')
            {
                c = bytes.at(i);
                str += m_decoder->toUnicode((const char *) &c, 1);
                i++;

                if (m_decoder->hasFailure())
                {
                    qDebug() << "Error: m_decoder->hasFailure";
                    resetTextDecoder();
                }
            }

            if (!str.isEmpty())
            {
                insertTextAtCaret(str);
            }
        }
        else
        {
            switch (c)
            {
            case '\0':
                break;

            case 0x0F:
                //TODO support
                break;

            case 0x1B:
                if (!m_escSeq.isEmpty())
                {
                    insertTextAtCaret(m_escSeq);
                    qDebug() << "Warning: aborted" << m_escSeq << "sequence";
                    m_escSeq.clear();
                }

                m_escSeq += "^[";
                break;

            case '\a':
                //qDebug() << "system bell";
                break;

            case '\n':
                //qDebug() << "\\n";
                moveCaretDownwardsBy(1);
                break;

            case '\t':
                insertTextAtCaret(QString(c)); // FIXME
                break;

            case '\b':
                caretBackspace();
                break;

            case '\r':
                //qDebug() << "\\r";
                moveCaretToTheFirstColumn();
                break;

            default:
                insertTextAtCaret(QString("\\u00%1").arg(c, 2, 16, QChar('0')));
                break;
            }
        }
    }

    QRect caretIsRect = this->cursorRect(m_caret);

    if (caretIsRect != caretWasRect)
    {
        viewport()->repaint(); // TODO: make it more efficient
    }
}

void PlainTextLog::insertTextAtCaret(const QString &text)
{
    if (text.isEmpty())
    {
        qDebug() << "Warning: inserting empty text";
        return;
    }

    int toRemove = qMin(m_caret.block().text().length() - m_caret.positionInBlock(), text.length());

    //qDebug() << text;

    m_caret.beginEditBlock();
    {
        m_caret.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, toRemove);
        m_caret.removeSelectedText();
        m_caret.insertText(text, m_tcfm);
    }
    m_caret.endEditBlock();
}
