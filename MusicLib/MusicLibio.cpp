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

#include "MusicLibrary.h"
#include "fileref.h"
#include "tag.h"
#include "file_iterator.h"

#include <fstream>

#define  _SAVE(v)     file.write((char*) (&(v)), sizeof(v))
#define  _SAVES(v,s)  file.write((char*) (&(v)), s)
#define  _LOAD(v)     file.read((char*) (&(v)), sizeof(v))

void MusicLibrary::savestr(ostream& file, const string& str)
{
    // without the terminating 0x00
    size_t len = str.length();
    _SAVE(len);
    file.write(str.c_str(), len);
}

char MusicLibrary::m_rwbuff[READ_BUFFER_SIZE];

char* MusicLibrary::loadstr(istream& file)
{
    size_t len;
    _LOAD(len);
    file.read(m_rwbuff, len);
    // set terminating 0x00
    m_rwbuff[len] = 0x00;
    return m_rwbuff;
}

void TagRec::save(ostream& file)
{
    //if(!file.is_open()) return;
    
    compactChildRefs();
    
    _SAVE(m_uid);
    
    MusicLibrary::savestr(file, m_tag);

    size_t count = m_childRefTbl.size();
    _SAVE(count);
    
    for(size_t n = 0; n < count; n++)
        _SAVES(m_childRefTbl[n], sizeof(refndx_t));
}

void TrackRec::save(ostream& file)
{
    //if(!file.is_open()) return;

    MusicLibrary::savestr(file, getTag());
    
    _SAVE(m_albumTblNdx);
    _SAVE(m_artistUid);
    _SAVE(m_genreUid);
    _SAVE(m_trackNum);
    
    MusicLibrary::savestr(file, m_file);

}


//////////////////////////////////////////////////

void TagTable::save(ostream& file) const
{
    //if(!file.is_open()) return;

    size_t count = size();
    _SAVE(count);
    for(size_t n = 0; n < count; n++)
        m_tbl[n]->save(file);
}

void TagTable::load(istream& file)
{
    //if(!file.is_open()) return;

    size_t count = 0;
    _LOAD(count);
    for(size_t n = 0; n < count; n++)
    {
        taguid_t uid;
        _LOAD(uid);

        char* ctag = MusicLibrary::loadstr(file);
        TagRec* tagRec = new TagRec(uid, string(ctag));
        
        size_t childCount = 0;
        _LOAD(childCount);
        refndx_t refndx;
        for(size_t m = 0; m < childCount; m++)
        {
            _LOAD(refndx);
            tagRec->getChildRefTbl().push_back(refndx);
        }

        m_tbl.push_back(tagRec);
    }
}

///////////////////////////////////////////////////

void TrackTable::load(istream& file)
{
    //if(!file.is_open()) return;

    size_t count = 0;
    _LOAD(count);

    for(size_t n = 0; n < count; n++)
    {
        char* ctag = MusicLibrary::loadstr(file);
        string tag(ctag);
        
        refndx_t albumTblNdx;
        _LOAD(albumTblNdx);

        taguid_t artistUid;
        _LOAD(artistUid);

        taguid_t genreUid;
        _LOAD(genreUid);

        tracknum_t trackNum;
        _LOAD(trackNum);

        char* cfname = MusicLibrary::loadstr(file);
       
        TrackRec* trackRec = new TrackRec(string(cfname),
                                            tag,
                                            trackNum,
                                            artistUid,
                                            genreUid);
        
        trackRec->setAlbumTblNdx(albumTblNdx);

        m_tbl.push_back(trackRec);
    }
}


//////////////////////////////////////////////////
void MusicLibrary::reindex(/*const string& libDir*/)
{
    init();

    scanDir();

    m_genreTbl.sort();
    m_genreTbl.setParentRefs(m_artistTbl);

    m_artistTbl.sort();
    m_artistTbl.setChildRefs(m_genreTbl);
    m_artistTbl.setParentRefs(m_albumTbl);

    m_albumTbl.sort();
    m_albumTbl.setChildRefs(m_artistTbl);
    m_albumTbl.setParentRefs(m_trackTbl/*, false*/);

    m_trackTbl.sort();

    orderAlbumTracks();
    
    save();
}

void MusicLibrary::save()
{
    ofstream ndxFile;
    ndxFile.open(m_ndxFilePath.c_str(), ios_base::out | ios_base::binary);

    if(!ndxFile.is_open()) return;

    m_genreTbl.save(ndxFile);
    m_artistTbl.save(ndxFile);
    m_albumTbl.save(ndxFile);
    m_trackTbl.save(ndxFile);

    ndxFile.close();
    
    ostringstream out;
    print(out);
    m_logger->log(out.str().c_str(), IBALogger::LOGS_INFO);
}

void MusicLibrary::load()
{
    ifstream ndxFile;
    ndxFile.open(m_ndxFilePath.c_str(), ios_base::in | ios_base::binary);
    if(!ndxFile.is_open()) return;

    m_genreTbl.load(ndxFile);
    m_artistTbl.load(ndxFile);
    m_albumTbl.load(ndxFile);
    m_trackTbl.load(ndxFile);

    ndxFile.close();
    
    //print();
}

using namespace filesystem;
using namespace TagLib;

void MusicLibrary::scanDir()
{
    for( file_iterator<> itFile(m_libPath);
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
            continue;
            
        FileRef mfile(fileName.c_str(), true, AudioProperties::Fast);
        if(NULL == mfile.audioProperties())
            continue;
        
        Tag* mtag = mfile.tag();
        if(NULL != mtag)
        {
            addTrack(fileName,
                        (mtag->genre()).to8Bit(),
                        (mtag->artist()).to8Bit(),
                        (mtag->album()).to8Bit(),
                        (mtag->title()).to8Bit(),
                        (tracknum_t) mtag->track());
        }
        
    }
}

void MusicLibrary::print(ostream& output) const
{
    
    output << "\n\n*********************\nGenres:\n";
    m_genreTbl.print(output);
    
    output << "\nArtists:\n";
    m_artistTbl.print(output);
    
    output << "\nAlbums:\n";
    m_albumTbl.print(output);
    
    output << "\nTracks:\n";
    m_trackTbl.print(output);
}
