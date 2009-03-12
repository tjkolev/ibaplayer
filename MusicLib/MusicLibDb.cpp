/***************************************************************************
 *   Copyright (C) 2004 by TJ Kolev                                        *
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

	char* sqlDropTables =
	"DROP TABLE IF EXISTS PlaylistTrack;"
	"DROP TABLE IF EXISTS Playlist;"
	"DROP TABLE IF EXISTS Genre;"
	"DROP TABLE IF EXISTS Album;"
	"DROP TABLE IF EXISTS Artist;"
	"DROP TABLE IF EXISTS Track;"
	"DROP TABLE IF EXISTS Setting;";
	rc = sqlite3_exec(_db, sqlDropTables, NULL, NULL, &errmsg);
	if(rc)
	{
		Log("Failed to drop tables: " + string(errmsg), IBALogger::LOGS_CRASH);
		sqlite3_free(errmsg);
		return false;
	}

	char* sqlCreateTables =
	"CREATE TABLE Track (Id INTEGER PRIMARY KEY AUTOINCREMENT, Number INTEGER, Title TEXT, GenreId INTEGER, AlbumId INTEGER, ArtistId INTEGER, Path TEXT);"
	"CREATE INDEX Ndx_Track_AlbumId ON Track (AlbumId);"
	"CREATE INDEX Ndx_Track_ArtistId ON Track (ArtistId);"
	"CREATE INDEX Ndx_Track_GenreId ON Track (GenreId);"
	"CREATE TABLE Album (Id INTEGER PRIMARY KEY AUTOINCREMENT, ArtistId INTEGER, Title TEXT, CONSTRAINT Udx_Album_ArtistId_Title UNIQUE (ArtistId, Title) ON CONFLICT IGNORE);"
	"CREATE TABLE Artist (Id INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, CONSTRAINT Udx_Artist_Name UNIQUE (Name) ON CONFLICT IGNORE);"
	"CREATE TABLE Genre (Id INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, CONSTRAINT Udx_Genre_Name UNIQUE (Name) ON CONFLICT IGNORE);"
	"CREATE TABLE Playlist (Id INTEGER PRIMARY KEY, Name TEXT, Path TEXT, CONSTRAINT Udx_Playlist_Path UNIQUE (Path) ON CONFLICT IGNORE);"
	"CREATE TABLE PlaylistTrack (PlaylistId INTEGER, TrackId INTEGER);"
	"CREATE TABLE Setting (Name TEXT, Value TEXT);";
	rc = sqlite3_exec(_db, sqlCreateTables, NULL, NULL, &errmsg);
	if(rc)
	{
		Log("Failed to create tables: " + string(errmsg), IBALogger::LOGS_CRASH);
		sqlite3_free(errmsg);
		return false;
	}

	Populate();

	return true;
}

bool MusicLibDb::Open()
{
	//string libpath(PRMS_LIB_PATH);
	_dbPath = GetConfigValue<string>(PRMS_LIB_PATH) + DB_FILE_NAME;
	int rc = sqlite3_open_v2(_dbPath.c_str(), &_db, SQLITE_OPEN_READWRITE, NULL);
	if(rc)
	{
		Log("Failed to open database file. Creating.", IBALogger::LOGS_ERROR);
		return Create();
	}

	return true;
}

using namespace filesystem;
using namespace TagLib;

void MusicLibDb::Populate()
{
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
}

const char* SQL_Insert_Genre = "INSERT INTO Genre(Name) VALUES(?001);";
const char* SQL_Find_Genre = "SELECT Id FROM Genre WHERE Name = ?001;";
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

const char* SQL_Insert_Artist = "INSERT INTO Artist(Name) VALUES(?001);";
const char* SQL_Find_Artist = "SELECT Id FROM Artist WHERE Name = ?001;";
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

const char* SQL_Insert_Album = "INSERT INTO Album(ArtistId, Title) VALUES(?001, ?002);";
const char* SQL_Find_Album = "SELECT Id FROM Album WHERE ArtistId = ?001 AND Title = ?002;";
int MusicLibDb::AddAlbum(Tag* mtag, int artistId)
{
	string name = (mtag->album()).to8Bit();
	if(_lastAlbumName == name && _lastAlbumArtistId == artistId)
		return _lastAlbumId;

	int rc = sqlite3_bind_int(_psqlFindAlbum, 1, artistId);
	rc = sqlite3_bind_text(_psqlFindAlbum, 2, name.c_str(), -1, SQLITE_STATIC);
	if(SQLITE_ROW == (rc = sqlite3_step(_psqlFindAlbum)))
	{
		_lastAlbumId = sqlite3_column_int(_psqlFindAlbum, 0);
	}
	else
	{
		rc = sqlite3_bind_int(_psqlInsertAlbum, 1, artistId);
		rc = sqlite3_bind_text(_psqlInsertAlbum, 2, name.c_str(), -1, SQLITE_STATIC);
		rc = sqlite3_step(_psqlInsertAlbum);
		_lastAlbumId = sqlite3_last_insert_rowid(_db);
		sqlite3_reset(_psqlInsertAlbum);
	}
	_lastAlbumName = name;
	_lastAlbumArtistId == artistId;
	sqlite3_reset(_psqlFindAlbum);

	return _lastAlbumId;
}

const char* SQL_Insert_Track = "INSERT INTO Track(Number, Title, GenreId, AlbumId, ArtistId, Path) VALUES(?001, ?002, ?003, ?004, ?005, ?006);";
void MusicLibDb::AddTrack(Tag* mtag, const string& path)
{
	int genreId = AddGenre(mtag);
	int artistId = AddArtist(mtag);
	int albumId = AddAlbum(mtag, artistId);

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

const char* SQL_Insert_Playlist = "INSERT INTO Playlist(Name, Path) VALUES(?001, ?002);";
const char* SQL_Find_Playlist = "SELECT Id FROM Playlist WHERE Path = ?001;";
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

const char* SQL_Insert_PlaylistTrack = "INSERT INTO PlaylistTrack(PlaylistId, TrackId) VALUES(?001, ?002);";
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

void MusicLibDb::PrepareAddStatements()
{
	const char* tail; // ignored
	int rc;
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Genre, -1, &_psqlInsertGenre, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Find_Genre, -1, &_psqlFindGenre, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Artist, -1, &_psqlInsertArtist, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Find_Artist, -1, &_psqlFindArtist, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Album, -1, &_psqlInsertAlbum, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Find_Album, -1, &_psqlFindAlbum, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Track, -1, &_psqlInsertTrack, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Insert_Playlist, -1, &_psqlInsertPlaylist, &tail);
	rc = sqlite3_prepare_v2(_db, SQL_Find_Playlist, -1, &_psqlFindPlaylist, &tail);
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
