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
// TrackRec.cpp: implementation of the TrackRec class.
//
//////////////////////////////////////////////////////////////////////

#include "TrackRec.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TrackRec::~TrackRec()
{

}

TrackRec::TrackRec(const string& fileName,
                   const string& title,
                      tracknum_t trackNum,
                        taguid_t artistUid,
                        taguid_t genreUid) :
        TagRec(0, title),
        m_file(fileName),
        m_trackNum(trackNum),
        m_artistUid(artistUid),
        m_genreUid(genreUid),
        m_albumTblNdx(0)
{
}

void TrackRec::addParentRefNdx(refndx_t ndx)
{
    setAlbumTblNdx(ndx);
}

void TrackRec::setAlbumTblNdx(refndx_t ndx)
{
    m_albumTblNdx = ndx;
}

refndx_t TrackRec::getAlbumTblNdx() const
{
    return m_albumTblNdx;
}

void TrackRec::print(ostream& output) const
{
    output << m_trackNum;
    output << ": ";
    output << m_tag;
    output << " [";
    output << m_albumTblNdx;
    output << "] A:";
    output << m_artistUid;
    output << " G:";
    output << m_genreUid;
    output << ", ";
    output << m_file;
    output << '\n';

}


//////////////////////////////////////////

void TrackTable::add(TrackRec* trackRec)
{
    if(NULL == trackRec)
        return;

    m_tbl.push_back(trackRec);
}

/*
void TrackTable::print(ostream& output) const
{
    size_t count = m_tbl.size();
    for(refndx_t ndx = 0; ndx < count; ndx++)
    {
        output << ndx;
        output << ": ";
        TrackRec& trk = (TrackRec&) m_tbl[ndx];
        trk.printt(output);
    }
}*/
