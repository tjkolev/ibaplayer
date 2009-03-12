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

#include "IBAPlayer.h"
#include "IBusMsg.h"
#include <assert.h>


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

// play mode 
// [Art  Alb] [Trk  Gen] [        ] [FM    AM] [     Rnd] [    MODE]
//  1     2     3    4
static const byte IBUS_LBLS_PLAY1[] =
{0x21,0x00,0x00,0x60, 'A','r','t',' ',0x05,' ','A','l','b',0x05,'T','r','k',' ',0x05,' ','G','e','n',0x05,' ',' ',' ',' ',0x05,' ',' ',' ',' '};
static const byte IBUS_LBLS_PLAY2_RNDON[] =
{0x21,0x00,0x00,0x06, 'F','M',' ',' ',0x05,' ',' ','A','M',0x05,' ',' ',' ',' ',0x05,'*','R','n','d',0x05,' ',' ',' ',' ',0x05,'M','O','D','E'};
static const byte IBUS_LBLS_PLAY2_RNDOFF[] =
{0x21,0x00,0x00,0x06, 'F','M',' ',' ',0x05,' ',' ','A','M',0x05,' ',' ',' ',' ',0x05,' ','R','n','d',0x05,' ',' ',' ',' ',0x05,'M','O','D','E'};

// browse mode
// [  Page  ] [  Levl  ] [Play Add] [FM   AM] [   Esc] [    MODE]
//  1     2     3    4    5    6
static const byte IBUS_LBLS_BROWSE1[] =  // 0xC1 is displayed as inverted ^ for 'down'
{0x21,0x00,0x00,0x60, 0xC1,' ','P','a',0x05,'g','e',' ','^',0x05,0xC1,' ','L','e',0x05,'v','l',' ','^',0x05,'P','l','a','y',0x05,' ','A','d','d'};
static const byte IBUS_LBLS_BROWSE2[] =
{0x21,0x00,0x00,0x06, 'F','M',' ',' ',0x05,' ',' ','A','M',0x05,' ',' ',' ',' ',0x05,' ','E','s','c',0x05,' ',' ',' ',' ',0x05,'M','O','D','E'};

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

#define IBA_LABELS_TIMEOUT      30      // seconds
#define IBA_BROWSE_TIMEOUT      15
#define IBA_STATUS_TIMEOUT      180


MsgProcessor::MsgProcessor()
{
    m_mode = MODE_INACTIVE;
    m_cdc_playing = false;
    m_iba = NULL;
    m_alsa = NULL;
    m_ibus = NULL;
    m_lib = NULL;
    m_log = NULL;
    m_respDelay = 100000;
    m_outPos = false;
    m_outInfo = false;
    m_dispInfoNdx = 0;
    m_scrollTrackInfo = false;
    m_scrollChars = 2;
    m_scrollPos = 0;
    m_repeatOn = false;
    m_randomOn = false;
}

MsgProcessor::~ MsgProcessor()
{
}

bool MsgProcessor::init(IBAPlayer& iba)
{
    m_iba = &iba;
    m_alsa = &iba.getPlayer();
    m_ibus = &iba.getIBusCntr();
    m_lib = &iba.getLibMngr().getBrowser();
    m_log = &iba.getLog();
    
    IBAConfig& cfg = iba.getConfig();
    
    int delay;
    cfg.getValue(IBAConfig::PRM_IBUS_RESPONSE_DELAY, delay);
    m_respDelay = delay * 1000;
    
    cfg.getValue(IBAConfig::PRM_SCROLL_TRACK_INFO, m_scrollTrackInfo);
    
    cfg.getValue(IBAConfig::PRM_TRACK_INFO_SCROLL_CHARS, m_scrollChars);
            
    float cfgSec;
    cfg.getValue(IBAConfig::PRM_TRACK_POS_IVL, cfgSec);
    iba.getTimers().setTimer(IBATimers::TI_TRACK_POS, cfgSec, this);
    
    cfg.getValue(IBAConfig::PRM_TRACK_INFO_IVL, cfgSec);
    iba.getTimers().setTimer(IBATimers::TI_TRACK_INFO, cfgSec, this);

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
    m_iba->getTimers().start(IBATimers::TI_TRACK_POS);
    m_iba->getTimers().start(IBATimers::TI_TRACK_INFO);
    
    unsigned long l = 0;
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
    if(now - m_lblTime > IBA_LABELS_TIMEOUT)
        setMIDLabels(m_mode);
       
    // Since the Esc from browse mode does not work as I want
    // it, return to play mode after a time out
    if(MODE_BROWSE == m_mode && now - m_browseTime > IBA_BROWSE_TIMEOUT)
        setMIDLabels(MODE_PLAY);
    
    // this is an attempt to prevent the radio for sporadically disconnecting
    // the CD Changer audio source. Not sure if it helps at all        
    if(m_cdc_playing && now - m_statusTime > IBA_STATUS_TIMEOUT)
        sendStatus();
}

