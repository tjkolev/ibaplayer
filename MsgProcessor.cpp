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

#include <assert.h>
#include "IBusMsg.h"
#include "MsgProcessor.h"

//static const byte IBUS_DATA_START_PL[]    = {0x38,0x03,0x00};

// data packets to setup the labels for the buttons
// two labels in 8 char display

/*
21 00 26 06 46 4D 05 41 4D 05 50 54 59 20 05 20 52 44 53 05 53 43 20 05 4D 4F 44 45
 !  .  &  .  F  M  .  A  M  .  P  T  Y     .     R  D  S  .  S  C     .  M  O  D  E
21 00 00 60 2D 20 42 41 05 53 53 20 2B 05 2D 20 54 52 05 45 42 20 2B 05 C1 20 46 41 05 44 52 20 5E
 !  .  .  `  -     B  A  .  S  S     +  .  -     T  R  .  E  B     +  .  .     F  A  .  D  R     ^
*/

// FM and AM buttons must remain, and not be handled, as those will switch to radio source. No way to prevent this.
// These are the button labels when the CD changer is the audio source:
// [1    2] [3    4] [5    6] [FM    AM] [    *RND] [SC*  MODE]
// Pressing RND or SC will update the labels with a * next to the buttons. So I'll have to re-send my labels for
// buttons 0x09 and 0x0A


// [Menu Sel] [  Page  ] [Play Add] [FM    AM] [     Rnd] [     MODE]
//  1      2   3      4   5      6
static const byte IBUS_LBLS1[] =  // 0xC1 is displayed as inverted ^ for 'down'
{0x21,0x00,0x00,0x60, 'M','e','n','u',0x05,' ','S','e','l',0x05,0xC1,' ','P','a',0x05,'g','e',' ','^',0x05,'P','l','a','y',0x05,' ','A','d','d'};
static const byte IBUS_LBLS2[] =
{0x21,0x00,0x00,0x06, 'F','M',' ',' ',0x05,' ',' ','A','M',0x05,' ',' ',' ',' ',0x05,' ',' ',' ',' ',0x05,' ',' ',' ',' ',0x05,'M','O','D','E'};

static const byte IBUS_RESET_OBC_LCD[] = {0x23, 0x01, 0x00};

#define MID_BUTN_CD1            1
#define MID_BUTN_CD2            2
#define MID_BUTN_CD3            3
#define MID_BUTN_CD4            4
#define MID_BUTN_CD5            5
#define MID_BUTN_CD6            6
#define MID_BUTN_NEXT           101
#define MID_BUTN_PREV           102
#define MID_RANDOM_ON           103
#define MID_RANDOM_OFF          104
#define MID_SCAN_ON             105
#define MID_SCAN_OFF            106


MsgProcessor::MsgProcessor()
{
    m_timers = NULL;

    m_mode = MODE_INACTIVE;
    m_cdc_playing = false;
    m_respDelay = 10000;
    m_outPos = false;
    m_outInfo = false;
    m_dispInfoNdx = 0;
    m_scrollTrackInfo = false;
    m_scrollChars = 2;
    m_scrollPos = 0;
    // timeouts are in seconds
    m_ibusLabelsTimeout = 30;
    m_ibusStatusTimeout = 180;
    m_browseTimeout = 15;
}

MsgProcessor::~ MsgProcessor()
{
}

void* startIBusCntr(void* arg)
{
    assert(arg);

    IBusCntr* ibus = (IBusCntr*) arg;
    ibus->run();
    return NULL;
}

