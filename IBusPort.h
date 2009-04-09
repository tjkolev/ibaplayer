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
#ifndef _IBUS_PORT_H_
#define _IBUS_PORT_H_

#include <string>

using namespace std;

typedef unsigned char byte;

class IBusPortListener
{
public:
    virtual void    haveData(byte* buffer, int len) = 0;
};


class IBusPort
{
public:

    IBusPort();
    ~IBusPort();

    bool            init();
    void            closePort();
    void            resetPort();

    bool            send(const byte* buffer, int len);
    void            receive();
    void            regListener(IBusPortListener&);

private:

    bool            openPort();

    IBusPortListener*   m_portListener;

    string              m_devPort;
    bool                m_monitorOnly;
    int                 m_fd;

    static const int    SERIAL_BUFF_SIZE = 32;
    byte                m_serialBuff[SERIAL_BUFF_SIZE];

};

#endif //_IBUS_PORT_H_
