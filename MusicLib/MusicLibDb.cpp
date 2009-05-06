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

#include <fstream>

#include "fileref.h"
#include "file_iterator.h"
#include "MusicLibDb.h"

#define DB_FILE_NAME	"IBALib.db"


MusicLibDb::MusicLibDb()
{
}

MusicLibDb::~MusicLibDb()
{
	Close();
}

void MusicLibDb::Close()
{
	sqlite3_close(_db);
}

bool MusicLibDb::Create()
{
	int rc = sqlite3_open_v2(_dbPath.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL );
	if(rc)
	{
		Log("Failed to create database file: " + string(sqlite3_errmsg(_db)), IBALogger::LOGS_CRASH);
		return false;
	}

	char* errmsg;

	string sqlDropTables =
	"DROP TABLE IF EXISTS PlaylistTrack;"
	"DROP TABLE IF EXISTS Playlist;"
	"DROP TABLE IF EXISTS Genre;"
	"DROP TABLE IF EXISTS Album;"
	"DROP TABLE IF EXISTS Artist;"
	"DROP TABLE IF EXISTS Track;"
	"DROP TABLE IF EXISTS Setting;";
	rc = sqlite3_exec(_db, sqlDropTables.c_str(), NULL, NULL, &errmsg);
	if(rc)
	{
		Log("Failed to drop tables: " + string(errmsg), IBALogger::LOGS_CRASH);
		sqlite3_free(errmsg);
		return false;
	}

	string sqlCreateTables =
	"CREATE TABLE Track (Id INTEGER PRIMARY KEY AUTOINCREMENT, Number INTEGER, Title TEXT, GenreId INTEGER, AlbumId INTEGER, ArtistId INTEGER, Path TEXT);"
	"CREATE INDEX Ndx_Track_AlbumId ON Track (AlbumId);"
	"CREATE INDEX Ndx_Track_ArtistId ON Track (ArtistId);"
	"CREATE INDEX Ndx_Track_GenreId ON Track (GenreId);"
	"CREATE TABLE Album (Id INTEGER PRIMARY KEY AUTOINCREMENT, Year INTEGER, Title TEXT, CONSTRAINT Udx_Album_Year_Title UNIQUE (Year, Title) ON CONFLICT IGNORE);"
	"CREATE TABLE Artist (Id INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, CONSTRAINT Udx_Artist_Name UNIQUE (Name) ON CONFLICT IGNORE);"
	"CREATE TABLE Genre (Id INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, CONSTRAINT Udx_Genre_Name UNIQUE (Name) ON CONFLICT IGNORE);"
	"CREATE TABLE Playlist (Id INTEGER PRIMARY KEY, Name TEXT, Path TEXT, CONSTRAINT Udx_Playlist_Path UNIQUE (Path) ON CONFLICT IGNORE);"
	"CREATE TABLE PlaylistTrack (PlaylistId INTEGER, Number INTEGER, TrackId INTEGER);"
	"CREATE TABLE Setting (Name TEXT PRIMARY KEY ON CONFLICT REPLACE, Value TEXT);"
	"INSERT INTO Playlist(Id, Name, Path) VALUES(-1, 'PlayQueue', '.');";

	rc = sqlite3_exec(_db, sqlCreateTables.c_str(), NULL, NULL, &errmsg);
	if(rc)
	{
		Log("Failed to create tables: " + string(errmsg), IBALogger::LOGS_CRASH);
		sqlite3_free(errmsg);
		return false;
	}

	Log("Created new database file.", IBALogger::LOGS_DEBUG);
	Populate();

	return true;
}

bool MusicLibDb::Open()
{
	//string libpath(PRMS_LIB_PATH);
	_dbPath = GetConfigValue<string>(PRMS_LIB_PATH) + "/" + DB_FILE_NAME;
	int rc = sqlite3_open_v2(_dbPath.c_str(), &_db, SQLITE_OPEN_READWRITE, NULL);
	if(rc)
	{
		Log("Failed to open database file. Creating.", IBALogger::LOGS_ERROR);
		return Create();
	}

	Log("Opened database.", IBALogger::LOGS_DEBUG);
	return true;
}

bool MusicLibDb::Reindex()
{
	const string sqlCleanTables =
	"DELETE FROM Track;"
	"DELETE FROM Album;"
	"DELETE FROM Artist;"
	"DELETE FROM Genre;"
	"DELETE FROM PlaylistTrack;"
	"DELETE FROM Playlist;";

	char* errmsg;
	int rc = sqlite3_exec(_db, sqlCleanTables.c_str(), NULL, NULL, &errmsg);
	if(rc)
	{
		Log("Failed to clean tables: " + string(errmsg), IBALogger::LOGS_CRASH);
		sqlite3_free(errmsg);
		return false;
	}

	Populate();
	return true;
}

