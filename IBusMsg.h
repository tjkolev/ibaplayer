/***************************************************************************
 *   Copyright (C) 2004 by TJ Kolev                                        *
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

#ifndef _IBUS_MSG_H_
#define _IBUS_MSG_H_

#include <errno.h>
#include <sys/msg.h>

#include "IBusCntr.h"

struct IBusMsg // custom definition for mag.h/msgbuf
{
    long int mtype; // as required for ipc

    byte    devFrom;
    byte    devTo;
    byte    data[IBusCntr::MAX_PACKET_LEN];
    byte    len;

    //IBusMsg(byte from, byte to, const byte* d, byte l);
};

class IBusMsgQueue
{
public:
    static IBusMsgQueue& getQueue();

    static IBusMsg* fillMsg(IBusMsg*, byte from, byte to, const byte* d, byte l);

    bool       hasMsg();
    void       put(IBusMsg*);
    IBusMsg*   get(IBusMsg*);
    //void       clear();

private:
    IBusMsgQueue();
    ~IBusMsgQueue();

    static IBusMsgQueue  m_msgQueue;

    int             m_mqId;

    /*
    static const int     QUEUE_SIZE = 8;
    IBusMsg*             m_queue[QUEUE_SIZE];
    int                  m_head;
    int                  m_tail;

    pthread_mutex_t      m_mutex;
    */
};

#endif //_IBUS_MSG_H_

