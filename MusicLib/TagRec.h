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
// TagRec.h: interface for the TagRec class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _IBA_TAG_REC_H_
#define _IBA_TAG_REC_H_

#include <vector>
#include <string>
#include <algorithm>
#include <map>

#include <iostream>

using namespace std;

class TagTable;
class TagRec;


typedef unsigned int        refndx_t;
typedef unsigned int        taguid_t;
typedef vector<refndx_t>    reftbl_t;
typedef vector<TagRec*>     tagtbl_t; // container of pointers

#define UID_INVALID         0
#define NDX_INVALID         (refndx_t)(-1)
#define NDX_FIRST           (refndx_t)(-2)
#define NDX_LAST            (refndx_t)(-3)

taguid_t nextUid();


class CmpLessTag //: binary_function<string, string, bool> 
{
public:
    bool operator()(const TagRec* tagL, const TagRec* tagR) const;
};

class TagRec
{
public:
	TagRec(taguid_t, const string&);
	virtual ~TagRec();

    static TagRec      BLANK;

    const string&      getTag() const;
    taguid_t           getUID() const;

    virtual void       addChildRefNdx(refndx_t);
    virtual void       addParentRefNdx(refndx_t);
    
    refndx_t           getChildRefNdxAt(refndx_t& ndx) const;

    reftbl_t&          getChildRefTbl();
    const reftbl_t&    getChildRefTbl() const;
    reftbl_t&          getParentRefTbl();
    const reftbl_t&    getParentRefTbl() const;
    
    void               setAsParent(TagTable& childTbl, refndx_t, bool clearChildRefs = true);
    void               setAsChild(TagTable& parentTbl, refndx_t);

    bool operator < (const TagRec&) const;

    virtual void       save(ostream&);
    
    virtual void       print(ostream& output = cout) const;

protected:
    void               compactChildRefs();
    
    string      m_tag;
    taguid_t    m_uid;

    reftbl_t    m_childRefTbl;
    reftbl_t    m_parentRefTbl;

    //const string&   getCompTag() const;
};

// todo: replace with hash
typedef map<string, refndx_t/*, CmpLessTag*/>  lookup_t;
//typedef hash_map<string, refndx_t> lookup_t;

/////////////////////////////////////////////
class TagTable
{
public:
    TagTable();
    virtual ~TagTable();

    void               init();
    void               sort();
    size_t             size() const;
    //tagtbl_t&          records();
    
    TagRec*            getRecAt(refndx_t& ndx) const;

    TagRec*            reg(const string&, refndx_t&, bool& isReg);

    void               setParentRefs(TagTable& childTbl, bool clearChildRefs = true);
    void               setChildRefs(TagTable& parentTbl);

    virtual void       save(ostream&) const;
    virtual void       load(istream&);
    
    virtual void       print(ostream& output = cout) const;

protected:
    TagRec*            operator[](refndx_t) const;
    
    refndx_t           lookUp(const string&);
    void               hash(const string&, refndx_t);

    tagtbl_t           m_tbl;

    lookup_t           m_refndxByTag;
    
    CmpLessTag         m_comparator;
};


/////////////////////////////////////////////
class UIDGen
{
public:
    taguid_t       nextUid() {return m_nextuid++;};

    static UIDGen& getUIDGen();

private:
    UIDGen();
    ~UIDGen();

    static UIDGen*      m_uidGen;

    taguid_t            m_nextuid;
};

#endif
