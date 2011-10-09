/*
 * mappos.cpp - Sof Game Psi plugin
 * Copyright (C) 2011  Aleksey Andreev
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "mappos.h"

MapPos::MapPos() :
	x_(0), y_(0),
	valid(false)
{
}

MapPos::MapPos(int posX, int posY)
{
	setPos(posX, posY);
}

MapPos::MapPos(const MapPos &pos)
{
	valid = pos.valid;
	if (valid) {
		x_ = pos.x();
		y_ = pos.y();
	}
}

void MapPos::setPos(int posX, int posY)
{
	x_ = posX;
	y_ = posY;
	valid = true;
}

bool MapPos::operator == (const MapPos &pos) const
{
	return (valid == pos.valid && x_ == pos.x_ && y_ == pos.y_);
}

bool MapPos::operator != (const MapPos &pos) const
{
	return (valid != pos.valid || x_ != pos.x_ || y_ != pos.y_);
}