bool MsgProcessor::init(IBATimers& timers)
{
	Log("Initializing message processor.", IBALogger::LOGS_DEBUG);
	m_timers = &timers; // timers must be initialized by caller

	// initialize components
	if(!m_alsa.init())
		return false;
	if(!m_lib.Init(m_alsa))
		return false;
	if(!m_ibus.init(*m_timers))
		return false;

	m_ibusLabelsTimeout = GetConfigValue<int>(PRMS_IBUS_LABELS_TIMEOUT);
	m_ibusStatusTimeout = GetConfigValue<int>(PRMS_IBUS_STATUS_TIMEOUT);
	m_browseTimeout = GetConfigValue<int>(PRMS_BROWSE_TIMEOUT);

    int delay = GetConfigValue<int>(PRMS_IBUS_RESPONSE_DELAY);
    m_respDelay = delay * 1000;

    m_scrollTrackInfo = GetConfigValue<bool>(PRMS_SCROLL_TRACK_INFO);
    m_scrollChars = GetConfigValue<int>(PRMS_TRACK_INFO_SCROLL_CHARS);
    float cfgSec = GetConfigValue<float>(PRMS_TRACK_POS_IVL);
    m_timers->setTimer(IBATimers::TI_TRACK_POS, cfgSec, this);

    cfgSec = GetConfigValue<float>(PRMS_TRACK_INFO_IVL);
    m_timers->setTimer(IBATimers::TI_TRACK_INFO, cfgSec, this);

	m_lib.LoadPlayQueue();
	m_alsa.pause(); // wait until request is received from radio

    // start a separate thread for ibus serial handling
    Log("Preparing ibus thread.", IBALogger::LOGS_DEBUG);
    pthread_attr_t thAttr;
    pthread_attr_init(&thAttr);
    pthread_attr_setdetachstate(&thAttr, PTHREAD_CREATE_DETACHED);
    pthread_create(&m_threadIBusCntr, &thAttr, startIBusCntr, &m_ibus);

    return true;
}

void MsgProcessor::onTimer(IBATimers::timerID tid)
{
    switch(tid)
    {
        case IBATimers::TI_TRACK_POS:
            m_outPos = true;
            break;

        case IBATimers::TI_TRACK_INFO:
            m_outInfo = true;
            break;
    }
}

void MsgProcessor::run()
{
    cout << "Main thread running...\n";
    m_timers->start(IBATimers::TI_TRACK_POS);
    m_timers->start(IBATimers::TI_TRACK_INFO);

    while(true)
    {
        outTrackPosition();
        outTrackInfo();
        checkForMsg();
        pthread_yield();
    }
}

void MsgProcessor::checkWatches()
{
    time_t now;
    time(&now);

    // occasionally the radio refreshes the labels to their
    // originals. So every now and then I refresh them to mine
    if(now - m_lblTime > m_ibusLabelsTimeout)
        setMIDLabels();

    // Return to play mode after a time out
    if(MODE_BROWSE == m_mode && now - m_browseTime > m_browseTimeout)
    {
    	m_mode = MODE_PLAY;
    }

    // this is an attempt to prevent the radio for sporadically disconnecting
    // the CD Changer audio source. Not sure if it helps at all
    if(m_cdc_playing && now - m_statusTime > m_ibusStatusTimeout)
        sendStatus();
}

void MsgProcessor::outTrackPosition()
{
    if(!m_outPos)
		return;

    m_outPos = false;

    if(!(m_mode == MODE_PLAY || m_mode == MODE_BROWSE))
		return;

    char* posInfo = NULL;
    if(NULL != (posInfo = m_alsa.getTimeInfo()))
        m_ibus.writeRadio(posInfo);
    else
        m_ibus.writeRadio("--:--/--:--");

    checkWatches();
}

void MsgProcessor::outTrackInfo()
{
    if(!m_outInfo)
		return;

    m_outInfo = false;

    if(MODE_PLAY != m_mode)
		return;

    if(m_scrollTrackInfo)
        scrollTrackInfo();
    else
        alternateTrackInfo();

}


void MsgProcessor::clearOBC()
{
    m_ibus.writeOBC(" ");
}

void MsgProcessor::resetOBC()
{
    m_ibus.sendPacket(IBUS_DEV_IKE, IBUS_DEV_OBC, IBUS_RESET_OBC_LCD, sizeof(IBUS_RESET_OBC_LCD));
}

