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

#ifndef _IBA_PLAYER_H_
#define _IBA_PLAYER_H_

#include <string>
#include "IBAVer.h"
#include "MusicLibrary.h"
#include "AlsaPlayerCntr.h"
#include "IBusCntr.h"
#include "IBATimers.h"

using namespace std;

class IBAPlayer;

class MsgProcessor : public IBATimerListener
{
public:
    MsgProcessor();
    ~MsgProcessor();

    bool            init(IBAPlayer&);
    void            run();

    void            onTimer(IBATimers::timerID);

private:

    enum mode
    {
        MODE_INACTIVE = 0,
        MODE_PLAY,
        MODE_BROWSE
    };

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
    void            startPlaying();
    void            setMIDLabels(mode);
    void            resetOBC();
    void            clearOBC();
    void            toBrowseMode();

    mode                m_mode;
    bool                m_cdc_playing;
    IBAPlayer*          m_iba;
    AlsaPlayerCntr*     m_alsa;
    IBusCntr*           m_ibus;
    MusicLibBrowser*    m_lib;

    unsigned long       m_respDelay;

    bool                m_outPos;
    bool                m_outInfo;
    int                 m_dispInfoNdx;
    time_t              m_browseTime;
    time_t              m_lblTime;
    time_t              m_statusTime;

    bool                m_scrollTrackInfo;
    int                 m_scrollChars;
    int                 m_scrollPos;

    bool                m_repeatOn;
    bool                m_randomOn;
};



class IBAPlayer
{
public:

	IBAPlayer();
	virtual ~IBAPlayer();

    //MusicLibMngr&   getLibMngr();
    AlsaPlayerCntr& getPlayer();
    IBusCntr&       getIBusCntr();
    IBATimers&      getTimers();

    void reindexLib();
    int  run();

private:

    bool             initPlayer();

    void             play();

    AlsaPlayerCntr   m_ap;
    //MusicLibMngr&    m_mlibMngr;
    IBusCntr         m_ibus;
    MsgProcessor     m_msgProc;
    IBATimers        m_timers;
    pthread_t        m_threadIBusCntr;
};

#endif // _IBA_PLAYER_H_
