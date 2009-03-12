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

#include "TagRec.h"

#include <assert.h>

const char* EMPTY_TAG = "|--|";
const char* START_A_TAG = "a ";
const size_t A_TAG_LEN = strlen(START_A_TAG);
const char* START_THE_TAG = "the ";
const size_t THE_TAG_LEN = strlen(START_THE_TAG);
/*
string& cmpLess::getCompStr(string& tag)
{
    if(0 == stricmp(tag.substr(0, 2).c_str(), START_A_TAG))
    {
        return tag.substr(2);
    }

    if(0 == stricmp(tag.substr(0, 4).c_str(), START_THE_TAG))
        return tag.substr(4);

    return tag;
}*/

bool cmpTags(const string& L, const string& R)
{
    const char* ltag = L.c_str();
    const char* rtag = R.c_str();

    size_t ltagFrom = 0;
    size_t rtagFrom = 0;

    if(0 == strncasecmp(ltag, START_A_TAG, A_TAG_LEN))
        ltagFrom = A_TAG_LEN;
    else if(0 == strncasecmp(ltag, START_THE_TAG, THE_TAG_LEN))
        ltagFrom = THE_TAG_LEN;

    if(0 == strncasecmp(rtag, START_A_TAG, A_TAG_LEN))
        rtagFrom = A_TAG_LEN;
    else if(0 == strncasecmp(rtag, START_THE_TAG, THE_TAG_LEN))
        rtagFrom = THE_TAG_LEN;
    
    return (strcasecmp(ltag + ltagFrom, rtag + rtagFrom) < 0);
}

bool CmpLessTag::operator()(const TagRec* tagL, const TagRec* tagR) const
{
    return cmpTags(tagL->getTag(), tagR->getTag());
}

////////////////////////////////////////////////////////

TagRec TagRec::BLANK(UID_INVALID, EMPTY_TAG);

TagRec::TagRec(taguid_t uid, const string& tag)
{
    if(tag.empty())
        m_tag = string(EMPTY_TAG);
    else
        m_tag = tag;

    m_uid = uid;
}

TagRec::~TagRec()
{

}

taguid_t TagRec::getUID() const
{
    return m_uid;
}

const string& TagRec::getTag() const
{
    return m_tag;
}


bool TagRec::operator <(const TagRec& tag) const
{
    return cmpTags(m_tag, tag.m_tag);
//    CmpLessTag cmp;
//    return cmp(m_tag, tag.m_tag);
}


/*
const string& TagRec::getCompTag() const
{
    if(0 == stricmp(m_tag.substr(0, 2).c_str(), START_A_TAG))
    {
        return m_tag.substr(2);
    }

    if(0 == stricmp(m_tag.substr(0, 4).c_str(), START_THE_TAG))
        return m_tag.substr(4);

    return m_tag;
}*/

void TagRec::addChildRefNdx(refndx_t refndx)
{
    m_childRefTbl.push_back(refndx);
}

void TagRec::addParentRefNdx(refndx_t refndx)
{
    m_parentRefTbl.push_back(refndx);
}

refndx_t TagRec::getChildRefNdxAt(refndx_t& ndx) const
{
    int count = m_childRefTbl.size();
    assert(count);
    //if(count < 1) return NDX_INVALID;
    
    if(NDX_FIRST == ndx) return m_childRefTbl[0];
    if(NDX_LAST == ndx) return m_childRefTbl[count-1];
    
    // corrects the passed ndx
    ndx = (ndx + count) % count;
    return m_childRefTbl[ndx];
}

reftbl_t& TagRec::getChildRefTbl()
{
    return m_childRefTbl;
}

const reftbl_t& TagRec::getChildRefTbl() const
{
    return m_childRefTbl;
}

reftbl_t& TagRec::getParentRefTbl()
{
    return m_parentRefTbl;
}

const reftbl_t& TagRec::getParentRefTbl() const
{
    return m_parentRefTbl;
}

void TagRec::setAsParent(TagTable& childTbl, refndx_t refndx, bool clearChildRefs)
{
    size_t count = m_childRefTbl.size();
    for(refndx_t ndx = 0; ndx < count; ndx++)
    {
        refndx_t childNdx = m_childRefTbl[ndx];
        TagRec* childRec = childTbl.getRecAt(childNdx);
        childRec->addParentRefNdx(refndx);
    }

    if(clearChildRefs)
        m_childRefTbl.clear();
}

void TagRec::setAsChild(TagTable& parentTbl, refndx_t refndx)
{
    size_t count = m_parentRefTbl.size();
    for(refndx_t ndx = 0; ndx < count; ndx++)
    {
        refndx_t parentNdx = m_parentRefTbl[ndx];
        TagRec* parentRec = parentTbl.getRecAt(parentNdx);
        parentRec->addChildRefNdx(refndx);
    }
}

