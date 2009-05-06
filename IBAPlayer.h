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
#include "MsgProcessor.h"

using namespace std;

void PrintList(const CascadeList_t&);

class IBAPlayer
{
public:

	IBAPlayer();
	virtual ~IBAPlayer();

    bool	Init();
    int		run();

private:

    MsgProcessor     m_msgProc;
    IBATimers        m_timers;
};

#endif // _IBA_PLAYER_H_