using namespace filesystem;
using namespace TagLib;

void MusicLibDb::Populate()
{
	Log("Populating database.", IBALogger::LOGS_DEBUG);
	PrepareAddStatements();

	//string path(PRMS_LIB_PATH);
	string libPath = GetConfigValue<string>(PRMS_LIB_PATH);
    for( file_iterator<> itFile(libPath.c_str());
         itFile != itFile.end();
         itFile.advance())
    {
        file_t currFile = *itFile;
        const string& fileName = currFile.getName();
        if(isDirectory(fileName))
            continue;

        // TODO put in configuration.
        // TODO find if file_iterator can do filtering
        if(fileName.substr(fileName.size() - 4, 4) != ".mp3")
        {
        	Log("File skipped:" + fileName, IBALogger::LOGS_INFO);
            continue;
        }

        //FileRef mfile(fileName.c_str(), true, AudioProperties::Fast);
        FileRef mfile(fileName.c_str());

        /*if(NULL == mfile.audioProperties())
        {
        	Log("No audio properties: " + fileName, IBALogger::LOGS_WARNING);
            continue;
        }*/

        Tag* mtag = mfile.tag();
        if(NULL == mtag)
        {
        	Log("No tags: " + fileName, IBALogger::LOGS_WARNING);
        }
        else
        {
        	AddTrack(mtag, fileName);
        }
    }

    DisposeAddStatements();
    Log("Database populated.", IBALogger::LOGS_DEBUG);
}

const string SQL_Insert_Genre = "INSERT INTO Genre(Name) VALUES(?001);";
const string SQL_Find_Genre = "SELECT Id FROM Genre WHERE Name = ?001;";
int MusicLibDb::AddGenre(Tag* mtag)
{
	string name = (mtag->genre()).to8Bit();
	if(_lastGenreName == name)
		return _lastGenreId;

	int rc = sqlite3_bind_text(_psqlFindGenre, 1, name.c_str(), -1, SQLITE_STATIC);
	if(SQLITE_ROW == (rc = sqlite3_step(_psqlFindGenre)))
	{
		_lastGenreId = sqlite3_column_int(_psqlFindGenre, 0);
	}
	else
	{
		rc = sqlite3_bind_text(_psqlInsertGenre, 1, name.c_str(), -1, SQLITE_STATIC);
		rc = sqlite3_step(_psqlInsertGenre);
		_lastGenreId = sqlite3_last_insert_rowid(_db);
		sqlite3_reset(_psqlInsertGenre);
	}
	_lastGenreName = name;
	sqlite3_reset(_psqlFindGenre);

	return _lastGenreId;
}

const string SQL_Insert_Artist = "INSERT INTO Artist(Name) VALUES(?001);";
const string SQL_Find_Artist = "SELECT Id FROM Artist WHERE Name = ?001;";
int MusicLibDb::AddArtist(Tag* mtag)
{
	string name = (mtag->artist()).to8Bit();
	if(_lastArtistName == name)
		return _lastArtistId;

	int rc = sqlite3_bind_text(_psqlFindArtist, 1, name.c_str(), -1, SQLITE_STATIC);
	if(SQLITE_ROW == (rc = sqlite3_step(_psqlFindArtist)))
	{
		_lastArtistId = sqlite3_column_int(_psqlFindArtist, 0);
	}
	else
	{
		rc = sqlite3_bind_text(_psqlInsertArtist, 1, name.c_str(), -1, SQLITE_STATIC);
		rc = sqlite3_step(_psqlInsertArtist);
		_lastArtistId = sqlite3_last_insert_rowid(_db);
		sqlite3_reset(_psqlInsertArtist);
	}
	_lastArtistName = name;
	sqlite3_reset(_psqlFindArtist);

	return _lastArtistId;
}

