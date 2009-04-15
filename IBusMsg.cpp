/***************************************************************************
 *   Copyright (C) 2009 by TJ Kolev                                        *
 *   tjkolev@yahoo.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "IBusMsg.h"

#define IBUS_MSG_TYPE       1

IBusMsg* IBusMsgQueue::fillMsg(IBusMsg* msg, byte from, byte to, const byte* d, byte l)
{
    if(NULL == msg) return NULL;

    msg->mtype   = IBUS_MSG_TYPE;

    msg->devFrom = from;
    msg->devTo   = to;
    memcpy(msg->data, d, l);
    msg->len     = l;
    return msg;
}

// the single message queue
IBusMsgQueue IBusMsgQueue::m_msgQueue;
IBusMsgQueue& IBusMsgQueue::getQueue()
{
    return m_msgQueue;
}

IBusMsgQueue::IBusMsgQueue()
{
    key_t mqKey = ftok(".", 0x01BA);

    m_mqId = msgget(mqKey, IPC_CREAT | 0660);
    if(m_mqId < 0) // failed.
        return;
}

IBusMsgQueue::~IBusMsgQueue()
{
    if(m_mqId < 0)
        return;

    // delete the queue
    msgctl(m_mqId, IPC_RMID, NULL);

    m_mqId = -1;
}

void IBusMsgQueue::put(IBusMsg* msg)
{
    if(NULL == msg || m_mqId < 0) return;

    int msize = sizeof(IBusMsg) - sizeof(msg->mtype);
    int result = msgsnd(m_mqId, msg, msize, IPC_NOWAIT);
    if(result < 0)
        perror("Failed to put message in queue");
}

bool IBusMsgQueue::hasMsg()
{
    if(m_mqId < 0)
        return false;

    int result = msgrcv(m_mqId, NULL, 0, IBUS_MSG_TYPE, IPC_NOWAIT);
    if(result < 1 && errno == E2BIG)
        return true;

    return false;
}

IBusMsg* IBusMsgQueue::get(IBusMsg* msg)
{
    if(NULL == msg || m_mqId < 0) return NULL;

    int msize = sizeof(IBusMsg) - sizeof(msg->mtype);
    int result = msgrcv(m_mqId, msg, msize, IBUS_MSG_TYPE, IPC_NOWAIT);
    if(result < 0)
    {
        if(errno == ENOMSG)
            return NULL;

        perror("Failed to get message from queue");
        return NULL;
    }

    return msg;
}

/*void IBusMsgQueue::clear()
{
    pthread_mutex_lock(&m_mutex);

    // destroy all unpicked objects in queue;
    for(int ndx = m_tail; m_tail != m_head; m_tail = (m_tail + 1) % QUEUE_SIZE)
        delete m_queue[ndx];

    m_head = 0;
    m_tail = m_head;

    pthread_mutex_unlock(&m_mutex);
}*/

