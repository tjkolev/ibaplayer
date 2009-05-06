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
#include <tag.h>
#include "fileref.h"

#include "IBALogger.h"
#include "IBAConfig.h"
#include "AlsaPlayerCntr.h"

using namespace TagLib;

AlsaPlayerCntr::AlsaPlayerCntr()
{
	m_pSubscriber = NULL;
    m_playlistPosition = -1;
    strcpy(m_tagSeparator, " * ");
    m_miscInfo[m_maxMiscInfoLen] = 0;
}

AlsaPlayerCntr::~AlsaPlayerCntr()
{
    stopAp();
}

bool AlsaPlayerCntr::init()
{
	Log("Initializing Alsa player control.", IBALogger::LOGS_DEBUG);
    m_apName = GetConfigValue<string>(PRMS_ALSAPLAYER_NAME);
    string tagSep = GetConfigValue<string>(PRMS_TAG_SEPARATOR);
    m_tagSeparator[1] = tagSep[0];

    if(!startAp())
    {
        const char* apFail = "Failed to start ALSAPlayer.";
        Log(apFail, IBALogger::LOGS_CRASH);
        cout << apFail << endl;

        return false;
    }

    return true;
}

bool AlsaPlayerCntr::isPlaying()
{
    int isIt = 0;
    ap_is_playing(m_apSession, &isIt);
    return (bool) isIt;
}

bool AlsaPlayerCntr::isApRunning()
{
    int res = ap_find_session((char*)m_apName.c_str(), &m_apSession);
    if(res)
        return ap_session_running(m_apSession);
    return false; //ap_session_running(m_apSession);
}

bool AlsaPlayerCntr::startAp()
{
    if(isApRunning())
    {
        cout << "Alsaplayer is already running" << endl;
        return true;
    }

    string apCmnd("alsaplayer -i daemon -s " + m_apName + "&");
    int exitcode = system(apCmnd.c_str());
    if(0 != exitcode)
        return false;

    int count = 4;
    while(count--)
    {
        sleep(2);
        if(isApRunning())
            return true;
    }

    return false;
}

void AlsaPlayerCntr::Subscribe(AlsaSubscriber* pSub)
{
	m_pSubscriber = pSub;
}

void AlsaPlayerCntr::stopAp()
{
    if(!isApRunning())
		return;

    ap_stop(m_apSession);
    ap_quit(m_apSession);
}

void AlsaPlayerCntr::play()
{
    ap_play(m_apSession);
}

void AlsaPlayerCntr::stop()
{
    ap_stop(m_apSession);
}

void AlsaPlayerCntr::pause()
{
    ap_pause(m_apSession);
}

void AlsaPlayerCntr::next()
{
    ap_next(m_apSession);
}

void AlsaPlayerCntr::previous()
{
    ap_prev(m_apSession);
}

/*
void AlsaPlayerCntr::setShuffle(bool isOn)
{
    ap_shuffle_playlist(m_apSession);
}
*/

void AlsaPlayerCntr::setLoop(bool isOn)
{
    ap_set_playlist_looping(m_apSession, isOn);
}

void AlsaPlayerCntr::add(const char* path)
{
	ap_add_path(m_apSession, path);
}

void AlsaPlayerCntr::add(const CascadeList_t& plist)
{
    for(CascadeList_t::const_iterator it = plist.begin();
        it != plist.end();
        it++)
        ap_add_path(m_apSession, (*it).Path.c_str());

	// stupid tricks to get rid of the currently buffered track
	// in case this is a clean add, not append.
	int trackCount = 0;
	ap_get_playlist_length(m_apSession, &trackCount);
	if(trackCount <= plist.size())
		next();
}

void AlsaPlayerCntr::SetPosition(const string& trackPath)
{
	int plLen = 0;
	ap_get_playlist_length(m_apSession, &plLen);
	if(plLen < 2)
		return;
	char path[AP_FILE_PATH_MAX];
	for(int n = 1; n <= plLen; n++)
	{
		ap_get_file_path_for_track(m_apSession, path, n);
		if(0 == trackPath.compare(path))
		{
			SetPosition(n);
			return;
		}
	}
}

void AlsaPlayerCntr::SetPosition(int pos)
{
	ap_set_current(m_apSession, pos - 1);
	next();
}

int AlsaPlayerCntr::GetPosition()
{
	int pos = 0;
	ap_get_playlist_position(m_apSession, &pos);
	return pos;
}

void AlsaPlayerCntr::clear()
{
    stop();
    ap_clear_playlist(m_apSession);
    m_info[0] = 0;
    m_playlistPosition = -1;
}


char* AlsaPlayerCntr::getTrack()
{
	fillInfo();
    return m_track;
}

char* AlsaPlayerCntr::getTitle()
{
	fillInfo();
    return m_title;
}

char* AlsaPlayerCntr::getArtist()
{
	fillInfo();
    return m_artist;
}