const string SQL_Insert_Album = "INSERT INTO Album(Year, Title) VALUES(?001, ?002);";
const string SQL_Find_Album = "SELECT Id FROM Album WHERE Year = ?001 AND Title = ?002;";
int MusicLibDb::AddAlbum(Tag* mtag)
{
	string name = (mtag->album()).to8Bit();
	int year = mtag->year();
	if(_lastAlbumName == name && _lastAlbumYear == year)
		return _lastAlbumId;

	int rc = sqlite3_bind_int(_psqlFindAlbum, 1, year);
	rc = sqlite3_bind_text(_psqlFindAlbum, 2, name.c_str(), -1, SQLITE_STATIC);
	if(SQLITE_ROW == (rc = sqlite3_step(_psqlFindAlbum)))
	{
		_lastAlbumId = sqlite3_column_int(_psqlFindAlbum, 0);
	}
	else
	{
		rc = sqlite3_bind_int(_psqlInsertAlbum, 1, year);
		rc = sqlite3_bind_text(_psqlInsertAlbum, 2, name.c_str(), -1, SQLITE_STATIC);
		rc = sqlite3_step(_psqlInsertAlbum);
		_lastAlbumId = sqlite3_last_insert_rowid(_db);
		sqlite3_reset(_psqlInsertAlbum);
	}
	_lastAlbumName = name;
	_lastAlbumYear = year;
	sqlite3_reset(_psqlFindAlbum);

	return _lastAlbumId;
}

