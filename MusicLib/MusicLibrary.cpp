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

// MusicLibrary.cpp: implementation of the MusicLibrary class.
//
//////////////////////////////////////////////////////////////////////

#include "MusicLibrary.h"

#include <assert.h>

const char* NDX_FILE_NAME   = "IBALib.ndx";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MusicLibrary::MusicLibrary(MusicLibMngr& libMngr)
    : m_config(NULL),
      m_logger(NULL)
{
    m_libMngr = &libMngr;
}

MusicLibrary::~MusicLibrary()
{

}


void MusicLibrary::init()
{
    m_config = &m_libMngr->getConfig();
    m_logger = &m_libMngr->getLogger();
    
    m_config->getValue(IBAConfig::PRM_LIB_PATH, m_libPath);
    m_ndxFilePath = m_libPath + '/' + NDX_FILE_NAME;
    
    m_genreTbl.init();
    m_artistTbl.init();
    m_albumTbl.init();
    m_trackTbl.init();
}


void MusicLibrary::addTrack(const string& file,
                            const string& genre,
                            const string& artist,
                            const string& album,
                            const string& title,
                            tracknum_t track)
{
    bool isGenReg = false;
    bool isArtReg = false;
    bool isAlbReg = false;
    
    refndx_t genNdx = NDX_INVALID;
    TagRec* genRec = m_genreTbl.reg(genre, genNdx, isGenReg);

    refndx_t artNdx = NDX_INVALID;
    TagRec* artRec = m_artistTbl.reg(artist, artNdx, isArtReg);

    refndx_t albNdx = NDX_INVALID;
    TagRec* albRec = m_albumTbl.reg(album, albNdx, isAlbReg);

    if(!isArtReg)
        genRec->addChildRefNdx(artNdx);
    if(!isAlbReg)
        artRec->addChildRefNdx(albNdx);

    TrackRec* trkRec = new TrackRec(file, title, track, artRec->getUID(), genRec->getUID());
    trkRec->addParentRefNdx(albNdx);
    m_trackTbl.add(trkRec);
    refndx_t trkNdx = m_trackTbl.size() - 1;
    albRec->addChildRefNdx(trkNdx);

    //print();
}

void MusicLibrary::orderAlbumTracks()
{
    // scan each album's tracks
    // use track number as index into album's vector
    // the indexes in the album's child vector are in order of the
    // thrack they are pointing to

    size_t trackCount = m_trackTbl.size();
    for(refndx_t ndx = 0; ndx < trackCount; ndx++)
    {
        TrackRec* trackRec = (TrackRec*) m_trackTbl.getRecAt(ndx);
        refndx_t albNdx = trackRec->getAlbumTblNdx();
        TagRec* albRec = m_albumTbl.getRecAt(albNdx);
        reftbl_t& childRefs = albRec->getChildRefTbl();
        
        // need to set the index ndx in position trackRec->m_trackNum
        // will have to manually increase the vector
        setAt(childRefs, trackRec->m_trackNum - 1, ndx);
    }
}

void MusicLibrary::setAt(reftbl_t& vect, refndx_t ndx, size_t val)
{
    if(ndx < 0) return;
    
    size_t count = vect.size();
    if(ndx >= count)
        vect.resize(max(count + count/2, (size_t) (ndx + 1)), NDX_INVALID);
    
    vect[ndx] = val;
}

const TagTable& MusicLibrary::getGenreTbl() const
{
    return m_genreTbl;
}

const TagTable& MusicLibrary::getArtistTbl() const
{
    return m_artistTbl;
}

const TagTable& MusicLibrary::getAlbumTbl() const
{
    return m_albumTbl;
}

const TrackTable& MusicLibrary::getTrackTbl() const
{
    return m_trackTbl;
}


///////////////////////////////////////////////////

MusicLibBrowser::MusicLibBrowser(MusicLibMngr& libMngr)
    : m_musicLib(NULL),
      m_config(NULL),
      m_logger(NULL)
{
    m_libMngr = &libMngr;
    m_brLevel = BR_LEVEL_ARTIST;
    m_childPagingSize = CHILD_PAGING_SIZE;
}

