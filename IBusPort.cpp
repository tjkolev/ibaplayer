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

#include "IBusPort.h"
 
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


IBusPort::IBusPort()
{
    m_monitorOnly = false;
    m_fd = -1;
    m_portListener = NULL;
}

IBusPort::~IBusPort()
{
    closePort();
}

bool IBusPort::init(IBAConfig& config, IBALogger& logger)
{
    m_config = &config;
    m_logger = &logger;
    
    m_config->getValue(IBAConfig::PRM_IBUS_PORT, m_devPort);
    
    m_config->getValue(IBAConfig::PRM_IBUS_MONITOR_MODE, m_monitorOnly);
    if(m_monitorOnly)
        m_logger->log("Running in monitor mode.", IBALogger::LOGS_WARNING);
    
    return openPort();
}

void IBusPort::regListener(IBusPortListener& listener)
{
    m_portListener = &listener;
}

void IBusPort::receive()
{
    if(NULL == m_portListener)
        return;
    
    int res = read(m_fd, m_serialBuff, SERIAL_BUFF_SIZE);
    if(res > 0)
       m_portListener->haveData(m_serialBuff, res);
}

bool IBusPort::send(const byte* buffer, int len)
{
    if(m_monitorOnly)
        return true;
    
    if(m_fd < 0) return true; // not considered and error

    int res = write(m_fd, buffer, len);
    return res == len;        
}

bool IBusPort::openPort()
{
    closePort();
    
    m_fd = open(m_devPort.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(m_fd < 0)
    {
        perror(m_devPort.c_str()); 
        return false; 
    }
    
    // Make the file descriptor asynchronous
    fcntl(m_fd, F_SETFL, FASYNC);
    //fcntl(m_fd, F_SETFL, FNDELAY);
    
    struct termios term_options;
    tcgetattr(m_fd, &term_options);
    
    // IBus is 9600 8E1
    cfsetispeed(&term_options, B9600);
    cfsetospeed(&term_options, B9600);
    
    term_options.c_cflag |= (CLOCAL | CREAD);
    
    term_options.c_cflag &= ~CSIZE;     // clear size bits with mask
    term_options.c_cflag |= CS8;        // Select 8 data bits 
    
    term_options.c_cflag |= PARENB;     // Enable parity
    term_options.c_cflag &= ~PARODD;    // Even parity (by disabling odd)
    
    term_options.c_cflag &= ~CSTOPB;    // 1 stop bit (by disabling two stop bits)
    
    term_options.c_cflag &= ~CRTSCTS;    // disable hardware flow control
    
    // local options
    term_options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // input options
    term_options.c_iflag |= INPCK; //(INPCK | ISTRIP);
    term_options.c_iflag &= ~(IXON | IXOFF | IXANY | ISTRIP); //disable software flow control
    
    // output options
    term_options.c_oflag &= ~OPOST;  // raw output
    
    //control characters
    term_options.c_cc[VTIME]    = 0;        // no timer
    term_options.c_cc[VMIN]     = 5;        // read when X char(s) available
    
    tcflush(m_fd, TCIFLUSH);
    tcsetattr(m_fd, TCSANOW, &term_options);
    
    return true;
}

void IBusPort::closePort()
{
    if(m_fd >= 0)
        close(m_fd);
    m_fd = -1;
}

void IBusPort::resetPort()
{
    closePort();
    openPort();
}


