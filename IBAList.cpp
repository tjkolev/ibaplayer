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

#include "IBAList.h"

ListItem::ListItem(int id, string& name)
{
	Id = id;
	Name = name;
}

ListItem::ListItem(int id, char* name)
{
	Id = id;
	Name = name;
}

ListItem CascadeList_t::EMPTY(0, "[Empty]");

ListItem& CascadeList_t::AtItem()
{
	if(size() < 1)
		return EMPTY;
	if(CurrentIndex < 0)
		CurrentIndex = 0;
	else if (CurrentIndex >= size())
		CurrentIndex = size() - 1;
	return at(CurrentIndex);
}