void MsgProcessor::setMIDLabels()
{
    // wait a bit before setting up the labels to be sure to overwrite any the radio sets.
    //usleep(150000);

    time(&m_lblTime);

    switch(m_mode)
    {
        case MODE_PLAY:
        case MODE_BROWSE:
            //resetOBC();
            m_ibus.sendPacket(IBUS_DEV_RADIO, IBUS_DEV_MID, IBUS_LBLS1, sizeof(IBUS_LBLS1));
            m_ibus.sendPacket(IBUS_DEV_RADIO, IBUS_DEV_MID, IBUS_LBLS2, sizeof(IBUS_LBLS2));
            break;

        case MODE_INACTIVE:
            //resetOBC();
            clearOBC();
            break;

        default:
            return;
    }
}

void MsgProcessor::sendStatus()
{
    Log("Send status.", IBALogger::LOGS_DEBUG);
    time(&m_statusTime);
    if(m_cdc_playing)
        m_ibus.sendPacket(IBUS_PACK_PLAYING_1_1, sizeof(IBUS_PACK_PLAYING_1_1));
    else
        m_ibus.sendPacket(IBUS_PACK_NOT_PLAYING_0_0, sizeof(IBUS_PACK_NOT_PLAYING_0_0));
}

void MsgProcessor::processIBUSmsg(void* msg)
{
    assert(msg);
    IBusMsg* ibusMsg = (IBusMsg*) msg;
    assert(IBUS_DEV_RADIO == ibusMsg->devFrom && IBUS_DEV_CD_CHANGER == ibusMsg->devTo);

    usleep(m_respDelay); // radio misses fast responces

    int datalen = sizeof(IBUS_DATA_INFO_CDC);
    int button = 0;
    if(0 == memcmp(ibusMsg->data, IBUS_DATA_INFO_CDC, datalen))
    {
        Log("Radio: CDC Info.", IBALogger::LOGS_DEBUG);
        sendStatus();
        return;
    }
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_START_PLAY, datalen))
    {
        Log("Radio: Start Play.", IBALogger::LOGS_DEBUG);
        m_ibus.sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
        m_ibus.sendPacket(IBUS_PACK_PLAYING_1_1, sizeof(IBUS_PACK_PLAYING_1_1));
		m_mode = MODE_PLAY;
		setMIDLabels();
		if(!m_alsa.isPlaying())
			m_alsa.play();
        if(!m_cdc_playing)
        {
            m_cdc_playing = true;
            m_ibus.setCDCplaying(m_cdc_playing);
        }
        return;
    }
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_STOP_PLAY, datalen))
    {
        Log("Radio: Stop Play.", IBALogger::LOGS_DEBUG);
        m_ibus.sendPacket(IBUS_PACK_NOT_PLAYING_0_0, sizeof(IBUS_PACK_NOT_PLAYING_0_0));
		m_mode = MODE_INACTIVE;
		setMIDLabels();
		if(m_alsa.isPlaying())
			m_alsa.pause();
        if(m_cdc_playing)
        {
            m_cdc_playing = false;
            m_ibus.setCDCplaying(m_cdc_playing);
        }
        return;
    }

    // translate these radio requests to button presssed
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_NEXT_TRACK, datalen))
        button = MID_BUTN_NEXT;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_PREV_TRACK, datalen))
        button = MID_BUTN_PREV;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_CD1_CDC, datalen))
        button = MID_BUTN_CD1;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_CD2_CDC, datalen))
        button = MID_BUTN_CD2;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_CD3_CDC, datalen))
        button = MID_BUTN_CD3;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_CD4_CDC, datalen))
        button = MID_BUTN_CD4;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_CD5_CDC, datalen))
        button = MID_BUTN_CD5;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_CD6_CDC, datalen))
        button = MID_BUTN_CD6;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_SCAN_ON, datalen))
        button = MID_SCAN_ON;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_SCAN_OFF, datalen))
        button = MID_SCAN_OFF;
    /*
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_FF, datalen))
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_REW, datalen))
    */
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_RANDOM_ON, datalen))
        button = MID_RANDOM_ON;
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_RANDOM_OFF, datalen))
        button = MID_RANDOM_OFF;

    if(button > 0)
        onButtonPress(button);
} // processIBUSmsg()

