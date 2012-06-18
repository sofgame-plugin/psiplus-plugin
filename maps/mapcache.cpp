/*
 * mapcache.cpp - Sof Game Psi plugin
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

#include "mapcache.h"

#define MAP_CACHE_SIZE 2

MapCache::MapCache() :
	persIndex(-1)
{
}

/**
 * @brief Возвращает сохраненный индекс массима для позиции карты или -1
 * @param pos - позиция карты
 * @return - индекс массива или -1 в случае если такого индекса нет в кэше
 */
int MapCache::index(const MapPos &pos)
{
	for (int i = 0, cnt = itemsList.size(); i < cnt; ++i)
	{
		if (itemsList.at(i).pos == pos)
		{
			if (i != 0)
				itemsList.move(i, 0);
			return itemsList.at(0).index;
		}
	}
	return -1;
}

/**
 * @brief Сохраняет индекс массива в кэше
 * @param pos - позиция на карте
 * @param index - индекс в массиве карты
 */
void MapCache::setIndex(const MapPos &pos, int index)
{
	itemsList.prepend(CacheItem(pos, index));
	if (itemsList.size() > MAP_CACHE_SIZE)
		itemsList.removeLast();
}

/**
 * @brief Очищает кэш индексов
 */
void MapCache::reset()
{
	itemsList.clear();
	persIndex = -1;
}