const string SQL_Insert_Track = "INSERT INTO Track(Number, Title, GenreId, AlbumId, ArtistId, Path) VALUES(?001, ?002, ?003, ?004, ?005, ?006);";
void MusicLibDb::AddTrack(Tag* mtag, const string& path)
{
	int genreId = AddGenre(mtag);
	int artistId = AddArtist(mtag);
	int albumId = AddAlbum(mtag);

	int trackNumber = mtag->track();
	string title = (mtag->title()).to8Bit();

	int rc = sqlite3_bind_int(_psqlInsertTrack, 1, trackNumber);
	rc = sqlite3_bind_text(_psqlInsertTrack, 2, title.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_bind_int(_psqlInsertTrack, 3, genreId);
	rc = sqlite3_bind_int(_psqlInsertTrack, 4, albumId);
	rc = sqlite3_bind_int(_psqlInsertTrack, 5, artistId);
	rc = sqlite3_bind_text(_psqlInsertTrack, 6, path.c_str(), -1, SQLITE_STATIC);

	rc = sqlite3_step(_psqlInsertTrack);
	sqlite3_reset(_psqlInsertTrack);
}

const string SQL_Insert_Playlist = "INSERT INTO Playlist(Name, Path) VALUES(?001, ?002);";
const string SQL_Find_Playlist = "SELECT Id FROM Playlist WHERE Path = ?001;";
int MusicLibDb::AddPlaylist(const string& name, const string& path)
{
	if(_lastPlaylistPath == path)
		return _lastPlaylistId;

	int rc = sqlite3_bind_text(_psqlFindPlaylist, 1, path.c_str(), -1, SQLITE_STATIC);
	if(SQLITE_ROW == (rc = sqlite3_step(_psqlFindPlaylist)))
	{
		_lastPlaylistId = sqlite3_column_int(_psqlFindPlaylist, 0);
	}
	else
	{
		rc = sqlite3_bind_text(_psqlInsertPlaylist, 1, name.c_str(), -1, SQLITE_STATIC);
		rc = sqlite3_bind_text(_psqlInsertPlaylist, 2, path.c_str(), -1, SQLITE_STATIC);
		rc = sqlite3_step(_psqlInsertPlaylist);
		_lastPlaylistId = sqlite3_last_insert_rowid(_db);
		sqlite3_reset(_psqlInsertPlaylist);
	}
	_lastPlaylistPath = path;
	sqlite3_reset(_psqlFindPlaylist);

	return _lastPlaylistId;
}

const string SQL_Insert_PlaylistTrack = "INSERT INTO PlaylistTrack(PlaylistId, TrackId) VALUES(?001, ?002);";
void MusicLibDb::AddPlaylistTrack()
{
	/*
	if(_lastPlaylistPath == path)
		return _lastPlaylistId;

	int rc = sqlite3_bind_text(_psqlFindPlaylist, 1, path, -1, SQLITE_STATIC);
	if(SQLITE_ROW == (rc = sqlite3_step(_psqlFindPlaylist))
	{
		_lastPlaylistId = sqlite3_column_int(_psqlFindPlaylist, 0);
	}
	else
	{
		rc = sqlite3_bind_text(_psqlInsertPlaylist, 1, name, -1, SQLITE_STATIC);
		rc = sqlite3_bind_text(_psqlInsertPlaylist, 2, path, -1, SQLITE_STATIC);
		rc = sqlite3_step(_psqlInsertPlaylist);
		_lastPlaylistId = sqlite3_last_insert_rowid(_db);
		sqlite3_reset(_psqlInsertPlaylist);
	}
	_lastPlaylistPath = path;
	_sqlite3_reset(_psqlFindPlaylist);

	return _lastPlaylistId;*/
}

const string SQL_Save_Setting = "INSERT INTO Setting(Name,Value) VALUES(?001,?002);";
const string SQL_Load_Setting = "SELECT Value FROM Setting WHERE Name = ?001;";
void MusicLibDb::Save_Setting(const string& name, const string& value)
{
	sqlite3_stmt* psql;
	const char* tail;
	int rc = sqlite3_prepare_v2(_db, SQL_Save_Setting.c_str(), -1, &psql, &tail);
	rc = sqlite3_bind_text(psql, 1, name.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_bind_text(psql, 2, value.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(psql);
	rc = sqlite3_finalize(psql);
}

string MusicLibDb::Load_Setting(const string& name, const string& defaultValue)
{
	sqlite3_stmt* psql;
	const char* tail;
	string value = defaultValue;
	int rc = sqlite3_prepare_v2(_db, SQL_Load_Setting.c_str(), -1, &psql, &tail);
	rc = sqlite3_bind_text(psql, 1, name.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(psql);
	if(rc == SQLITE_ROW)
	{
		const char* sqlTxt = (const char*) sqlite3_column_text(psql, 0);
		//int bytes = sqlite3_column_bytes(psql, 0);
		value = sqlTxt;
	}
	rc = sqlite3_finalize(psql);
	return value;
}

void MusicLibDb::PrepareAddStatements()
{
	const char* tail; // ignored
	int rc;
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Genre.c_str(), -1, &_psqlInsertGenre, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Find_Genre.c_str(), -1, &_psqlFindGenre, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Artist.c_str(), -1, &_psqlInsertArtist, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Find_Artist.c_str(), -1, &_psqlFindArtist, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Album.c_str(), -1, &_psqlInsertAlbum, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Find_Album.c_str(), -1, &_psqlFindAlbum, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Track.c_str(), -1, &_psqlInsertTrack, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Playlist.c_str(), -1, &_psqlInsertPlaylist, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Find_Playlist.c_str(), -1, &_psqlFindPlaylist, &tail);
	//rc = sqlite3_prepare_v2(_db, SQL_Insert_PlaylistTrack, -1, &_psqlInsertPlaylistTrack, &tail);
}

void MusicLibDb::DisposeAddStatements()
{
	int rc;
	rc = sqlite3_finalize(_psqlInsertGenre);
	rc = sqlite3_finalize(_psqlFindGenre);
	rc = sqlite3_finalize(_psqlInsertArtist);
	rc = sqlite3_finalize(_psqlFindArtist);
	rc = sqlite3_finalize(_psqlInsertAlbum);
	rc = sqlite3_finalize(_psqlFindAlbum);
	rc = sqlite3_finalize(_psqlInsertTrack);
	rc = sqlite3_finalize(_psqlInsertPlaylist);
	rc = sqlite3_finalize(_psqlFindPlaylist);
	//rc = sqlite3_finalize(_psqlInsertPlaylistTrack);
}

string MusicLibDb::TAG_UNKNOWN("Unknown");

void MusicLibDb::LoadList(CascadeList_t& lst, const string& sql, int id1, int id2)
{
	const char* tail;
	sqlite3_stmt* pStmt;
	int rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &pStmt, &tail);
	if(id1 != INVALID_SQL_ID)
	{
		rc = sqlite3_bind_int(pStmt, 1, id1);
		if(id2 != INVALID_SQL_ID)
			rc = sqlite3_bind_int(pStmt, 2, id2);
	}

	while(SQLITE_ROW == (rc = sqlite3_step(pStmt)))
	{
		int itemId = sqlite3_column_int(pStmt, 0);
		const char* sqlTxt = (const char*) sqlite3_column_text(pStmt, 1);
		//int bytes = sqlite3_column_bytes(pStmt, 1);
		string name(sqlTxt);
		ListItem item(itemId, name.length() < 1 ? TAG_UNKNOWN : name);
		if(sqlite3_column_count(pStmt) >= 3)
		{
			sqlTxt = (const char*) sqlite3_column_text(pStmt, 2);
			string path(sqlTxt);
			if(name.length() > 0)
				item.Path = path;
		}
		lst.push_back(item);
	}

	rc = sqlite3_finalize(pStmt);
}

int MusicLibDb::LoadTrackCount()
{
	string sql = "select count(*) from Track;";
	const char* tail;
	sqlite3_stmt* pStmt;
	int count = 0;
	int rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &pStmt, &tail);
	if(SQLITE_ROW == (rc = sqlite3_step(pStmt)))
	{
		count = sqlite3_column_int64(pStmt, 0);
	}
	rc = sqlite3_finalize(pStmt);
	return count;
}

void MusicLibDb::SavePlayQueue(const CascadeList_t& playQueue)
{
	int listCount = playQueue.size();
	if(listCount < 1)
		return;
	char* errmsg;
	string sql = "delete from PlaylistTrack where PlaylistId = -1;";
	int rc = sqlite3_exec(_db, sql.c_str(), NULL, NULL, &errmsg);
	sqlite3_free(errmsg);

	const char* tail;
	sqlite3_stmt* pStmt;
	sql = "insert into PlaylistTrack(PlaylistId, Number, TrackId) values(-1, ?001, ?002);";
	rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &pStmt, &tail);
	for(int n = 0; n < listCount; n++)
	{
		const ListItem& item = playQueue[n];
		rc = sqlite3_bind_int(pStmt, 1, n);
		rc = sqlite3_bind_int(pStmt, 2, item.Id);
		rc = sqlite3_step(pStmt);
		rc = sqlite3_reset(pStmt);
	}
	rc = sqlite3_finalize(pStmt);
}

