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
#include <semaphore.h>
#include <queue>

#define IBUS_MSG_TYPE       1

IBusMsg::IBusMsg(byte from, byte to, const byte* d, byte l)
{
    devFrom = from;
    devTo = to;
    len = l;
    if(NULL != d && l > 0)
		memcpy(data, d, l);
}

// the single message queue
IBusMsgQueue IBusMsgQueue::m_msgQueue;
IBusMsgQueue& IBusMsgQueue::getQueue()
{
    return m_msgQueue;
}

sem_t sem;
IBusMsgQueue::IBusMsgQueue()
{
	m_semaphore = &sem;
	sem_init(&sem, 0, 1);
}

IBusMsgQueue::~IBusMsgQueue()
{
	sem_destroy(&sem);
}

queue<IBusMsg> theQueue;
void IBusMsgQueue::enqueue(IBusMsg& msg)
{
	if(sem_wait(&sem) < 0)
		return;
	theQueue.push(msg);
	sem_post(&sem);
}

IBusMsg IBusMsgQueue::dequeue()
{
	if(sem_wait(&sem) < 0)
		return IBusMsg(0, 0, NULL, 0);
	IBusMsg msg = theQueue.front();
	theQueue.pop();
	sem_post(&sem);
	return msg;
}

bool IBusMsgQueue::hasMsg()
{
	if(sem_wait(&sem) < 0)
		return false;
	bool empty = theQueue.empty();
	sem_post(&sem);
	return !empty;
}