void MusicLibBrowser::init()
{
    m_config = &m_libMngr->getConfig();
    m_logger = &m_libMngr->getLogger();
    m_musicLib = &m_libMngr->getLibrary();


    m_brTbl[BR_LEVEL_GENRE].m_tagTbl = &m_musicLib->getGenreTbl();
    m_brTbl[BR_LEVEL_GENRE].m_recCount = m_brTbl[BR_LEVEL_GENRE].m_tagTbl->size();
    m_brTbl[BR_LEVEL_GENRE].m_currNdx = 0;
    m_brTbl[BR_LEVEL_GENRE].m_childNdx = 0;
    m_brTbl[BR_LEVEL_GENRE].m_pgSize = 1;

    m_brTbl[BR_LEVEL_ARTIST].m_tagTbl = &m_musicLib->getArtistTbl();
    m_brTbl[BR_LEVEL_ARTIST].m_recCount = m_brTbl[BR_LEVEL_ARTIST].m_tagTbl->size();
    m_brTbl[BR_LEVEL_ARTIST].m_currNdx = 0;
    m_brTbl[BR_LEVEL_ARTIST].m_childNdx = 0;
    m_brTbl[BR_LEVEL_ARTIST].m_pgSize = 1;

    m_brTbl[BR_LEVEL_ALBUM].m_tagTbl = &m_musicLib->getAlbumTbl();
    m_brTbl[BR_LEVEL_ALBUM].m_recCount = m_brTbl[BR_LEVEL_ALBUM].m_tagTbl->size();
    m_brTbl[BR_LEVEL_ALBUM].m_currNdx = 0;
    m_brTbl[BR_LEVEL_ALBUM].m_childNdx = 0;
    m_brTbl[BR_LEVEL_ALBUM].m_pgSize = 1;

    m_brTbl[BR_LEVEL_TRACK].m_tagTbl = &m_musicLib->getTrackTbl();
    m_brTbl[BR_LEVEL_TRACK].m_recCount = m_brTbl[BR_LEVEL_TRACK].m_tagTbl->size();
    m_brTbl[BR_LEVEL_TRACK].m_currNdx = 0;
    m_brTbl[BR_LEVEL_TRACK].m_childNdx = 0;
    m_brTbl[BR_LEVEL_TRACK].m_pgSize = 1;
    
    
    float pgSize = 0;    
    m_config->getValue(IBAConfig::PRM_PAGE_SIZE, pgSize);    
    if(pgSize <= 0)
    {
        m_logger->log("Invalid page size in configuration. Page size set to 1.\n", IBALogger::LOGS_WARNING);
    }
    else
    {
        if(pgSize >= 1)
            setPageSizeFixed((int) pgSize);
        else
            setPageSizePercent(pgSize);
    }
    
    int lvlStart = 0;
    m_config->getValue(IBAConfig::PRM_START_BROWSE, lvlStart);
    setStartLevel((BrowseLevel)lvlStart);
}

const string& MusicLibBrowser::setStartLevel(BrowseLevel lvl)
{
    if(lvl < BR_LEVEL_GENRE || lvl > BR_LEVEL_TRACK)
        lvl = BR_LEVEL_ARTIST;
    m_brLevel = lvl;
    m_brDepth = m_brLevel;
    return getLvlTag();
}

bool MusicLibBrowser::isChildBrowse() const
{
    if(m_brDepth == m_brLevel)
        return false;
        
    if(m_brDepth > m_brLevel)
        return true;
    
    assert(false);
}

const string& MusicLibBrowser::lvlDn()
{
    assert(m_brDepth >= m_brLevel && m_brDepth <= BR_LEVEL_TRACK);
    
    if(isChildBrowse())
    {
        if(m_brDepth == BR_LEVEL_TRACK)
            return getChildRec(m_brDepth).getTag();
            
        refndx_t refndx = getChildRefNdx(m_brDepth);
        brtbl_t& st = m_brTbl[m_brDepth];
        st.m_currNdx = refndx;
        st.m_childNdx = 0;
        m_brDepth++;
        return toChildNdx(st.m_childNdx);
    }
    else
    {
        if(m_brLevel == BR_LEVEL_TRACK)
            return getLvlTag();
            
        brtbl_t& st = m_brTbl[m_brLevel];
        st.m_childNdx = 0;
        m_brDepth = m_brLevel + 1;
        return toChildNdx(st.m_childNdx); 
    }
}

const string& MusicLibBrowser::lvlUp()
{
    assert(m_brDepth >= m_brLevel && m_brDepth <= BR_LEVEL_TRACK);

    m_brDepth--;
    if(m_brDepth < m_brLevel)
        m_brDepth = m_brLevel;
    
    if(isChildBrowse()) 
        return getLvlTag(m_brDepth);
    else
        return getLvlTag();
}

const string& MusicLibBrowser::toChildNdx(int offset)
{
    if(!isChildBrowse())
        return getLvlTag();
    
    brtbl_t& st = m_brTbl[m_brDepth-1];
    st.m_childNdx += offset;
    const TagRec& rec = getChildRec(m_brDepth);
    return rec.getTag();
}

