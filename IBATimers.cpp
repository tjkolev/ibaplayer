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

#include <assert.h>
#include <string>
#include "IBATimers.h"

IBATimers::IBATimers()
{
    m_tickCount = 0;
    memset(m_timers, 0, sizeof(m_timers));
}

IBATimers::~IBATimers()
{
    timer_delete(m_sysTimerId);
}

void timerHandler(sigval_t sa)
{
    if(NULL == sa.sival_ptr)
        return;

    IBATimers* ibat = (IBATimers*) sa.sival_ptr;
    assert(ibat);
    ibat->onTimer();
}

void IBATimers::init()
{
    m_tspec.it_value.tv_sec = 0;
    m_tspec.it_value.tv_nsec = MIN_INTERVAL * 1000000L; //millisec to nanosec
    m_tspec.it_interval = m_tspec.it_value;

    memset(&m_sige, 0, sizeof(m_sige));
    m_sige.sigev_notify = SIGEV_THREAD;
    m_sige.sigev_value.sival_ptr = this;
    m_sige.sigev_notify_function = timerHandler;
    int result = timer_create(CLOCK_REALTIME, &m_sige, &m_sysTimerId);
    assert(0 == result);

    result = timer_settime(m_sysTimerId, 0, &m_tspec, NULL);
    assert(0 == result);
}

void IBATimers::setTimer(timerID ti, float sec, IBATimerListener* lsn)
{
    assert(ti >= 0 && ti < _TI_COUNT_);

    m_timers[ti].tiTicks = 0;
    m_timers[ti].tiCfgTicks = secToTicks(sec);
    m_timers[ti].listener = lsn;
}

void IBATimers::onTimer()
{
    for(int tid = 0; tid < _TI_COUNT_; tid++)
    {
        ti_t& ti = m_timers[tid];
        if(ti.tiTicks > 0 &&
           NULL != ti.listener &&
           (m_tickCount % ti.tiTicks == 0))

            ti.listener->onTimer((timerID) tid);
    }

    m_tickCount++;
}

int IBATimers::secToTicks(float sec)
{
    assert(sec >= 0);

    int msec = (int) (sec * 1000);
    return msec / MIN_INTERVAL;
}

void IBATimers::stopAll()
{
    for(int ti = 0; ti < _TI_COUNT_; ti++)
        stop((timerID) ti);
}

void IBATimers::startAll()
{
    for(int ti = 0; ti < _TI_COUNT_; ti++)
        start((timerID) ti);
}

void IBATimers::stop(timerID ti)
{
    assert(ti >= 0 && ti < _TI_COUNT_);
    m_timers[ti].tiTicks = 0;
}

void IBATimers::start(timerID ti)
{
    m_timers[ti].tiTicks = m_timers[ti].tiCfgTicks;
}
