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

// MusicLibrary.h: interface for the MusicLibrary class.
//

//////////////////////////////////////////////////////////////////////

#ifndef _IBA_MUSIC_LIBRARY_H_
#define _IBA_MUSIC_LIBRARY_H_

#include "TrackRec.h"
#include "../IBAConfig.h"
#include "../IBALogger.h"

#define READ_BUFFER_SIZE    512


#define CHILD_PAGING_SIZE   3


class MusicLibMngr;

//////////////////////////////////////////////

class MusicLibrary  
{
public:
	MusicLibrary(MusicLibMngr&);
	virtual ~MusicLibrary();

    void        init();
    void        reindex();
  
    void        load();
    void        save();

    const TagTable&     getGenreTbl() const;
    const TagTable&     getArtistTbl() const;
    const TagTable&     getAlbumTbl() const;
    const TrackTable&   getTrackTbl() const;
    
    void        print(ostream& output = cout) const;

    static void    savestr(ostream&, const string&);
    static char*   loadstr(istream&); // each call overrites the buffer
    
private:
    void        scanDir();
    
    void        addTrack(const string& file,
                         const string& genre,
                         const string& artist,
                         const string& album,
                         const string& title,
                         tracknum_t track
                        );

    void        orderAlbumTracks();
    
    static void setAt(reftbl_t& vect, refndx_t ndx, size_t val);

    MusicLibMngr*     m_libMngr;
    IBAConfig*        m_config;
    IBALogger*        m_logger;

    TagTable    m_genreTbl;
    TagTable    m_artistTbl;
    TagTable    m_albumTbl;
    TrackTable  m_trackTbl;

    string      m_libPath;
    string      m_ndxFilePath;
    
    // buffer for string read
    static char m_rwbuff[READ_BUFFER_SIZE];
};

//////////////////////////////////////////////////
typedef vector<const string*> playlist_t;

class MusicLibBrowser
{
public:
    enum BrowseLevel
    {
        BR_LEVEL_GENRE = 0,
        BR_LEVEL_ARTIST,
        BR_LEVEL_ALBUM,
        BR_LEVEL_TRACK,
    };

    MusicLibBrowser(MusicLibMngr&);
    ~MusicLibBrowser() {};

    void                init();

    const string&       next();
    const string&       prev();
    const string&       first();
    const string&       last();
    const string&       pgDn();
    const string&       pgUp();
    
    const string&       lvlUp();
    const string&       lvlDn();
    
    const string&       setStartLevel(BrowseLevel);
    const playlist_t&   select();
    const playlist_t&   getDefaultPlaylist();

    const string&       getGenre() const;
    const string&       getArtist() const;
    const string&       getAlbum() const;
    const string&       getTrack() const;

    const string&       getLvlTag() const;
    
private:
    const string&       toNdx(int offset);
    const string&       toChildNdx(int offset);
    const string&       getLvlTag(int) const;

    bool                isChildBrowse() const;
    playlist_t&         selectAlbum(playlist_t&, const TagRec& album) const;
    playlist_t&         selectArtist(playlist_t&, const TagRec& artist) const;
    playlist_t&         selectGenre(playlist_t&, const TagRec& genre) const;
    
    const TagRec&        getRecord(int, refndx_t) const;
    const TagRec&        getRecord(int) const;
    const TagRec&        getRecord() const;
    refndx_t             getChildRefNdx(int);
    const TagRec&        getChildRec(int);

    // if pgSize < 1 then it is interpreted as percentage of the total list size
    // if pgSize >= 1 then it is simply number of records to page on the list
    void                 setPageSizeFixed(int);
    void                 setPageSizePercent(float);

    void                 logSelection();
        
    MusicLibMngr*                 m_libMngr;
    MusicLibrary*                 m_musicLib;
    IBAConfig*                    m_config;
    IBALogger*                    m_logger;
    
    int                           m_childPagingSize;
    int                           m_brLevel;
    int                           m_brDepth; // should be equal or less than m_brLevel. Acts like stack pointer.

    struct brtbl_t
    {
        const TagTable*        m_tagTbl;
        size_t                 m_recCount;
        refndx_t               m_currNdx;
        refndx_t               m_childNdx;
        int                    m_pgSize;
    };

    brtbl_t                    m_brTbl[4];
    playlist_t                 m_playList;
};


/////////////////////////////////////////////////

class MusicLibMngr
{

public:
    virtual ~MusicLibMngr();

    static MusicLibMngr& getMLibMngr();

    void                    init(IBAConfig&, IBALogger&);
    void                    reindex();
    
    MusicLibrary&           getLibrary();
    MusicLibBrowser&        getBrowser();
    IBAConfig&              getConfig();
    IBALogger&              getLogger();

private:
    MusicLibMngr();
    
    static MusicLibMngr  single_mlMngr;

    MusicLibrary*        m_mLib;
    MusicLibBrowser*     m_mBro;
    
    IBAConfig*           m_config;
    IBALogger*           m_logger;
};

#endif