const string& MusicLibBrowser::toNdx(int offset)
{
    brtbl_t& st = m_brTbl[m_brLevel];
    if(offset < 0)
        st.m_currNdx = (st.m_recCount + st.m_currNdx + offset) % st.m_recCount;
    else
        st.m_currNdx += offset;
    return getRecord().getTag();
}

refndx_t MusicLibBrowser::getChildRefNdx(int brLvl)
{
    assert(brLvl > BR_LEVEL_GENRE && brLvl <= BR_LEVEL_TRACK);
    
    brtbl_t& st = m_brTbl[brLvl-1];
    const TagRec& tagrec = getRecord(brLvl-1, st.m_currNdx);
    refndx_t refndx = tagrec.getChildRefNdxAt(st.m_childNdx);
    return refndx;
}

const TagRec& MusicLibBrowser::getChildRec(int brLvl)
{
    assert(brLvl > BR_LEVEL_GENRE && brLvl <= BR_LEVEL_TRACK);
    
    const TagRec& tagrec = getRecord(brLvl, getChildRefNdx(brLvl));
    return tagrec;
}

const TagRec& MusicLibBrowser::getRecord(int brLvl, refndx_t ndx) const
{
    assert(brLvl >= BR_LEVEL_GENRE && brLvl <= BR_LEVEL_TRACK);
    
    return *((m_brTbl[brLvl].m_tagTbl)->getRecAt(ndx));
}

const TagRec& MusicLibBrowser::getRecord(int brLvl) const
{
    refndx_t ndx = m_brTbl[brLvl].m_currNdx;
    return getRecord(brLvl, ndx);
}

const TagRec& MusicLibBrowser::getRecord() const
{
    return getRecord(m_brLevel);
}

const string& MusicLibBrowser::getLvlTag(int brLvl) const
{
    return getRecord(brLvl).getTag();
}

const string& MusicLibBrowser::getLvlTag() const
{
    return getLvlTag(m_brLevel);
}

const string& MusicLibBrowser::next()
{
    return isChildBrowse() ? toChildNdx(1) : toNdx(1);
}

const string& MusicLibBrowser::prev()
{
    return isChildBrowse() ? toChildNdx(-1) : toNdx(-1);
}

const string& MusicLibBrowser::pgDn()
{
    return isChildBrowse() ? toChildNdx(m_childPagingSize) : toNdx(m_brTbl[m_brLevel].m_pgSize);
}

const string& MusicLibBrowser::pgUp()
{
    return isChildBrowse() ? toChildNdx(-m_childPagingSize) : toNdx(-m_brTbl[m_brLevel].m_pgSize);
}

const string& MusicLibBrowser::first()
{
    if(isChildBrowse())
    {
        m_brTbl[m_brDepth-1].m_childNdx = NDX_FIRST;
        return toChildNdx(0); // same one
    }
    else
    {
        m_brTbl[m_brLevel].m_currNdx = NDX_FIRST;
        return toNdx(0);
    }
}

const string& MusicLibBrowser::last()
{
    if(isChildBrowse())
    {
        m_brTbl[m_brDepth-1].m_childNdx = NDX_LAST;
        return toChildNdx(0);
    }
    else
    {
        m_brTbl[m_brLevel].m_currNdx = NDX_LAST;
        return toNdx(0);
    }
}

const string& MusicLibBrowser::getGenre() const
{
    return getLvlTag(BR_LEVEL_GENRE);
}

const string& MusicLibBrowser::getArtist() const
{
    return getLvlTag(BR_LEVEL_ARTIST);
}

const string& MusicLibBrowser::getAlbum() const
{
    return getLvlTag(BR_LEVEL_ALBUM);
}

const string& MusicLibBrowser::getTrack() const
{
    return getLvlTag(BR_LEVEL_TRACK);
}


void MusicLibBrowser::setPageSizeFixed(int pgSize)
{
    if(pgSize <= 0) return;

    m_brTbl[BR_LEVEL_GENRE].m_pgSize = 
    m_brTbl[BR_LEVEL_ARTIST].m_pgSize = 
    m_brTbl[BR_LEVEL_ALBUM].m_pgSize = 
    m_brTbl[BR_LEVEL_TRACK].m_pgSize = pgSize;
}

void MusicLibBrowser::setPageSizePercent(float pgPercent)
{
    if(pgPercent <= 0 || pgPercent >= 1) return;

    m_brTbl[BR_LEVEL_GENRE].m_pgSize = (int) (m_brTbl[BR_LEVEL_GENRE].m_recCount * pgPercent);
    m_brTbl[BR_LEVEL_ARTIST].m_pgSize = (int)(m_brTbl[BR_LEVEL_ARTIST].m_recCount * pgPercent);
    m_brTbl[BR_LEVEL_ALBUM].m_pgSize = (int) (m_brTbl[BR_LEVEL_ALBUM].m_recCount * pgPercent);
    m_brTbl[BR_LEVEL_TRACK].m_pgSize = (int) (m_brTbl[BR_LEVEL_TRACK].m_recCount * pgPercent);
}


