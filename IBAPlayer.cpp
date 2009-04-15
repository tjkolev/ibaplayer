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

#include "IBAPlayer.h"
#include "MusicLib/MusicLibDb.h"

IBAPlayer::IBAPlayer()
{
}

IBAPlayer::~IBAPlayer()
{
}

bool IBAPlayer::Init()
{
    return m_ap.init();
}


AlsaPlayerCntr& IBAPlayer::getPlayer()
{
    return m_ap;
}

IBusCntr& IBAPlayer::getIBusCntr()
{
    return m_ibus;
}

// MsgProcessor& IBAPlayer::getMsgProcessor()
// {
//     return m_msgProc;
// }

IBATimers& IBAPlayer::getTimers()
{
    return m_timers;
}

void IBAPlayer::reindexLib()
{
    //getLog().log("Reindexing library.\n");
    //m_mlibMngr.reindex();
}


void* startIBusCntr(void* arg)
{
    assert(arg);

    IBusCntr* ibus = (IBusCntr*) arg;
    ibus->run();
}

void IBAPlayer::play()
{
    m_msgProc.run();
}

int IBAPlayer::run()
{
    Log("Starting player.\n");

//    m_mlibMngr.getLibrary().load();
//    m_mlibMngr.getBrowser().init();

    if(!Init())
        return 2;

    m_timers.init();

    m_ibus.init(m_timers);
    m_msgProc.init(*this);

    // start a separate thread for ibus serial handling
    pthread_attr_t thAttr;
    pthread_attr_init(&thAttr);
    pthread_attr_setdetachstate(&thAttr, PTHREAD_CREATE_DETACHED);
    pthread_create(&m_threadIBusCntr, &thAttr, startIBusCntr, &m_ibus);

    play();

    return 0;
}

//////////////////////////////////////////////

extern void consolePlay(IBAPlayer*);

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
    ostringstream out;
    out << IBA_APP_NAME << ". Version " << IBA_VER_MAJOR << '.' << IBA_VER_MINOR << ". " << IBA_COPYRIGHT << endl;
    cout << out.str();

    if(argc > 1)                // the first one is the config file
    {
    	string cfgPath(argv[1]);
		IBAConfig::Config().init(cfgPath);
    }

	IBALogger::Logger().setLogOutput(IBALogger::LOGO_FILE, GetConfigValue<string>(PRMS_LOG_FILE));
	int sl = GetConfigValue<int>(PRMS_SVR_LEVEL);
	IBALogger::Logger().setLogLevel(sl);

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
	consolePlay(&player);
	return 0;
/*
    if(argc > 2)    // existance of second - reindex
    {
        if(0 == strcmp(argv[2], "reindex"))
        {
            player.reindexLib();
            return 0;
        }
        else if(0 == strcmp(argv[2], "conplay"))
        {
            consolePlay(&player);
            return 0;
        }
        else
            return 1;
    }
*/
//    return player.run();

/*    IBusCntr ibus;
    if(!ibus.init(player.getConfig(), player.getLog()))
        return 2;*/

    // start a separate thread for ibus serial handling
/*    pthread_attr_t thAttr;
    pthread_attr_init(&thAttr);
    pthread_attr_setdetachstate(&thAttr, PTHREAD_CREATE_DETACHED);

    pthread_t* threadIBusCntr = NULL;
    pthread_create(threadIBusCntr, &thAttr, startIBusCntr, &ibus);*/

    //ibus.run();

        //byte t[] = {0xC0,0x06,0xFF,0x20,0x00,0x01,0x00,0x18};
        //ibus.haveData(t, 8);

/*        char* s;
        int len;
        byte h[40];

        s = "681EC021001606464D05414D0550545920052052445305534320054D4F4445AE";
        len = strlen(s);
        ibus.haveData(strTohex(s, len, h), len/2);*/

        /*
        s = "0302A00100FFC006"; //++
        len = strlen(s);
        ibus.haveData(strTohex(s, len, h), len/2);

        s = "FF20000100"; //++
        len = strlen(s);
        ibus.haveData(strTohex(s, len, h), len/2);


        s = "18C006FF2020B2008B";
        len = strlen(s);
        ibus.haveData(strTohex(s, len, h), len/2);


        s = "6812C0234020464D20032039372E31";
        len = strlen(s);
        ibus.haveData(strTohex(s, len, h), len/2);

        s = "04202020C400010100681DC021001660203120052032200520332005203420052035200520362AEA";
        len = strlen(s);
        ibus.haveData(strTohex(s, len, h), len/2);*/

        /*
        s = "681EC021001606464D05414D0550545920052052445305534320054D4F4445AE";
        len = strlen(s);
        ibus.haveData(strTohex(s, len, h), len/2);

        s = "C0046822018F";
        len = strlen(s);
        ibus.haveData(strTohex(s, len, h), len/2);*/

        //while(true);

    //return 0;
}
