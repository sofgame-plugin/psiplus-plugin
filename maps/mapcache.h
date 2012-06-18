/*
 * mapcache.h - Sof Game Psi plugin
 * Copyright (C) 2012  Aleksey Andreev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef MAPCACHE_H
#define MAPCACHE_H

#include <QList>

#include "mappos.h"

class MapCache
{
public:
	MapCache();
	int  index(const MapPos &pos);
	void setIndex(const MapPos &pos, int index);
	int  persPosIndex() const {return persIndex;}
	void setPersPosIndex(int i) {persIndex = i;};
	void reset();

private:
	struct CacheItem
	{
		MapPos pos;
		int    index;
		CacheItem(const MapPos &p, int i) : pos(p), index(i) {}
	};
	QList<CacheItem> itemsList;
	int persIndex;
};

#endif // MAPCACHE_H
