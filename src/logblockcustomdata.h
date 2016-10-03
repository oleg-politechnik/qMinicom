#ifndef LOGBLOCKCUSTOMDATA_H
#define LOGBLOCKCUSTOMDATA_H

#include <QTextObject>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QDebug>

class LogBlockCustomData : public QTextBlockUserData
{
public:
    LogBlockCustomData(QGraphicsRectItem *item) : m_item(item)
    {
    }

    ~LogBlockCustomData()
    {
    }

    QGraphicsRectItem *m_item; // LogBlockCustomData object deletion works properly even when someone else has already deleted the object pointed by m_item, don't know why
};

#endif // LOGBLOCKCUSTOMDATA_H
