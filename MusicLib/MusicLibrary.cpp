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

#include "MusicLibrary.h"
#include "../IBAConfig.h"
#include "../IBALogger.h"

#define TOP_MENU_LIST	0
#define TOP_MENU_NDX_GENRE		0
#define TOP_MENU_NDX_ARTIST		1
#define TOP_MENU_NDX_ALBUM		2
#define TOP_MENU_NDX_PLIST		3
#define TOP_MENU_NDX_PQUEUE		4
#define TOP_MENU_NDX_SHUFFLE	5
#define TOP_MENU_NDX_RNDPICK	6
#define TOP_MENU_NDX_REINDEX	7

MusicLibBrowser::MusicLibBrowser()
{
}

void MusicLibBrowser::Init(AlsaPlayerCntr& ap)
{
	_cascadeLists[TOP_MENU_LIST][TOP_MENU_NDX_GENRE] = ListItem(TOP_MENU_NDX_GENRE + 1, "Genres");
	_cascadeLists[TOP_MENU_LIST][TOP_MENU_NDX_ARTIST] = ListItem(2, "Artists");
	_cascadeLists[TOP_MENU_LIST][TOP_MENU_NDX_ALBUM] = ListItem(3, "Albums");
	_cascadeLists[TOP_MENU_LIST][TOP_MENU_NDX_PLIST] = ListItem(4, "Playlists");
	_cascadeLists[TOP_MENU_LIST][TOP_MENU_NDX_PQUEUE] = ListItem(5, "Play Queue");
	_cascadeLists[TOP_MENU_LIST][TOP_MENU_NDX_SHUFFLE] = ListItem(6, "Shuffle");
	_cascadeLists[TOP_MENU_LIST][TOP_MENU_NDX_RNDPICK] = ListItem(7, "Random Pick 20");
	_cascadeLists[TOP_MENU_LIST][TOP_MENU_NDX_REINDEX] = ListItem(8, "Reindex");

	_cascadeNdx = TOP_MENU_LIST;
	_cascadeLists[TOP_MENU_LIST].CurrentIndex = TOP_MENU_NDX_PQUEUE;

	_musicDb.Open();

    _pagingSize = GetConfigValue<float>(PRMS_PAGE_SIZE);
    if(_pagingSize <= 0)
    {
        Log("Invalid page size in configuration. Page size set to 3.\n", IBALogger::LOGS_WARNING);
        _pagingSize = 3;
    }

    _pAp = &ap;
}

int MusicLibBrowser::TopMenuIndex()
{
	return _cascadeLists[TOP_MENU_LIST].CurrentIndex;
}

CascadeList_t& MusicLibBrowser::CurrentCascadeList()
{
	if(_cascadeNdx == 1 && TOP_MENU_NDX_PQUEUE == TopMenuIndex())
		return PlayQueue();
	else
		return _cascadeLists[_cascadeNdx];
}

ListItem& MusicLibBrowser::AtItem()
{
	return CurrentCascadeList().AtItem();
}

ListItem& MusicLibBrowser::PrevItem()
{
	// does not work with PlayQueue
	return _cascadeLists[_cascadeNdx - 1].AtItem();
}

int MusicLibBrowser::ListPageSize()
{
	if(_pagingSize >= 1.0)
		return (int)_pagingSize;

	int lstCount = CurrentCascadeList().size();
	return (int)(lstCount * _pagingSize);
}

const string& MusicLibBrowser::GoOffset(int offset)
{
	CascadeList_t& list = CurrentCascadeList();
	int lstCount = list.size();
	int newNdx = list.CurrentIndex + offset;
	if(newNdx >= lstCount)
		newNdx = 0;
	else if(newNdx < 0)
		newNdx = lstCount - 1;

	list.CurrentIndex = newNdx;
	return list[newNdx].Name;
}

const string& MusicLibBrowser::Next()
{
    return GoOffset(1);
}

const string& MusicLibBrowser::Prev()
{
    return GoOffset(-1);
}

const string& MusicLibBrowser::PgDn()
{
    return GoOffset(ListPageSize());
}

const string& MusicLibBrowser::PgUp()
{
    return GoOffset(-ListPageSize());
}

const string& MusicLibBrowser::Menu()
{
	if(_cascadeNdx > TOP_MENU_LIST) // already at top
		_cascadeNdx--;
	return AtItem().Name;
}

CascadeList_t& MusicLibBrowser::PlayQueue(bool clear)
{
	if(clear)
		_playQueue.clear();
	return _playQueue;
}

void MusicLibBrowser::PlayTrack(ListItem& item)
{
	PlayQueue(true).push_back(item);
}

void MusicLibBrowser::AddTrack(ListItem& item)
{
	PlayQueue(false).push_back(item);
}