void MsgProcessor::onButtonPress(int button)
{
    assert(button > 0);

    // first give the expected response to the radio
    switch(button)
    {
        case MID_BUTN_NEXT:
        case MID_BUTN_PREV:
            //m_ibus.sendPacket(IBUS_PACK_END_PLAYING_1_1, sizeof(IBUS_PACK_END_PLAYING_1_1));
            //m_ibus.sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
            //break;

        case MID_BUTN_CD1:
        case MID_BUTN_CD2:
        case MID_BUTN_CD3:
        case MID_BUTN_CD4:
        case MID_BUTN_CD5:
        case MID_BUTN_CD6:
            //m_ibus.sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
            break;


    // The following are not the correct responses. These cause the radio to disconnect
    // the CD changer audio source
        case MID_RANDOM_ON:
            m_ibus.sendPacket(IBUS_PACK_END_PLAYING_1_1, sizeof(IBUS_PACK_END_PLAYING_1_1));
            m_ibus.sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
        case MID_RANDOM_OFF:
            m_ibus.sendPacket(IBUS_PACK_PLAYING_1_1, sizeof(IBUS_PACK_PLAYING_1_1));
            break;

        case MID_SCAN_ON:
        case MID_SCAN_OFF:
            //m_ibus.sendPacket(IBUS_PACK_END_PLAYING_1_1, sizeof(IBUS_PACK_END_PLAYING_1_1));
            m_ibus.sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
            break;
    }

    // then do my stuff
    switch(m_mode)
    {
        case MODE_PLAY:
			m_outPos = m_outInfo = true;
            handlePlayButton(button);
            break;

        case MODE_BROWSE:
			m_outPos = m_outInfo = false;
            handleBrowseButton(button);
            break;
    }
}


void MsgProcessor::handlePlayButton(int button)
{
    switch(button)
    {
    	// in play mode these directly control the alsa player
        case MID_BUTN_NEXT:
            Log("Play: Next.", IBALogger::LOGS_DEBUG);
            m_alsa.next();
            break;

        case MID_BUTN_PREV:
            Log("Play: Previous.", IBALogger::LOGS_DEBUG);
            m_alsa.previous();
            break;

        case MID_BUTN_CD5: // Play
            Log("Play: Play.", IBALogger::LOGS_DEBUG);
            m_alsa.play();
            break;

		// these switch back to browse mode
        case MID_BUTN_CD1: // Menu
        case MID_BUTN_CD2: // Select
        case MID_BUTN_CD3: // Page Down
        case MID_BUTN_CD4: // Page Up
        case MID_BUTN_CD6: // Add
            Log("Play: Browse mode.", IBALogger::LOGS_DEBUG);
            toBrowseMode();
            break;
    }
}

void MsgProcessor::handleBrowseButton(int button)
{
    const char* txt = NULL;

    switch(button)
    {
        case MID_BUTN_NEXT:
            Log("Browse: Next.", IBALogger::LOGS_DEBUG);
            txt = m_lib.Next().c_str();
            break;
        case MID_BUTN_PREV:
            Log("Browse: Previous.", IBALogger::LOGS_DEBUG);
            txt = m_lib.Prev().c_str();
            break;
        case MID_BUTN_CD1: // Menu
            Log("Browse: PageDn.", IBALogger::LOGS_DEBUG);
            txt = m_lib.Menu().c_str();
            break;
        case MID_BUTN_CD2: // Select
            Log("Browse: PageUp.", IBALogger::LOGS_DEBUG);
            txt = m_lib.Select().c_str();
            break;
        case MID_BUTN_CD3: // Page Down
            Log("Browse: Page Down.", IBALogger::LOGS_DEBUG);
            txt = m_lib.PgDn().c_str();
            break;
        case MID_BUTN_CD4: // Page Up
            Log("Browse: Page Up.", IBALogger::LOGS_DEBUG);
            txt = m_lib.PgUp().c_str();
            break;
        case MID_BUTN_CD5: // Play
            Log("Browse: Play.", IBALogger::LOGS_DEBUG);
            m_lib.Play();
            m_mode = MODE_PLAY;
            break;
        case MID_BUTN_CD6: // Add
            Log("Browse: Add.", IBALogger::LOGS_DEBUG);
            m_lib.Add();
            m_ibus.writeRadio("   Added   ");
            break;
    }

    if(NULL != txt)
    {
        Log(txt, IBALogger::LOGS_DEBUG);
        m_ibus.writeOBC(txt);
    }

    time(&m_browseTime);
}


