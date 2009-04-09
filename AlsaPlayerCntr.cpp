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

#include "IBALogger.h"
#include "IBAConfig.h"
#include "AlsaPlayerCntr.h"

AlsaPlayerCntr::AlsaPlayerCntr()
{
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

/*    ostringstream apCmnd;
    apCmnd << "alsaplayer -q -i daemon -s " << m_apName << " &\n";
    cout << "Starting alsaplayer with:" << endl << apCmnd.str() << endl;
    int exitcode = system(apCmnd.str().c_str());
    if(0 != exitcode)
        return false;

    int count = 4;
    while(count--)
    {
        sleep(5);
        if(isApRunning())
            return true;
    }*/

    return false;
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


void AlsaPlayerCntr::setShuffle(bool isOn)
{
    ap_shuffle_playlist(m_apSession);
}

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
        ap_add_path(m_apSession, (*it).Name.c_str());
}

void AlsaPlayerCntr::clear()
{
    stop();
    ap_clear_playlist(m_apSession);
}


char* AlsaPlayerCntr::getTrack()
{
    ap_get_track_number(m_apSession, m_track);
    return m_track;
}

char* AlsaPlayerCntr::getTitle()
{
    ap_get_title(m_apSession, m_title);
    return m_title;
}

char* AlsaPlayerCntr::getArtist()
{
    ap_get_artist(m_apSession, m_artist);
    return m_artist;
}

char* AlsaPlayerCntr::getAlbum()
{
    ap_get_album(m_apSession, m_album);
    return m_album;
}

char* AlsaPlayerCntr::getGenre()
{
    ap_get_genre(m_apSession, m_genre);
    return m_genre;
}

char* AlsaPlayerCntr::getInfo()
{
    if(!isPlaying())
        return NULL;
    if(!isOnSameTrack())
        formatInfo();
    return m_info;
}


int AlsaPlayerCntr::getLength()
{
    int len;
    ap_get_length(m_apSession, &len);
    return len;
}

int AlsaPlayerCntr::getPosition()
{
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
	ap_get_file_path(m_apSession, m_path);
	return m_path;
}

char* AlsaPlayerCntr::getFileName()
{
    getFilePath();
    char* lastCh = strrchr(m_path, '/');
    if(lastCh)
        strcpy(m_path, lastCh + 1);

    return m_path;
}

void AlsaPlayerCntr::formatInfo()
{
    m_info[0] = 0;
    getTrack();
    if(m_track[0])
    {
        strcat(m_info, m_track);
        strcat(m_info, m_tagSeparator);
        strcat(m_info, getTitle());
        strcat(m_info, m_tagSeparator);
        strcat(m_info, getAlbum());
        strcat(m_info, m_tagSeparator);
        strcat(m_info, getArtist());
        strcat(m_info, m_tagSeparator);
    }
    else
    {
        // use the file name, get rid of the full path, and extension
        getFileName();
    }
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
    int pos = getPosition();
    int minutesPos = pos / 60;
    int secondsPos = pos % 60;

    int len = getLength();
    int minutesLen = len / 60;
    int secondsLen = len % 60;

    sprintf(m_timeInfo,
            "%2.2u:%02.2u/%2.2u:%02.2u",
            minutesPos, secondsPos, minutesLen, secondsLen);
}

char* AlsaPlayerCntr::getMiscInfo()
{
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

