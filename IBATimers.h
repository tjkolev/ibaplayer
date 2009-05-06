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
#ifndef _IBA_TIMERS_H_
#define _IBA_TIMERS_H_

#include <signal.h>
#include <time.h>

class IBATimerListener;

class IBATimers
{
// keeping my own timer ticks with one timer
// resolution and min interval is 200 milliseconds.

public:
    enum timerID
    {
        TI_ANNOUNCE = 0,
        TI_TRACK_POS,
        TI_TRACK_INFO,

        _TI_COUNT_
    };

    static const int MIN_INTERVAL = 200; //msec

    IBATimers();
    ~IBATimers();

    bool init();
    void setTimer(timerID, float sec, IBATimerListener*);

    void startAll();
    void stopAll();
    void start(timerID);
    void stop(timerID);

private:

    struct timer_reg
    {
        int                 tiTicks;
        int                 tiCfgTicks;
        IBATimerListener*   listener;
    };

    typedef timer_reg ti_t;

    int         secToTicks(float);
    void        onTimer();

    friend void timerHandler(sigval_t);

    timer_t             m_sysTimerId;
    struct itimerspec   m_tspec;
    sigevent_t          m_sige;

    ti_t                m_timers[_TI_COUNT_];

    unsigned long       m_tickCount;
};


class IBATimerListener
{
public:
    virtual void onTimer(IBATimers::timerID) = 0;
};


#endif

