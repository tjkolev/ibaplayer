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

#include "IBAVer.h"
#include "IBAPlayer.h"

IBAPlayer::IBAPlayer()
{
}

IBAPlayer::~IBAPlayer()
{
}

bool IBAPlayer::Init()
{
    ostringstream out;
    out << IBA_APP_NAME << ". Version " << IBA_VER_MAJOR << '.' << IBA_VER_MINOR << ". " << IBA_COPYRIGHT << endl;
    cout << out.str();
    return true;
}

int IBAPlayer::run()
{
    Log("Starting player.", IBALogger::LOGS_DEBUG);

    if(!m_timers.init())
    {
    	Log("Failed to initialize timers.", IBALogger::LOGS_ERROR);
    	return 4;
    }
    if(!m_msgProc.init(m_timers))
    {
    	Log("Failed to initialize message processor.", IBALogger::LOGS_ERROR);
		return 3;
    }
    m_msgProc.run();

    return 0;
}

//////////////////////////////////////////////

extern void consolePlay();

void PrintList(const CascadeList_t& lst)
{
	cout << "List:" << endl;
	for(CascadeList_t::const_iterator it = lst.begin();	it != lst.end(); it++)
	{
		const ListItem& item = (*it);
		cout << item.Id << "|" << item.Name << "|" << item.Path << endl;
	}
}

int main(int argc, char* argv[])
{
    if(argc > 1)                // the first one is the config file
    {
    	string cfgPath(argv[1]);
		IBAConfig::Config().init(cfgPath);
    }

	IBALogger::Logger().setLogOutput(IBALogger::LOGO_FILE, GetConfigValue<string>(PRMS_LOG_FILE));
	int sl = GetConfigValue<int>(PRMS_SVR_LEVEL);
	IBALogger::Logger().setLogLevel((IBALogger::Severity)sl);

/* testing code */
if(false)
{
	MusicLibDb musicDb;
	musicDb.Open();
	CascadeList_t lst;
	musicDb.LoadGenres(lst);
	PrintList(lst);
	lst.clear();
	musicDb.LoadArtists(lst);
	PrintList(lst);
	lst.clear();
	musicDb.LoadTracksByArtist(lst, 1);
	PrintList(lst);
	lst.clear();
	musicDb.LoadAlbumsByArtist(lst, 1);
	PrintList(lst);
	musicDb.Close();
	return 0;
}
/* end testing code */


    IBAPlayer player;
	if(!player.Init())
		return 1;

    if(argc > 2)    // existance of second - console play
    {
		if(0 == strcmp(argv[2], "console"))
        {
            consolePlay();
            return 0;
        }
        else
        {
        	cout << "The only acceptable parameter is 'console'";
            return 1;
        }
    }

	// real deal, will not return
	return player.run();
}

/*
static byte charhex[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                         0, 0, 0, 0, 0, 0, 0,
                         10, 11, 12, 13, 14, 15, 16};

byte charTohex(char* c)
{
    byte hex = 16 * charhex[*c - '0'] + charhex[*(c+1) - '0'];
    return hex;
}

byte* strTohex(char* str, int len, byte* buff)
{
    for(int ndx = 0; ndx < len; ndx += 2)
        buff[ndx/2] = charTohex(str + ndx);
    return buff;
}
*/
