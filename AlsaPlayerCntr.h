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

#ifndef _ALSA_PLAYER_CNTR_H_
#define _ALSA_PLAYER_CNTR_H_

#include "control.h"
#include "IBAList.h"

class AlsaSubscriber
{
public:
	virtual void OnNewTrack() = 0;
};

class AlsaPlayerCntr
{
public:
    AlsaPlayerCntr();
    ~AlsaPlayerCntr();

    bool init();
    void Subscribe(AlsaSubscriber*);

    bool isApRunning();
    bool isPlaying();

    void play();
    void stop();
    void pause();
    void next();
    void previous();

    void setShuffle(bool);
    void setLoop(bool);

    void add(const CascadeList_t&);
    void add(const char*);
    int  GetPosition();
	void SetPosition(const string& trackPath);
	void SetPosition(int pos);
    void clear();

    char* getTrack();
    char* getTitle();
    char* getArtist();
    char* getAlbum();
    //char* getGenre();
    char* getFileName();
    char* getFilePath();
    char* getInfo();

    int   getLength();
    int   getTimePosition();
    char* getTimeInfo();
    char* getMiscInfo();

private:
    bool    startAp();
    void    stopAp();

	AlsaSubscriber*	m_pSubscriber;

    bool    isOnSameTrack();

    void    fillInfo();
    void    formatTimeInfo();
    char*	getInfoFromPlayer();
    char*	getInfoFromFile();

    int				m_apSession;
    string			m_apName;
	bool			m_fullInfoNeeded;
    int				m_playlistPosition;

	char	m_path[AP_FILE_PATH_MAX];
    char    m_track[AP_TRACK_NUMBER_MAX];
    char    m_title[AP_TITLE_MAX];
    char    m_album[AP_ALBUM_MAX];
    char    m_artist[AP_ARTIST_MAX];
    //char    m_genre[AP_GENRE_MAX];
    static const int m_maxInfoLen = AP_TRACK_NUMBER_MAX +
                                    AP_TITLE_MAX +
                                    AP_ALBUM_MAX +
                                    AP_ARTIST_MAX +
                                    //AP_GENRE_MAX +
                                    16;
    char    m_info[m_maxInfoLen + 1];

    static const int m_maxMiscInfoLen = 20;
    char    m_miscInfo[m_maxMiscInfoLen + 1];

    static const int m_maxTimeInfoLen = 15;
    char    m_timeInfo[m_maxTimeInfoLen + 1];

    static const int m_maxTagSeparatorLen = 3;
    char    m_tagSeparator[m_maxTagSeparatorLen + 1];
};

#endif //_ALSA_PLAYER_CNTR_H_
