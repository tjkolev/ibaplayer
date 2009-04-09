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

#ifndef _IBA_MUSIC_LIBRARY_H_
#define _IBA_MUSIC_LIBRARY_H_

#include "../IBAList.h"
#include "MusicLibDb.h"
#include "../AlsaPlayerCntr.h"

#define CASCADE_LIST_COUNT	4

class MusicLibBrowser
{
public:
    MusicLibBrowser();
    virtual ~MusicLibBrowser() {};

    void                Init(AlsaPlayerCntr&);

	const string&		Menu();
	const string&		Select(bool withPlay = false, bool withAdd = false);
    const string&       Next();
    const string&       Prev();
    const string&       PgDn();
    const string&       PgUp();
	const string&		Play();
	const string&		Add();

private:

	MusicLibDb		_musicDb;
	AlsaPlayerCntr*	_pAp;

	CascadeList_t	_cascadeLists[CASCADE_LIST_COUNT];
	//int				_cascadeListNdx[CASCADE_LIST_COUNT];
	int				_cascadeNdx;

	CascadeList_t	_playQueue;
	//int				_playQueueNdx;

	float			_pagingSize;

	CascadeList_t&		CurrentCascadeList();
	int					TopMenuIndex();
	ListItem& 			AtItem();
	ListItem&			PrevItem();
	//ListItem& 			Item(int lstNdx, int itemNdx);
	CascadeList_t&		PlayQueue(bool clear = false);
	void				AddTrack(ListItem&);
	void				PlayTrack(ListItem&);
	void				RequeueTracks();
	void				Shuffle();
	int					GetPlayQueueNdx(string& trackPath);
	const string&		GoOffset(int);
	int					ListPageSize();
    void				logSelection();

};

#endif
