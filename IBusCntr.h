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
#ifndef _IBUS_CNTR_H_
#define _IBUS_CNTR_H_

#include "IBusPort.h"
#include "IBATimers.h"

// source device addresses of interest
#define IBUS_DEV_BROADCAST        0xFF
#define IBUS_DEV_BROADCASTX       0x00
#define IBUS_DEV_CD_CHANGER       0x18
#define IBUS_DEV_WHEEL            0x50
#define IBUS_DEV_RADIO            0x68
#define IBUS_DEV_IKE              0x80
#define IBUS_DEV_LCM              0xBF
#define IBUS_DEV_MID              0xC0
#define IBUS_DEV_OBC              0xE7
#define IBUS_DEV_LIGHTS           0xED
#define IBUS_DEV_BMB              0xF0

#define IBUS_LOG_LEVEL_NONE         0
#define IBUS_LOG_LEVEL_MIN          1
#define IBUS_LOG_LEVEL_MAX          2

#define IBUS_POLL_WATCH_TIMEOUT     45     //seconds

static const char* IBUS_DEVT_BROADCAST        = "Broadcast";
static const char* IBUS_DEVT_BROADCASTX       = "BroadcastX";
static const char* IBUS_DEVT_CD_CHANGER       = "CD Changer";
static const char* IBUS_DEVT_WHEEL            = "Wheel";
static const char* IBUS_DEVT_RADIO            = "Radio";
static const char* IBUS_DEVT_IKE              = "IKE";
static const char* IBUS_DEVT_MID              = "MID";
static const char* IBUS_DEVT_LCM              = "Lights Control";
static const char* IBUS_DEVT_OBC              = "OBC";
static const char* IBUS_DEVT_LIGHTS           = "Lights and Seats";
static const char* IBUS_DEVT_BMB              = "BMB";
static const char* IBUS_DEVT_UNKNOWN          = "Unknown";


static const byte IBUS_ALL_DEV[]    = {IBUS_DEV_CD_CHANGER,IBUS_DEV_WHEEL,IBUS_DEV_RADIO,IBUS_DEV_IKE,IBUS_DEV_LCM,
                                       IBUS_DEV_MID,IBUS_DEV_OBC,IBUS_DEV_LIGHTS,IBUS_DEV_BMB,
                                       IBUS_DEV_BROADCAST,IBUS_DEV_BROADCASTX};
static const int  IBUS_ALL_DEV_COUNT = sizeof(IBUS_ALL_DEV);

static const byte IBUS_LOG_DEV[] = {IBUS_DEV_RADIO,IBUS_DEV_MID,IBUS_DEV_IKE,IBUS_DEV_CD_CHANGER};
static const int  IBUS_LOG_DEV_COUNT = sizeof(IBUS_LOG_DEV);

// fixed packets, ready to send
// the player will behave as having one disk with one track
static const byte IBUS_PACK_ANNOUNCE[] = {IBUS_DEV_CD_CHANGER, 0x04, IBUS_DEV_BROADCAST,
                                          0x02, 0x01,
                                          0xE0};
static const byte IBUS_PACK_RESPOND_POLL[] = {IBUS_DEV_CD_CHANGER, 0x04, IBUS_DEV_BROADCAST,
                                              0x02, 0x00,
                                              0xE1};
static const byte IBUS_PACK_PLAYING_1_1[] = {IBUS_DEV_CD_CHANGER, 0x0A, IBUS_DEV_RADIO,    // playing disk 1 track 1, always...
                                             0x39,0x00,0x09,0x00,0x3F,0x00,0x01,0x01,
                                             0x75};
static const byte IBUS_PACK_NOT_PLAYING_0_0[] = {IBUS_DEV_CD_CHANGER, 0x0A, IBUS_DEV_RADIO,
                                             0x39,0x00,0x02,0x00,0x3F,0x00,0x00,0x00,
                                             0x7E};
/*static const byte IBUS_PACK_NOT_PLAYING_1_1[] = {IBUS_DEV_CD_CHANGER, 0x0A, IBUS_DEV_RADIO,
                                             0x39,0x00,0x02,0x00,0x3F,0x00,0x01,0x01,
                                             0x7E};*/
static const byte IBUS_PACK_START_PLAYING_1_1[] = {IBUS_DEV_CD_CHANGER, 0x0A, IBUS_DEV_RADIO,
                                             0x39,0x02,0x09,0x00,0x3F,0x00,0x01,0x01,
                                             0x77};
static const byte IBUS_PACK_END_PLAYING_1_1[] = {IBUS_DEV_CD_CHANGER, 0x0A, IBUS_DEV_RADIO,
                                             0x39,0x07,0x09,0x00,0x3F,0x00,0x01,0x01,
                                             0x72};
static const byte IBUS_PACK_SCAN_FORE_1_1[] = {IBUS_DEV_CD_CHANGER, 0x0A, IBUS_DEV_RADIO,
                                             0x39,0x03,0x09,0x00,0x3F,0x00,0x01,0x01,
                                             0x76};