void MusicLibDb::LoadPlayQueue(CascadeList_t& playQueue)
{
	LoadTracksByPlaylist(playQueue, PLAYQUEUE_PLAYLIST_ID);
}

void MusicLibDb::LoadGenres(CascadeList_t& lst)
{
	string sql("select Id, Name from Genre order by Name;");
	LoadList(lst, sql);
}

void MusicLibDb::LoadArtists(CascadeList_t& lst)
{
	string sql("select Id, Name from Artist order by Name;");
	LoadList(lst, sql);
}

void MusicLibDb::LoadAlbums(CascadeList_t& lst)
{
	string sql("select Id, Title from Album order by Title;");
	LoadList(lst, sql);
}

void MusicLibDb::LoadPlaylists(CascadeList_t& lst)
{
	string sql("select Id, Name from Playlist order by Name;");
	LoadList(lst, sql);
}

void MusicLibDb::LoadAlbumsByGenre(CascadeList_t& lst, int genreId)
{
	string sql("select distinct a.Id, a.Title from Album a inner join Track t on a.Id = t.AlbumId where t.GenreId = ?001 order by a.Title;");
	LoadList(lst, sql, genreId);
}

void MusicLibDb::LoadAlbumsByArtist(CascadeList_t& lst, int artistId)
{
	string sql("select distinct a.Id, a.Title from Album a inner join Track t on a.Id = t.AlbumId where t.ArtistId = ?001 order by a.Title;");
	LoadList(lst, sql, artistId);
}

void MusicLibDb::LoadTracksByGenre(CascadeList_t& lst, int genreId)
{
	string sql("select Id, Title, Path from Track where GenreId = ?001 order by Number;");
	LoadList(lst, sql, genreId);
}

void MusicLibDb::LoadTracksByArtist(CascadeList_t& lst, int artistId)
{
	string sql("select Id, Title, Path from Track where ArtistId = ?001 order by Number;");
	LoadList(lst, sql, artistId);
}

void MusicLibDb::LoadTracksByGenreAlbum(CascadeList_t& lst, int genreId, int albumId)
{
	string sql("select Id, Title, Path from Track where GenreId = ?001 and AlbumId = ?002 order by Number;");
	LoadList(lst, sql, genreId, albumId);
}

void MusicLibDb::LoadTracksByAlbum(CascadeList_t& lst, int albumId)
{
	string sql("select Id, Title, Path from Track where AlbumId = ?001 order by Number;");
	LoadList(lst, sql, albumId);
}

void MusicLibDb::LoadTracksByPlaylist(CascadeList_t& lst, int playlistId)
{
	string sql("select t.Id, t.Title, t.Path from Track t inner join PlaylistTrack p on t.Id = p.TrackId where p.PlaylistId = ?001 order by p.Number;");
	LoadList(lst, sql, playlistId);
}

void MusicLibDb::LoadTracksRandomPick(CascadeList_t& lst, int count)
{
	int trackCount = LoadTrackCount();
	if(trackCount < 1)
		return;
	// generate random track ids
	ostringstream sql;
	sql << "select Id, Title, Path from Track where Id in (";
	srand(time(NULL));
	for(int n = 0; n < count; n++)
	{
		int trackId = rand() % trackCount + 1;
		sql << (n > 0 ? "," : "") << trackId;
	}
	sql << ");";
	LoadList(lst, sql.str());
}
