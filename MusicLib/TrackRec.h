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
// TrackRec.h: interface for the TrackRec class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _IBA_TRACK_REC_H_
#define _IBA_TRACK_REC_H_

#include "TagRec.h"

#include <vector>

typedef unsigned int    tracknum_t;

class TrackRec : public TagRec
{
public:
    TrackRec(const string& fileName,
             const string& title,
                tracknum_t trackNum,
                  taguid_t artistUid,
                  taguid_t genreUid);
	virtual ~TrackRec();

    virtual void       addParentRefNdx(refndx_t);

    refndx_t    getAlbumTblNdx() const;
    void        setAlbumTblNdx(refndx_t);
    
    const tracknum_t          m_trackNum;
    const taguid_t            m_artistUid;
    const taguid_t            m_genreUid;
    const string              m_file;

    virtual void       save(ostream&);
    
    virtual void       print(ostream& output = cout) const;

private:
    refndx_t               m_albumTblNdx;
};

/////////////////////////////////////////////
class TrackTable : public TagTable
{
public:
    void               add(TrackRec*);
    virtual void       load(istream&);
};


#endif