const string& MusicLibBrowser::Select(bool withPlay, bool withAdd)
{
	if(_cascadeNdx < 0)
		_cascadeNdx = 0;

	CascadeList_t& targetList = PlayQueue();

	//load next list from db depending on top menu and the cascade depth
	switch(TopMenuIndex())
	{
		case TOP_MENU_NDX_GENRE:
			if(_cascadeNdx > 3)
				_cascadeNdx = 0;

			if(_cascadeNdx < 3)
			{
				targetList = _cascadeLists[_cascadeNdx + 1];
				targetList.clear();
			}

			switch(_cascadeNdx)
			{
			case 0:
				withPlay = withAdd = false;
				_musicDb.LoadGenres(targetList);
				break;

			case 1:
				if(withPlay || withAdd)
				{
					_musicDb.LoadTracksByGenre(PlayQueue(withPlay), AtItem().Id);
				}
				else
				{
					_musicDb.LoadAlbumsByGenre(targetList, AtItem().Id);
				}
				break;

			case 2:
				if(withPlay || withAdd)
					targetList = PlayQueue(withPlay);
				_musicDb.LoadTracksByGenreAlbum(targetList,	PrevItem().Id, AtItem().Id);
				break;

			case 3:
				if(withAdd)
				{
					AddTrack(AtItem());
				}
				else
				{
					PlayTrack(AtItem());
				}
				break;
			}

			break;

		case TOP_MENU_NDX_ARTIST:
			if(_cascadeNdx > 3)
				_cascadeNdx = 0;

			if(_cascadeNdx < 3)
			{
				targetList = _cascadeLists[_cascadeNdx + 1];
				targetList.clear();
			}

			switch(_cascadeNdx)
			{
			case 0:
				withPlay = withAdd = false;
				_musicDb.LoadArtists(targetList);
				break;

			case 1:
				if(withPlay || withAdd)
				{
					_musicDb.LoadTracksByArtist(PlayQueue(withPlay), AtItem().Id);
				}
				else
				{
					_musicDb.LoadAlbumsByArtist(targetList, AtItem().Id);
				}
				break;

			case 2:
				if(withPlay || withAdd)
					targetList = PlayQueue(withPlay);
				_musicDb.LoadTracksByAlbum(targetList, AtItem().Id);
				break;

			case 3:
				if(withAdd)
					AddTrack(AtItem());
				else
					PlayTrack(AtItem());
				break;
			}
			break;

		case TOP_MENU_NDX_ALBUM:
			if(_cascadeNdx > 2)
				_cascadeNdx = 0;

			if(_cascadeNdx < 2)
			{
				targetList = _cascadeLists[_cascadeNdx + 1];
				targetList.clear();
			}

			switch(_cascadeNdx)
			{
			case 0:
				withPlay = withAdd = false;
				_musicDb.LoadAlbums(targetList);
				break;

			case 1:
				if(withPlay || withAdd)
					targetList = PlayQueue(withPlay);
				_musicDb.LoadTracksByAlbum(targetList, AtItem().Id);
				break;

			case 2:
				if(withAdd)
					AddTrack(AtItem());
				else
					PlayTrack(AtItem());
				break;
			}
			break;

		case TOP_MENU_NDX_PLIST:
			if(_cascadeNdx > 2)
				_cascadeNdx = 0;

			if(_cascadeNdx < 2)
			{
				targetList = _cascadeLists[_cascadeNdx + 1];
				targetList.clear();
			}

			switch(_cascadeNdx)
			{
			case 0:
				withPlay = withAdd = false;
				_musicDb.LoadPlaylists(targetList);
				break;

			case 1:
				if(withPlay || withAdd)
					targetList = PlayQueue(withPlay);
				_musicDb.LoadTracksByPlaylist(targetList, AtItem().Id);
				break;

			case 2:
				if(withAdd)
					AddTrack(AtItem());
				else
					PlayTrack(AtItem());
				break;
			}
			break;

		case TOP_MENU_NDX_PQUEUE:
			if(_cascadeNdx > 1)
				_cascadeNdx = 0;

			switch(_cascadeNdx)
			{
			case 0:
				{
					// position on the currently playing track
					string trackPath = _pAp->getFilePath();
					int ndx = GetPlayQueueNdx(trackPath);
					if(ndx < 0)
						ndx = 0;
					_cascadeNdx++;
					PlayQueue().CurrentIndex = ndx;
					return AtItem().Name;
				}
				break;
			case 1:
				withPlay = withAdd = false;
				//TODO Move play pointer
				break;
			}
			break;

		case TOP_MENU_NDX_SHUFFLE:
			if(_cascadeNdx > 0)
				_cascadeNdx = 0;
			withPlay = withAdd = false;
			Shuffle();
			break;

		case TOP_MENU_NDX_RNDPICK:
			if(_cascadeNdx > 0)
				_cascadeNdx = 0;
			withPlay = !withAdd;
			_musicDb.LoadTracksRandomPick(PlayQueue(withPlay), 20);
			break;

		case TOP_MENU_NDX_REINDEX:
			if(_cascadeNdx > 0)
				_cascadeNdx = 0;
			_musicDb.Reindex();
			break;
	}

	if(withPlay || withAdd)
	{
		if(withPlay)
			RequeueTracks();
	}
	else
	{
		// move to next list
		_cascadeNdx++;
		CurrentCascadeList().CurrentIndex = 0;
	}
	return AtItem().Name;
}

void MusicLibBrowser::RequeueTracks()
{
	_pAp->stop();
	_pAp->clear();
	_pAp->add(PlayQueue(false));
	_pAp->play();
}

void MusicLibBrowser::Shuffle()
{
	_pAp->stop();
	_pAp->setShuffle(true);
	_pAp->play();
}

const string& MusicLibBrowser::Play()
{
	return Select(true, false);
}

const string& MusicLibBrowser::Add()
{
	return Select(false, true);
}

int MusicLibBrowser::GetPlayQueueNdx(string& trackPath)
{
	int ndx = 0;
	for(CascadeList_t::const_iterator it = PlayQueue().begin();
		it != PlayQueue().end();
		it++, ndx++)
	{
		if((*it).Path == trackPath)
			return ndx;
	}
	return -1;
}

void MusicLibBrowser::logSelection()
{
    if(IBALogger::Logger().getLogLevel() & IBALogger::LOGS_DEBUG)
    {
        ostringstream out;
        out << "Selection:" << endl;
        for(CascadeList_t::const_iterator it = PlayQueue().begin();
            it != PlayQueue().end();
            it++)
            out << (*it).Name << endl;

        Log(out.str(), IBALogger::LOGS_DEBUG);
    }
}
