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

#include "IBusCntr.h"
#include "IBusMsg.h"

IBusCntr::IBusCntr()
{
    m_ibusLogLevel    = IBUS_LOG_LEVEL_NONE;
    m_head            = 0;
    m_tail            = m_head; // all consumed
    //m_isPolled        = false;
    m_respDelay       = 100000;
    m_cdc_playing     = false;
}

IBusCntr::~IBusCntr()
{
    m_ibus.closePort();
}

bool IBusCntr::init(IBAConfig& config, IBALogger& logger, IBATimers& timer)
{
    m_config = &config;
    m_logger = &logger;
    
    m_config->getValue(IBAConfig::PRM_LOG_IBUS, m_ibusLogLevel);
    int delay;
    m_config->getValue(IBAConfig::PRM_IBUS_RESPONSE_DELAY, delay);
    m_respDelay = delay * 1000;

    m_ibus.regListener(*this);
    
    if(!m_ibus.init(config, logger))
        return false;
    
    m_timer = &timer;
    float cfgSec = 10;
    m_config->getValue(IBAConfig::PRM_IBUS_ANNOUNCE_IVL, cfgSec);
    m_timer->setTimer(IBATimers::TI_ANNOUNCE, cfgSec, this);
    
    return true;
}

void IBusCntr::onTimer(IBATimers::timerID tid)
{
    if(tid != IBATimers::TI_ANNOUNCE) return;
    
    //m_logger->log("Sending CDC announcement.", IBALogger::LOGS_DEBUG);
    m_ibus.send(IBUS_PACK_ANNOUNCE, sizeof(IBUS_PACK_ANNOUNCE));
}

void IBusCntr::stopAnnounce()
{
    m_timer->stop(IBATimers::TI_ANNOUNCE);
    
}

void IBusCntr::startAnnounce()
{
    m_timer->start(IBATimers::TI_ANNOUNCE);
    
}

void IBusCntr::run()
{
    time(&m_pollWatch);
    
    m_ibus.send(IBUS_PACK_ANNOUNCE, sizeof(IBUS_PACK_ANNOUNCE));
    //m_logger->log("Initial CDC Announcement", IBALogger::LOGS_DEBUG);
    
    startAnnounce();
    
    while(true)
    {
        m_ibus.receive();
        pthread_yield();
    }
}

void IBusCntr::setCDCplaying(bool isPlaying)
{
    m_cdc_playing = isPlaying;
}

void IBusCntr::haveData(byte* buffer, int len)
{
    int space = RECEIVE_BUFFER_SIZE - m_tail;
    
    if(space <= 0)
    {
        m_logger->log("Receiver buffer overrun. Data lost.", IBALogger::LOGS_WARNING);
        return;
    }
    
    if(space < len)
        len = space;
    
    memcpy(m_receiveBuff + m_tail, buffer, len);
    m_tail += len;
    
    parseForPacket();
    
} //IBusCntr::haveData


void IBusCntr::shiftRcvBuffer(int from)
{
    assert(from >= 0);
    if(0 == from || from > m_tail) return;
    
    memcpy(m_receiveBuff, m_receiveBuff + from, m_tail - from);
    m_head = 0;
    m_tail -= from;
}

void IBusCntr::parseForPacket()
{
    // parse from head to tail.
    if(m_head == m_tail)
        return; // nothing to parse
    
    // format is src | len | dest | data | chk
    // len is from dest to chk inclusive
    // chk is XOR on previous bytes in packet

    // shortest packet is 5
    while(m_tail - m_head >= MIN_PACKET_SIZE)
    {
        // check the packet len
        int plen = *(m_receiveBuff + m_head + POFF_LEN);
        if(plen < MIN_PACKET_LEN || plen > MAX_PACKET_LEN)
        {
            m_head++;
            continue;
        }

        // check for acceptable source and destination
        // source dev can't be a broadcast one
        if(NULL == memchr(IBUS_ALL_DEV, *(m_receiveBuff + m_head + POFF_SRC),  IBUS_ALL_DEV_COUNT - 2) ||
           NULL == memchr(IBUS_ALL_DEV, *(m_receiveBuff + m_head + POFF_DEST), IBUS_ALL_DEV_COUNT))       
        {
            m_head++;
            continue;
        }
        
        // verify the checksum
        // if there's enough bytes in buffer
        int psize = plen + 2;
        if(m_tail - m_head < psize) // not enough bytes in buffer to work with
            break;
        
        if(checkSum(m_receiveBuff + m_head, psize))
        {
            // copy packet for processing
            memcpy(m_inPacket, m_receiveBuff + m_head, psize);
            m_head += psize;
            
            processRcvPacket();
        }
        else
        {
            m_head++;
        }
        
    } // while()
    
    // shift receiving buffer
    if(m_head)
        shiftRcvBuffer(m_head);
}