void TagRec::compactChildRefs()
{
    reftbl_t::iterator it = m_childRefTbl.begin();
    while(it != m_childRefTbl.end())
    {
        if(NDX_INVALID == *it)
            m_childRefTbl.erase(it);
        else
            it++;
    }
}

void TagRec::print(ostream& output) const
{
    output << m_tag;
    output << " (";
    output << m_uid;
    output << ") ";
    
    size_t count = m_childRefTbl.size();
    output << " [";
    for(refndx_t ndx = 0; ndx < count; ndx++)
    {
        refndx_t child_ndx = m_childRefTbl[ndx];
        if(NDX_INVALID == child_ndx)
            output << '_';
        else
            output << child_ndx;
        output << ',';
    }
    output << "]";

    count = m_parentRefTbl.size();
    output << " [";
    for(refndx_t ndx = 0; ndx < count; ndx++)
        output << m_parentRefTbl[ndx] << ',';
    
    output << "]\n";

}

///////////////////////////////////////////////////////

TagTable::TagTable()
{
}

TagTable::~TagTable()
{
    size_t count = m_tbl.size();
    for(size_t ndx = 0; ndx < count; ndx++)
    {
        TagRec* tagrec = m_tbl[ndx];
        if(NULL != tagrec)
            delete tagrec;
    }
}
void TagTable::init()
{
    m_tbl.clear();
    m_refndxByTag.clear();
}

void TagTable::sort()
{
    std::sort(m_tbl.begin(), m_tbl.end(), m_comparator);
}

size_t TagTable::size() const
{
    return m_tbl.size();
}

TagRec* TagTable::operator[](refndx_t ndx) const
{
    return getRecAt(ndx);
}

TagRec* TagTable::getRecAt(refndx_t& ndx) const
{
    //if(NDX_INVALID == ndx) return NULL;
    
    int count = m_tbl.size();
    assert(count);
    //if(count < 1) return NULL;
    
    if(NDX_FIRST == ndx) return m_tbl[0];
    if(NDX_LAST == ndx) return m_tbl[count-1];
    
    ndx = (ndx + count) % count; // corrects ndx
    return m_tbl[ndx];
}

/*
    If the tag is not in the look up table, add it in the main table
    and to the look up table with its index from the main table.
    Return the tag record.
*/
TagRec* TagTable::reg(const string& tag, refndx_t& refndx, bool& isReg)
{
    TagRec* tagrec = NULL;
    
    isReg = true;
    
    refndx_t ndx = lookUp(tag);
    if(NDX_INVALID == ndx)
    {
        tagrec = new TagRec(nextUid(), tag);
        m_tbl.push_back(tagrec);
        ndx = size() - 1;
        hash(tag, ndx);
        isReg = false;
    }
    else
    {
        tagrec = m_tbl[ndx];
    }
    
    refndx = ndx;

    return tagrec;
}

void TagTable::setParentRefs(TagTable& childTbl, bool clearChildRefs)
{
    size_t count = m_tbl.size();
    for(refndx_t ndx = 0; ndx < count; ndx++)
        m_tbl[ndx]->setAsParent(childTbl, ndx, clearChildRefs);
}

void TagTable::setChildRefs(TagTable& parentTbl)
{
    size_t count = m_tbl.size();
    for(refndx_t ndx = 0; ndx < count; ndx++)
        m_tbl[ndx]->setAsChild(parentTbl, ndx);
}

refndx_t TagTable::lookUp(const string& tag)
{
    lookup_t::iterator it = m_refndxByTag.find(tag);
    if(it == m_refndxByTag.end())
        return NDX_INVALID;
    else
        return (*it).second;
}

void TagTable::hash(const string& tag, refndx_t refndx)
{
    //if(NDX_INVALID != lookUp(tag))
    //    return;

    m_refndxByTag.insert(pair<const string, refndx_t>(tag, refndx));
}

void TagTable::print(ostream& output) const
{
    size_t count = m_tbl.size();
    for(refndx_t ndx = 0; ndx < count; ndx++)
    {
        output << ndx;
        output << ": ";
        m_tbl[ndx]->print(output);
    }
}

//////////////////////////////////////////////

UIDGen* UIDGen::m_uidGen;

UIDGen::UIDGen() : m_nextuid(100)
{
}

UIDGen::~UIDGen()
{
    delete m_uidGen;
}

UIDGen& UIDGen::getUIDGen() 
{
    if(NULL == m_uidGen)
        m_uidGen = new UIDGen();

    return *m_uidGen;
}

taguid_t nextUid()
{
    return UIDGen::getUIDGen().nextUid();
}