void MsgProcessor::outTrackPosition()
{
    if(!m_outPos) return;

    m_outPos = false;
    
    if(!(m_mode == MODE_PLAY || m_mode == MODE_BROWSE)) return;
    
    char* posInfo = NULL;
    if(m_alsa->isPlaying() && NULL != (posInfo = m_alsa->getTimeInfo()))
        m_ibus->writeRadio(posInfo);
    else
        m_ibus->writeRadio("--:--/--:--");
        
    checkWatches();
}

void MsgProcessor::outTrackInfo()
{
    if(!m_outInfo) return;
    
    m_outInfo = false;
    
    if(MODE_PLAY != m_mode) return;
    
    if(m_scrollTrackInfo)
        scrollTrackInfo();
    else
        alternateTrackInfo();
    
}


void MsgProcessor::clearOBC()
{
    m_ibus->writeOBC(" ");
}

void MsgProcessor::resetOBC()
{
    m_ibus->sendPacket(IBUS_DEV_IKE, IBUS_DEV_OBC, IBUS_RESET_OBC_LCD, sizeof(IBUS_RESET_OBC_LCD));
}

void MsgProcessor::setMIDLabels(mode m)
{
    // wait a bit before setting up the labels to be sure to overwrite any the radio sets.
    usleep(150000);
    
    time(&m_lblTime);
    
    switch(m)
    {
        case MODE_PLAY:
            //resetOBC();
            
            //m_log->log("Switching to Play display.", IBALogger::LOGS_DEBUG);
            m_ibus->sendPacket(IBUS_DEV_RADIO, IBUS_DEV_MID, IBUS_LBLS_PLAY1, sizeof(IBUS_LBLS_PLAY1));
            if(m_randomOn)
                m_ibus->sendPacket(IBUS_DEV_RADIO, IBUS_DEV_MID, IBUS_LBLS_PLAY2_RNDON, sizeof(IBUS_LBLS_PLAY2_RNDON));
            else
                m_ibus->sendPacket(IBUS_DEV_RADIO, IBUS_DEV_MID, IBUS_LBLS_PLAY2_RNDOFF, sizeof(IBUS_LBLS_PLAY2_RNDOFF));
                
            break;
            
        case MODE_BROWSE:
            //resetOBC();
            
            //m_log->log("Switching to Browse display.", IBALogger::LOGS_DEBUG);
            m_ibus->sendPacket(IBUS_DEV_RADIO, IBUS_DEV_MID, IBUS_LBLS_BROWSE1, sizeof(IBUS_LBLS_BROWSE1));
            m_ibus->sendPacket(IBUS_DEV_RADIO, IBUS_DEV_MID, IBUS_LBLS_BROWSE2, sizeof(IBUS_LBLS_BROWSE2));
            
            break;
            
        case MODE_INACTIVE:
            //resetOBC();
            clearOBC();
            
            break;
        
        default:
            return;
    }
    
    m_mode = m;
}