void IBusCntr::sendPacket(byte fromDev, byte toDev, const byte* data, int datalen)
{
    assert(data);
    assert(datalen > 0);
    
    byte outPacket[MAX_PACKET_SIZE];
    outPacket[POFF_SRC] = fromDev;
    outPacket[POFF_LEN] = datalen + 2;
    outPacket[POFF_DEST] = toDev;
    memcpy(outPacket + POFF_DATA, data, datalen);
    outPacket[datalen + 3] = calcSum(outPacket, datalen + 3);
    
    sendPacket(outPacket, datalen + 4);
}

void IBusCntr::sendPacket(const byte* packet, int packet_size)
{
    assert(packet);
    assert(packet_size > 0);
    

    if(!m_ibus.send(packet, packet_size))
    {
        unsigned long usec = m_respDelay/2;
        unsigned long usec_incr = usec;
        int attempt = 0;
        
        do
        {
            usleep(usec+=usec_incr);
        }
        while(!m_ibus.send(packet, packet_size) && attempt++ < 5);
    }
        
}

bool IBusCntr::checkSum(byte* buff, int len)
{
    return *(buff + (len - 1)) == calcSum(buff, len - 1);
}

byte IBusCntr::calcSum(byte* buff, int len)
{
    byte chk = 0x00;
    for(int off = 0; off < len; chk ^= *(buff + off), off++);
    return chk;
}