const playlist_t& MusicLibBrowser::getDefaultPlaylist()
{
    // for now I'll return some album in the library
    // TODO: persist current playlist and load/return that one

    setStartLevel(BR_LEVEL_ALBUM);    
    toNdx(time(NULL) % m_brTbl[BR_LEVEL_ALBUM].m_recCount);
    return select();
}


const playlist_t& MusicLibBrowser::select()
{
    m_playList.clear();
    const TagRec& currLvlRec = isChildBrowse() ? getChildRec(m_brDepth) : getRecord();

    switch(m_brDepth)
    {
    case BR_LEVEL_GENRE:
        selectGenre(m_playList, currLvlRec);
        break;

    case BR_LEVEL_ARTIST:
        selectArtist(m_playList, currLvlRec);
        break;

    case BR_LEVEL_ALBUM:
        selectAlbum(m_playList, currLvlRec);
        break;

    case BR_LEVEL_TRACK:
        m_playList.push_back(&((TrackRec&) currLvlRec).m_file);
        break;
    }

    logSelection();
    
    return m_playList;
}

void MusicLibBrowser::logSelection()
{
    if(m_logger->getLogLevel() & IBALogger::LOGS_DEBUG)
    {
        ostringstream out;
        out << "Selection:" << endl;
        for(playlist_t::const_iterator it = m_playList.begin();
            it != m_playList.end();
            it++)
            out << **it << endl; 
    
        m_logger->log(out.str().c_str(), IBALogger::LOGS_DEBUG);
    }
}

playlist_t& MusicLibBrowser::selectAlbum(playlist_t& playList, const TagRec& album) const
{
    const reftbl_t& tracks = album.getChildRefTbl();
    size_t count = tracks.size();
    for(size_t n = 0; n < count; n++)
    {
        refndx_t trkNdx = tracks[n];
        playList.push_back( &((const TrackRec&) getRecord(BR_LEVEL_TRACK, trkNdx)).m_file );
    }

    return playList;
}

playlist_t& MusicLibBrowser::selectArtist(playlist_t& playList, const TagRec& artist) const
{
    const reftbl_t& albTbl = artist.getChildRefTbl();
    size_t count = albTbl.size();
    for(size_t n = 0; n < count; n++)
    {
        refndx_t albNdx = albTbl[n];
        const TagRec& album = getRecord(BR_LEVEL_ALBUM, albNdx);
        selectAlbum(playList, album);
    }

    return playList;
}

playlist_t& MusicLibBrowser::selectGenre(playlist_t& playList, const TagRec& genre) const
{
    const reftbl_t& artTbl = genre.getChildRefTbl();
    size_t count = artTbl.size();
    for(size_t n = 0; n < count; n++)
    {
        refndx_t artNdx = artTbl[n];
        const TagRec& artist = getRecord(BR_LEVEL_ARTIST, artNdx);
        selectArtist(playList, artist);
    }

    return playList;
}


////////////////////////////////////////////////////

MusicLibMngr MusicLibMngr::single_mlMngr;
MusicLibMngr& MusicLibMngr::getMLibMngr()
{
    return single_mlMngr;
}

MusicLibMngr::MusicLibMngr()
    : m_mLib(NULL),
      m_mBro(NULL),
      m_logger(NULL),
      m_config(NULL)
{
}

MusicLibMngr::~MusicLibMngr()
{
    if(NULL != m_mBro)
        delete m_mBro;

    if(NULL != m_mLib)
        delete m_mLib;
}

void MusicLibMngr::reindex()
{
    if(NULL != m_mLib)
        delete m_mLib;
    m_mLib = new MusicLibrary(*this);
    
    m_mLib->reindex();
}

void MusicLibMngr::init(IBAConfig& config, IBALogger& logger)
{
    m_config = &config;
    m_logger = &logger;

    if(NULL != m_mLib)
        delete m_mLib;
    m_mLib = new MusicLibrary(*this);
    m_mLib->init();

    if(NULL != m_mBro)
        delete m_mBro;
    m_mBro = new MusicLibBrowser(*this);
    //m_mBro->init();
}

MusicLibBrowser& MusicLibMngr::getBrowser()
{
    return *m_mBro;
}

MusicLibrary& MusicLibMngr::getLibrary()
{
    return *m_mLib;
}

IBAConfig& MusicLibMngr::getConfig()
{
    return *m_config;
}

IBALogger& MusicLibMngr::getLogger()
{
    return *m_logger;
}