char* AlsaPlayerCntr::getAlbum()
{
	fillInfo();
    return m_album;
}

/*
char* AlsaPlayerCntr::getGenre()
{
    if(!isPlaying())
        return NULL;
    ap_get_genre(m_apSession, m_genre);
    return m_genre;
}
*/

char* AlsaPlayerCntr::getInfo()
{
	fillInfo();
    return m_info;
}


int AlsaPlayerCntr::getLength()
{
    if(!isPlaying())
        return 0;
    int len;
    ap_get_length(m_apSession, &len);
    return len;
}

int AlsaPlayerCntr::getTimePosition()
{
    if(!isPlaying())
        return 0;
    int pos;
    ap_get_position(m_apSession, &pos);
    return pos;
}

char* AlsaPlayerCntr::getTimeInfo()
{
    if(!isPlaying())
        return NULL;
    formatTimeInfo();
    return m_timeInfo;
}

char* AlsaPlayerCntr::getFilePath()
{
	fillInfo();
	return m_path;
}

char* AlsaPlayerCntr::getFileName()
{
	fillInfo();
    char* lastCh = strrchr(m_path, '/');
    if(lastCh)
		return lastCh + 1;

    return m_path;
}

char* AlsaPlayerCntr::getInfoFromFile()
{
	if(NULL == m_path || m_path[0] == 0) //this should never happen
		return NULL;
	FileRef mfile(m_path);
	Tag* mtag = mfile.tag();
	if(NULL == mtag)
		return NULL;
	sprintf(m_track, "%d", mtag->track());
//	string tag = mtag->genre().to8Bit();
//	sprintf(m_genre, "%s", tag.substr(0, AP_GENRE_MAX-1).c_str());
	string tag = mtag->artist().to8Bit();
	sprintf(m_artist, "%s", tag.substr(0, AP_ARTIST_MAX-1).c_str());
	tag = mtag->album().to8Bit();
	sprintf(m_album, "%s", tag.substr(0, AP_ALBUM_MAX-1).c_str());
	tag = mtag->title().to8Bit();
	sprintf(m_title, "%s", tag.substr(0, AP_TITLE_MAX-1).c_str());
	return m_title;
}

char* AlsaPlayerCntr::getInfoFromPlayer()
{
	ap_get_file_path(m_apSession, m_path); //this better work every time

    ap_get_track_number(m_apSession, m_track);
    ap_get_artist(m_apSession, m_artist);
    ap_get_album(m_apSession, m_album);
    ap_get_title(m_apSession, m_title);
    if(NULL == m_title || m_title[0] == 0)
		return NULL;
	return m_title;
}


void AlsaPlayerCntr::fillInfo()
{
    if(!isPlaying())
    {
		m_info[0] = 0;
        return;
    }

	if(isOnSameTrack())
		return;

	if(NULL != m_pSubscriber)
		m_pSubscriber->OnNewTrack();

	m_info[0] = 0;
    if(NULL == getInfoFromPlayer())
		if(NULL == getInfoFromFile())
			{
				strcpy(m_info, getFileName());
				return;
			}

	sprintf(m_info, "%s%s%s%s%s%s%s%s",
					m_track, m_tagSeparator, m_title, m_tagSeparator, m_album, m_tagSeparator, m_artist, m_tagSeparator);
}


bool AlsaPlayerCntr::isOnSameTrack()
{
    int currPosition;
    int result = ap_get_playlist_position(m_apSession, &currPosition);
    if(!result)
        return false;

    if(currPosition != m_playlistPosition)
    {
        m_playlistPosition = currPosition;
        return false;
    }

    return true;
}

void AlsaPlayerCntr::formatTimeInfo()
{
    int pos = getTimePosition();
    int minutesPos = pos / 60;
    int secondsPos = pos % 60;

    int len = getLength();
    int minutesLen = len / 60;
    int secondsLen = len % 60;

    sprintf(m_timeInfo,
            "%2.2u:%2.2u/%2.2u:%2.2u",
            minutesPos, secondsPos, minutesLen, secondsLen);
}

char* AlsaPlayerCntr::getMiscInfo()
{
    if(!isPlaying())
        return NULL;
    // track_in_playlist/total_tracks kbps time
    // 1/52 192kbit    5:45

    int trackCount = 0;
    ap_get_playlist_length(m_apSession, &trackCount);
    int trackPos = 0;
    ap_get_playlist_position(m_apSession, &trackPos);
    char streamType[32];
    ap_get_stream_type(m_apSession, streamType);
    // MP3 44KHz stereo 192kbit
    snprintf(m_miscInfo, m_maxMiscInfoLen,
             "%u/%u %.7s     ",
             trackPos, trackCount, streamType + 17);

    time_t now;
    time(&now);
    struct tm tmResult;
    localtime_r(&now, &tmResult);
    strftime(m_miscInfo + (m_maxMiscInfoLen - 5), 6, "%l:%M", &tmResult);

    return m_miscInfo;
}