static char hexchar[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

char* hexTochar(const byte c, char* hex)
{
    assert(hex);
    *hex = hexchar[c/16];
    *(hex + 1) = hexchar[c%16];
    return hex;
}


void printBuff(const byte* buff, int len, char* lbl = NULL, ostream& out = cout)
{
    char hex[3];
    hex[2] = 0;
    
    if(lbl)
        out << lbl;
    for(int ndx = 0; ndx < len; ndx++)
        out << hexTochar(*(buff+ndx), hex);
    out << endl;
}

void IBusCntr::pfmt(ostream& out, const byte* p)
{
    if(!p)
        return;

    out << "From: " << getDevTxt(*(p+POFF_SRC));
    out << ", To: " << getDevTxt(*(p+POFF_DEST)) << endl; 
    
    int psize = *(p+POFF_LEN) + 2;
    printBuff(p, psize, "Packet: ", out);
                 out << ".           -[";
    for(int ndx = POFF_DATA; ndx < psize - 1; ndx++)
    {
        byte ch = *(p+ndx);
        if(ch <= 0x20)
            out << "  ";
        else
            out << ch << ' ';
    }
    
    out << "]-" << endl;
}

const char* IBusCntr::getDevTxt(byte d)
{
    switch(d)
    {
        case IBUS_DEV_BROADCAST:    return IBUS_DEVT_BROADCAST;
        case IBUS_DEV_BROADCASTX:   return IBUS_DEVT_BROADCASTX;
        case IBUS_DEV_CD_CHANGER:   return IBUS_DEVT_CD_CHANGER;
        case IBUS_DEV_WHEEL:        return IBUS_DEVT_WHEEL;
        case IBUS_DEV_RADIO:        return IBUS_DEVT_RADIO;
        case IBUS_DEV_IKE:          return IBUS_DEVT_IKE;
        case IBUS_DEV_LCM:          return IBUS_DEVT_LCM;
        case IBUS_DEV_MID:          return IBUS_DEVT_MID;
        case IBUS_DEV_OBC:          return IBUS_DEVT_OBC;
        case IBUS_DEV_LIGHTS:       return IBUS_DEVT_LIGHTS;
        case IBUS_DEV_BMB:          return IBUS_DEVT_BMB;
        default:                    return IBUS_DEVT_UNKNOWN;
    }
}

void IBusCntr::processRcvPacket()
{
    if(m_logger->getLogLevel() & IBALogger::LOGS_DEBUG)
    {
        if(IBUS_LOG_LEVEL_MAX == m_ibusLogLevel || 
        (IBUS_LOG_LEVEL_MIN == m_ibusLogLevel && memchr(IBUS_LOG_DEV, *(m_inPacket+POFF_SRC), IBUS_LOG_DEV_COUNT)))
        {
            ostringstream out;
            pfmt(out, m_inPacket);
            m_logger->log(out.str().c_str(), IBALogger::LOGS_DEBUG);
        }
    }
    
    // Only listen to requests from the radio
    // and stop announcing
    if(!(IBUS_DEV_RADIO == *(m_inPacket+POFF_SRC) && THIS_DEVICE == *(m_inPacket+POFF_DEST)))
    {
        // Sometimes the radio disconnects the CD Changer audio source. It happens when I don't
        // respond correctly to a radio request, but it also happens seemingly for no reason.
        // This code will re-annouce the CD Changer if too much time has passed without any request
        // from the radio (including polling). This should be done only when the CD Changer is in
        // playing mode.
        if(m_cdc_playing && (time(NULL) - m_pollWatch > IBUS_POLL_WATCH_TIMEOUT))
        {
            m_logger->log("Poll watch timed out. Start announcing again.", IBALogger::LOGS_WARNING);
            startAnnounce();
            time(&m_pollWatch);
        }
        
        return;
    }
        
    stopAnnounce();
    time(&m_pollWatch); // refresh the watch timeout
    
    // additionally handle a polling message right here
    if(MIN_PACKET_LEN == *(m_inPacket+POFF_LEN) && IBUS_DATA_POLL_CDC[0] == *(m_inPacket+POFF_DATA))
    {
        //m_logger->log("Polled.", IBALogger::LOGS_DEBUG);
        usleep(m_respDelay); // radio misses too fast responce 
        sendPacket(IBUS_PACK_RESPOND_POLL, sizeof(IBUS_PACK_RESPOND_POLL));
        return;
    }
    
    // place message in queue
    IBusMsg ibus_msg;
    IBusMsgQueue::fillMsg(&ibus_msg,
                          *(m_inPacket+POFF_SRC), 
                          *(m_inPacket+POFF_DEST), 
                          m_inPacket+POFF_DATA, 
                          *(m_inPacket+POFF_LEN) - 2);
    IBusMsgQueue::getQueue().put(&ibus_msg);
}

void IBusCntr::writeRadio(const char* txt)
{
    if(NULL == txt)
        return;
        
    int len = strlen(txt);
    if(len < 1)
        return;
        
    char buff[RADIO_LCD_LEN + 3] = {0x23, 0x40, 0x20};
       
    if(len < RADIO_LCD_LEN)
    {
        memcpy(buff + 3, txt, len);
        memset(buff + 3 + len, ' ', RADIO_LCD_LEN - len);
    }
    else
    {
        memcpy(buff + 3, txt, RADIO_LCD_LEN);
    }
    
    sendPacket(IBUS_DEV_RADIO, IBUS_DEV_MID, (const byte*) buff, RADIO_LCD_LEN + 3);
}

void IBusCntr::writeOBC(const char* txt)
{
    if(NULL == txt)
        return;

    int len = strlen(txt);
    if(len < 1)
        return;
        
    //char buff[OBC_LCD_LEN + 3] = {0x24, 0x01, 0x00};
    char buff[OBC_LCD_LEN + 3] = {0x23, 0x02, 0x20};
       
    if(len < OBC_LCD_LEN)
    {
        memcpy(buff + 3, txt, len);
        memset(buff + 3 + len, ' ', OBC_LCD_LEN - len);
    }
    else
    {
        memcpy(buff + 3, txt, OBC_LCD_LEN);
    }
    
    sendPacket(IBUS_DEV_IKE, IBUS_DEV_OBC, (const byte*) buff, OBC_LCD_LEN + 3);
}

