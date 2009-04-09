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

// MusicLibrary.h: interface for the MusicLibrary class.
//

//////////////////////////////////////////////////////////////////////

#ifndef _IBA_MUSICLIB_DB_H_
#define _IBA_MUSICLIB_DB_H_

#include <tag.h>

#include "../sqlite3/sqlite3.h"
#include "../IBAConfig.h"
#include "../IBALogger.h"
#include "../IBAList.h"

using namespace TagLib;

class MusicLibDb
{
public:
	MusicLibDb();
	virtual ~MusicLibDb();

	bool Open();
	void Close();
	bool Reindex();

	void SaveSetting(const string& name, const string& value);
	string LoadSetting(const string& name, const string& defaultValue);

	void LoadGenres(CascadeList_t& lst);
	void LoadArtists(CascadeList_t& lst);
	void LoadAlbums(CascadeList_t& lst);
	void LoadAlbumsByGenre(CascadeList_t& lst, int genreId);
	void LoadAlbumsByArtist(CascadeList_t& lst, int artistId);
	void LoadPlaylists(CascadeList_t& lst);

	void LoadTracksByGenre(CascadeList_t&, int genreId);
	void LoadTracksByGenreAlbum(CascadeList_t& lst, int genreId, int albumId);
	void LoadTracksByArtist(CascadeList_t&, int artistId);
	void LoadTracksByAlbum(CascadeList_t& lst, int albumId);
	void LoadTracksByPlaylist(CascadeList_t& lst, int playlistId);
	void LoadTracksRandomPick(CascadeList_t& lst, int count);


private:
	static const int	INVALID_SQL_ID = -1001;
	static string		TAG_UNKNOWN;

	bool	Create();
	void	Populate();
	void	PrepareAddStatements();
	void	DisposeAddStatements();
	int		AddGenre(Tag*);
	int		AddArtist(Tag*);
	int		AddAlbum(Tag*);
	void	AddTrack(Tag*, const string& path);
	int		AddPlaylist(const string& name, const string& path);
	void	AddPlaylistTrack();
	void	LoadList(CascadeList_t& lst, string& sql, int id1 = INVALID_SQL_ID, int id2 = INVALID_SQL_ID);
	string				_dbPath;
	sqlite3*			_db;

	sqlite3_stmt*		_psqlInsertGenre;
	sqlite3_stmt*		_psqlFindGenre;
	sqlite3_stmt*		_psqlInsertArtist;
	sqlite3_stmt*		_psqlFindArtist;
	sqlite3_stmt*		_psqlInsertAlbum;
	sqlite3_stmt*		_psqlFindAlbum;
	sqlite3_stmt*		_psqlInsertTrack;
	sqlite3_stmt*		_psqlInsertPlaylist;
	sqlite3_stmt*		_psqlFindPlaylist;

	string				_lastGenreName;
	int					_lastGenreId;
	string				_lastArtistName;
	int					_lastArtistId;
	string				_lastAlbumName;
	int                 _lastAlbumYear;
	//int					_lastAlbumArtistId;
	int					_lastAlbumId;
	string				_lastPlaylistPath;
	int					_lastPlaylistId;
};

#endif
