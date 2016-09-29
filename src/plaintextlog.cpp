#include "plaintextlog.h"

#include <QScrollBar>

PlainTextLog::PlainTextLog(QWidget *parent) :
    QPlainTextEdit(parent)
{
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
