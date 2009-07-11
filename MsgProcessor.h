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

#ifndef _MSG_PROCESSOR_H_
#define _MSG_PROCESSOR_H_

#include <string>
#include "IBATimers.h"
#include "IBusCntr.h"
#include "MusicLibrary.h"

using namespace std;

class MsgProcessor : public IBATimerListener
{
public:
    MsgProcessor();
    ~MsgProcessor();

	bool            init(IBATimers&);
    void            run();
    void            onTimer(IBATimers::timerID);

private:

    enum mode
    {
        MODE_INACTIVE = 0,
        MODE_PLAY,
        MODE_BROWSE
    };

    IBATimers*			m_timers;
    IBusCntr            m_ibus;
    AlsaPlayerCntr		m_alsa;
    MusicLibBrowser		m_lib;

    pthread_t        	m_threadIBusCntr;

    void            outTrackPosition();
    void            outTrackInfo();
    void            checkForMsg();
    void            checkWatches();
    void            sendStatus();
    void            processIBUSmsg(void* msg);
    void            scrollTrackInfo();
    void            alternateTrackInfo();
    bool            displayTrackInfo(char*);
    bool            displayWithShift(char*);
    char*           scrollTxt(char* scrollBuf, char* txt);
    void            onButtonPress(int);
    void            handlePlayButton(int);
    void            handleBrowseButton(int);
//    void            startPlaying();
    void            setMIDLabels(bool waitSome = false);
    void            resetOBC();
    void            clearOBC();
    void            toBrowseMode();

    mode                m_mode;
    bool                m_cdc_playing;

    //unsigned long       m_respDelay;

    bool                m_outPos;
    bool                m_outInfo;
    int                 m_dispInfoNdx;
    time_t              m_browseTime;
    time_t              m_lblTime;
    time_t              m_statusTime;

    bool                m_scrollTrackInfo;
    int                 m_scrollChars;
    int                 m_scrollPos;

    int					m_ibusLabelsTimeout;
    int					m_ibusStatusTimeout;
    int					m_browseTimeout;
};


#endif // _MSG_PROCESSOR_H_