static const byte IBUS_PACK_SCAN_BACK_1_1[] = {IBUS_DEV_CD_CHANGER, 0x0A, IBUS_DEV_RADIO,
                                             0x39,0x04,0x09,0x00,0x3F,0x00,0x01,0x01,
                                             0x71};
static const byte IBUS_PACK_SEEK_1_1[] = {IBUS_DEV_CD_CHANGER, 0x0A, IBUS_DEV_RADIO,
                                             0x39,0x08,0x09,0x00,0x3F,0x00,0x01,0x01,
                                             0x7D};

// fixed packet data to respond to
static const byte IBUS_DATA_POLL_CDC[]      = {0x01};
static const byte IBUS_DATA_INFO_CDC[]      = {0x38,0x00,0x00};    // query disc and track
static const byte IBUS_DATA_STOP_PLAY[]     = {0x38,0x01,0x00};
static const byte IBUS_DATA_START_PLAY[]    = {0x38,0x03,0x00};
static const byte IBUS_DATA_FF[]            = {0x38,0x04,0x00};
static const byte IBUS_DATA_REW[]           = {0x38,0x04,0x01};
static const byte IBUS_DATA_CD1_CDC[]       = {0x38,0x06,0x01};
static const byte IBUS_DATA_CD2_CDC[]       = {0x38,0x06,0x02};
static const byte IBUS_DATA_CD3_CDC[]       = {0x38,0x06,0x03};
static const byte IBUS_DATA_CD4_CDC[]       = {0x38,0x06,0x04};
static const byte IBUS_DATA_CD5_CDC[]       = {0x38,0x06,0x05};
static const byte IBUS_DATA_CD6_CDC[]       = {0x38,0x06,0x06};
static const byte IBUS_DATA_SCAN_ON[]       = {0x38,0x07,0x00};
static const byte IBUS_DATA_SCAN_OFF[]      = {0x38,0x07,0x01};
static const byte IBUS_DATA_RANDOM_ON[]     = {0x38,0x08,0x00};
static const byte IBUS_DATA_RANDOM_OFF[]    = {0x38,0x08,0x01};
static const byte IBUS_DATA_NEXT_TRACK[]    = {0x38,0x0A,0x00};
static const byte IBUS_DATA_PREV_TRACK[]    = {0x38,0x0A,0x01};

class IBusCntr : public IBusPortListener, IBATimerListener
{
public:
    IBusCntr();
    ~IBusCntr();
    
    bool            init(IBAConfig&, IBALogger&, IBATimers&);
    void            run();
    void            setCDCplaying(bool);
    virtual void    haveData(byte* buffer, int len);
    virtual void    onTimer(IBATimers::timerID);
    
    void            sendPacket(byte fromDev, byte toDev, const byte* data, int datalen);
    void            sendPacket(const byte* packet, int packet_size);
    
    void            writeRadio(const char*);
    void            writeOBC(const char*);
   
    static const char*      getDevTxt(byte d);

    static const byte THIS_DEVICE = IBUS_DEV_CD_CHANGER;
        
    // the shortest packet is 5 bytes: src|len|dest|info_byte|chk
    static const int  MIN_PACKET_SIZE = 5;
    static const int  MIN_PACKET_LEN = MIN_PACKET_SIZE - 2; // min value for len field
    
    // longest packet I've seen was 32 bytes
    static const int  MAX_PACKET_SIZE = 64; // twice for slack
    static const int  MAX_PACKET_LEN = MAX_PACKET_SIZE - 2;
    
    static const int  RADIO_LCD_LEN = 11;
    static const int  OBC_LCD_LEN   = 20;
    
private:
    void             startAnnounce();
    void             stopAnnounce();
    void             parseForPacket();
    void             shiftRcvBuffer(int from);
    byte             calcSum(byte* buff, int len);
    bool             checkSum(byte* buff, int len);
    void             processRcvPacket();
    void             pfmt(ostream& out, const byte* p);
    
    
    IBAConfig*       m_config;
    IBALogger*       m_logger;
    IBATimers*       m_timer;
    
    IBusPort         m_ibus;
    bool             m_cdc_playing;
    int              m_ibusLogLevel;
    unsigned long    m_respDelay;
    time_t           m_pollWatch;
    
    //bool             m_isPolled;
    
    static const int  RECEIVE_BUFFER_SIZE = 128;
    byte              m_receiveBuff[RECEIVE_BUFFER_SIZE];
    int               m_head; // position to start parsing from
    int               m_tail; // position to start inserting from, post-increment
    
    byte              m_inPacket[MAX_PACKET_SIZE];
    //byte              m_outPacket[MAX_PACKET_SIZE];
    
    static const int  POFF_SRC = 0; // source offset in packet
    static const int  POFF_LEN = 1;
    static const int  POFF_DEST = 2;
    static const int  POFF_DATA = 3;
    
};

#endif //_IBUS_CNTR_H_