void MsgProcessor::sendStatus()
{
    m_log->log("Send status.", IBALogger::LOGS_DEBUG);
    time(&m_statusTime);
    if(m_cdc_playing)
        m_ibus->sendPacket(IBUS_PACK_PLAYING_1_1, sizeof(IBUS_PACK_PLAYING_1_1));
    else
        m_ibus->sendPacket(IBUS_PACK_NOT_PLAYING_0_0, sizeof(IBUS_PACK_NOT_PLAYING_0_0));
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
        m_log->log("Radio: CDC Info.", IBALogger::LOGS_DEBUG);
        sendStatus();
        return;
    }
    
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_START_PLAY, datalen))
    {
        m_log->log("Radio: Start Play.", IBALogger::LOGS_DEBUG);
        m_ibus->sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
        m_ibus->sendPacket(IBUS_PACK_PLAYING_1_1, sizeof(IBUS_PACK_PLAYING_1_1));
        if(!m_cdc_playing)
        {
            startPlaying();
            setMIDLabels(MODE_PLAY);
            m_cdc_playing = true;
            m_ibus->setCDCplaying(m_cdc_playing);
        }
        return;
    }
    
    else if(0 == memcmp(ibusMsg->data, IBUS_DATA_STOP_PLAY, datalen))
    {
        m_log->log("Radio: Stop Play.", IBALogger::LOGS_DEBUG);
        m_ibus->sendPacket(IBUS_PACK_NOT_PLAYING_0_0, sizeof(IBUS_PACK_NOT_PLAYING_0_0));
        if(m_cdc_playing)
        {
            m_alsa->stop();
            setMIDLabels(MODE_INACTIVE);
            m_cdc_playing = false;
            m_ibus->setCDCplaying(m_cdc_playing);
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
    
          
// // //     // intercept MID keys
// // //     else if(MODE_INACTIVE != m_mode &&
// // //             IBUS_DEV_MID == ibusMsg->devFrom &&
// // //             IBUS_DEV_RADIO == ibusMsg->devTo)
// // //     {
// // //         m_log->log("Intercept MID to Radio message.", IBALogger::LOGS_DEBUG);
// // //         
// // //         // handling button release
// // //         if(ibusMsg->data[0] == 0x31 &&
// // //            ibusMsg->data[1] == 0x00 &&
// // //            (ibusMsg->data[3] & 0x40))
// // //         {
// // //             m_log->log("MID: Button release.", IBALogger::LOGS_DEBUG);
// // //             switch(m_mode)
// // //             {
// // //                 case MODE_PLAY:
// // //                     doPlayInput(ibusMsg->data[3] & 0x0F);
// // //                     break;
// // //                     
// // //                 case MODE_BROWSE:
// // //                     doBrowseInput(ibusMsg->data[3] & 0x0F);
// // //                     break;
// // //             }
// // //         }
// // //     } // if message from MID to Radio

} // processIBUSmsg()


void MsgProcessor::startPlaying()
{
    // this will make sure the player starts playing, because if not
    // the radio will disconnect it as audio source
    m_alsa->play();
    
    if(m_alsa->isPlaying())
        return;
    
    m_alsa->clear();
    m_alsa->add(m_lib->getDefaultPlaylist());
    m_alsa->setLoop(true);
    //m_alsa->setShuffle(m_randomOn);    
    m_alsa->play();
}


void MsgProcessor::onButtonPress(int button)
{
    assert(button > 0);

    // first give the expected response to the radio
    switch(button)
    {
        case MID_BUTN_NEXT:
        case MID_BUTN_PREV:
            //m_ibus->sendPacket(IBUS_PACK_END_PLAYING_1_1, sizeof(IBUS_PACK_END_PLAYING_1_1));
            //m_ibus->sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
            //break;
            
        case MID_BUTN_CD1:
        case MID_BUTN_CD2:
        case MID_BUTN_CD3:
        case MID_BUTN_CD4:
        case MID_BUTN_CD5:
        case MID_BUTN_CD6:
            //m_ibus->sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
            break;

       
    // The following are not the correct responses. These cause the radio to disconnect
    // the CD changer audio source                             
        case MID_RANDOM_ON:
            m_ibus->sendPacket(IBUS_PACK_END_PLAYING_1_1, sizeof(IBUS_PACK_END_PLAYING_1_1));
            m_ibus->sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
        case MID_RANDOM_OFF:
            m_ibus->sendPacket(IBUS_PACK_PLAYING_1_1, sizeof(IBUS_PACK_PLAYING_1_1));
            break;
            
        case MID_SCAN_ON:
        case MID_SCAN_OFF:
            //m_ibus->sendPacket(IBUS_PACK_END_PLAYING_1_1, sizeof(IBUS_PACK_END_PLAYING_1_1));
            m_ibus->sendPacket(IBUS_PACK_START_PLAYING_1_1, sizeof(IBUS_PACK_START_PLAYING_1_1));
            break;
    }
    
    // then do my stuff    
    switch(m_mode)
    {
        case MODE_PLAY:
            handlePlayButton(button);
            break;
            
        case MODE_BROWSE:
            handleBrowseButton(button);
            break;
    }
}


void MsgProcessor::handlePlayButton(int button)
{
    switch(button)
    {
        case MID_BUTN_NEXT:
            m_log->log("Play: Next.", IBALogger::LOGS_DEBUG);
            m_alsa->next();
            m_outPos = true;
            m_outInfo = true;
            break;
            
        case MID_BUTN_PREV:
            m_log->log("Play: Previous.", IBALogger::LOGS_DEBUG);
            m_alsa->previous();
            m_outPos = true;
            m_outInfo = true;
            break;
            
        case MID_BUTN_CD1: // Brwose: Art
            m_log->log("Play: Browse artist.", IBALogger::LOGS_DEBUG);
            toBrowseMode(MusicLibBrowser::BR_LEVEL_ARTIST);
            break;
        case MID_BUTN_CD2: // Browse: Alb
            m_log->log("Play: Browse album.", IBALogger::LOGS_DEBUG);
            toBrowseMode(MusicLibBrowser::BR_LEVEL_ALBUM);
            break;
        case MID_BUTN_CD3: // Browse: Trk
            m_log->log("Play: Browse track.", IBALogger::LOGS_DEBUG);
            toBrowseMode(MusicLibBrowser::BR_LEVEL_TRACK);
            break;
        case MID_BUTN_CD4: // Browse: Gen
            m_log->log("Play: Browse genre.", IBALogger::LOGS_DEBUG);
            toBrowseMode(MusicLibBrowser::BR_LEVEL_GENRE);
            break;
            
        case MID_RANDOM_ON: // Random
            m_log->log("Play: Random on.", IBALogger::LOGS_DEBUG);
            m_randomOn = true;
            //m_alsa->setShuffle(m_randomOn);
            setMIDLabels(MODE_PLAY); // refresh labels
            break;
            
        case MID_RANDOM_OFF: // Random
            m_log->log("Play: Random off.", IBALogger::LOGS_DEBUG);
            m_randomOn = false;
            //m_alsa->setShuffle(m_randomOn);
            setMIDLabels(MODE_PLAY); // refresh labels
            break;
    }
}

void MsgProcessor::handleBrowseButton(int button)
{
    const char* txt = NULL;
    
    switch(button)
    {
        case MID_BUTN_NEXT:
            m_log->log("Browse: Next.", IBALogger::LOGS_DEBUG);
            txt = m_lib->next().c_str();
            break;
            
        case MID_BUTN_PREV:
            m_log->log("Browse: Previous.", IBALogger::LOGS_DEBUG);
            txt = m_lib->prev().c_str();
            break;
            
        case MID_BUTN_CD1: // pageDn
            m_log->log("Browse: PageDn.", IBALogger::LOGS_DEBUG);
            txt = m_lib->pgDn().c_str();
            break;
        case MID_BUTN_CD2: // pageUp
            m_log->log("Browse: PageUp.", IBALogger::LOGS_DEBUG);
            txt = m_lib->pgUp().c_str();
            break;
        case MID_BUTN_CD3: // DrillIn
            m_log->log("Browse: LevelDn.", IBALogger::LOGS_DEBUG);
            txt = m_lib->lvlDn().c_str();
            break;
        case MID_BUTN_CD4: // Back
            m_log->log("Browse: LevelUp.", IBALogger::LOGS_DEBUG);
            txt = m_lib->lvlUp().c_str();
            break;
        case MID_BUTN_CD5: // play
            m_log->log("Browse: Play.", IBALogger::LOGS_DEBUG);
            m_alsa->clear();
            m_alsa->add(m_lib->select());
            m_alsa->setLoop(true);
            //m_alsa->setShuffle(m_randomOn);
            //m_alsa->play(); // this will still start playing the current track
            m_alsa->next(); // this will skip to the first track of the new selectio and start playing
            setMIDLabels(MODE_PLAY);
            break;
        case MID_BUTN_CD6: // add
            m_log->log("Browse: Add.", IBALogger::LOGS_DEBUG);
            m_alsa->add(m_lib->select());
            m_ibus->writeRadio("   Added   ");
            break;
                        
        case MID_RANDOM_ON: // Esc
        case MID_RANDOM_OFF:
            m_log->log("Browse: Esc.", IBALogger::LOGS_DEBUG);
            setMIDLabels(MODE_PLAY); // labels refreshed
            break;
    }
    
    if(NULL != txt)
    {
        m_log->log(txt, IBALogger::LOGS_DEBUG);
        m_ibus->writeOBC(txt);
    }
    
    time(&m_browseTime);
}


void MsgProcessor::toBrowseMode(MusicLibBrowser::BrowseLevel lvl)
{
    const char* txt = NULL;

    m_log->log("To browse mode.", IBALogger::LOGS_DEBUG);
        
    setMIDLabels(MODE_BROWSE);
    txt = m_lib->setStartLevel(lvl).c_str();
    time(&m_browseTime);
    if(NULL == txt)
    {
        m_log->log("No text from setStartLevel().", IBALogger::LOGS_DEBUG);
        return;
    }
    
    m_log->log(txt, IBALogger::LOGS_DEBUG);
    m_ibus->writeOBC(txt);
}

void MsgProcessor::checkForMsg()
{
    IBusMsgQueue& msgQ = IBusMsgQueue::getQueue();
    if(!msgQ.hasMsg())
    {
        //cout << "checkForMsg(): No message in queue.\n";
        return;
    }
    
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
            trackInfo = m_alsa->getTitle();
            break;
            
        case 1: // artist
            trackInfo = m_alsa->getArtist();
            break;
            
        case 2: // album
            trackInfo = m_alsa->getAlbum();
            break;
            
        default: // 3 misc info
            trackInfo = m_alsa->getMiscInfo();
    }
    
    if(!trackInfo[0]) // empty string
    {
        trackInfo = m_alsa->getFileName();
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
        m_ibus->writeOBC(txt);
        m_scrollPos = 0;
        return false;
    }

    if(m_scrollPos + IBusCntr::OBC_LCD_LEN > txtlen - 1)
    {
        // that would be the last part for this txt
        m_scrollPos = txtlen - IBusCntr::OBC_LCD_LEN;
        m_ibus->writeOBC(txt + m_scrollPos);
        m_scrollPos = 0;
        return false;
    }
    
    m_ibus->writeOBC(txt + m_scrollPos);
    
    m_scrollPos += IBusCntr::OBC_LCD_LEN - m_scrollChars;    
    return true;
}


void MsgProcessor::scrollTrackInfo()
{
    char* trackInfo = m_alsa->getInfo();
    if(strlen(trackInfo) <= IBusCntr::OBC_LCD_LEN)
    {
        m_ibus->writeOBC(trackInfo);
        return;
    }
    
    char scroll[IBusCntr::OBC_LCD_LEN + 1];
    memset(scroll, 0, IBusCntr::OBC_LCD_LEN + 1);
    m_ibus->writeOBC(scrollTxt(scroll, trackInfo));
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