void MsgProcessor::toBrowseMode()
{
    const char* txt = NULL;

    Log("To browse mode.", IBALogger::LOGS_DEBUG);
	m_mode = MODE_BROWSE;
    txt = m_lib.CurrentItem().Name.c_str();
    time(&m_browseTime);
    if(NULL == txt)
    {
        Log("No text from CurrentItem.", IBALogger::LOGS_DEBUG);
        return;
    }

    Log(txt, IBALogger::LOGS_DEBUG);
    m_ibus.writeOBC(txt);
}

void MsgProcessor::checkForMsg()
{
    IBusMsgQueue& msgQ = IBusMsgQueue::getQueue();
    if(!msgQ.hasMsg())
        return;

    IBusMsg msg;
    while(NULL != msgQ.get(&msg))
        processIBUSmsg(&msg);
}


void MsgProcessor::alternateTrackInfo()
{
    char* trackInfo;
    bool skip = false;

    // will rotate through title, artist, album, misc info
    switch(m_dispInfoNdx)
    {
        case 0: // title
            trackInfo = m_alsa.getTitle();
            break;

        case 1: // artist
            trackInfo = m_alsa.getArtist();
            break;

        case 2: // album
            trackInfo = m_alsa.getAlbum();
            break;

        default: // 3 misc info
            trackInfo = m_alsa.getMiscInfo();
    }

    if(!trackInfo[0]) // empty string
    {
        trackInfo = m_alsa.getFileName();
        skip = true;
    }

    if(!displayWithShift(trackInfo))
        m_dispInfoNdx = skip ? 3 : (m_dispInfoNdx + 1) % 4;
}

bool MsgProcessor::displayWithShift(char* txt)
{
    // returns true if txt needs to be shifted, i.e. call this method again
    assert(txt);

    int txtlen = strlen(txt);
    if(txtlen <= IBusCntr::OBC_LCD_LEN)
    {
        m_ibus.writeOBC(txt);
        m_scrollPos = 0;
        return false;
    }

    if(m_scrollPos + IBusCntr::OBC_LCD_LEN > txtlen - 1)
    {
        // that would be the last part for this txt
        m_scrollPos = txtlen - IBusCntr::OBC_LCD_LEN;
        m_ibus.writeOBC(txt + m_scrollPos);
        m_scrollPos = 0;
        return false;
    }

    m_ibus.writeOBC(txt + m_scrollPos);

    m_scrollPos += IBusCntr::OBC_LCD_LEN - m_scrollChars;
    return true;
}


void MsgProcessor::scrollTrackInfo()
{
    char* trackInfo = m_alsa.getInfo();
    if(strlen(trackInfo) <= IBusCntr::OBC_LCD_LEN)
    {
        m_ibus.writeOBC(trackInfo);
        return;
    }

    char scroll[IBusCntr::OBC_LCD_LEN + 1];
    memset(scroll, 0, IBusCntr::OBC_LCD_LEN + 1);
    m_ibus.writeOBC(scrollTxt(scroll, trackInfo));
}


char* MsgProcessor::scrollTxt(char* scrollBuf, char* txt)
{
    assert(scrollBuf);
    assert(txt);

    int len = strlen(txt);
    if(m_scrollPos >= len)
        m_scrollPos = 0;

    int tail = len - m_scrollPos;
    if(tail >= IBusCntr::OBC_LCD_LEN)
    {
        memcpy(scrollBuf, txt + m_scrollPos, IBusCntr::OBC_LCD_LEN);
    }
    else
    {
        memcpy(scrollBuf, txt + m_scrollPos, tail);
        int head = IBusCntr::OBC_LCD_LEN - tail;
        memcpy(scrollBuf + tail, txt, head);
    }

    m_scrollPos += m_scrollChars;

    return scrollBuf;
}

