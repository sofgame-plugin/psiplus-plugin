/*
 * mappos.h - Sof Game Psi plugin
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

#ifndef MAPPOS_H
#define MAPPOS_H

#include <QPoint>

class MapPos
{
public:
	MapPos();
	MapPos(int posX, int posY);
	MapPos(const MapPos &pos);
	void reset() {valid = false;};
	bool isValid() const {return valid;};
	int  x() const {return x_;};
	int  y() const {return y_;};
	void setPos(int posX, int posY);
	bool operator == (const MapPos &pos) const;
	bool operator != (const MapPos &pos) const;

private:
	int x_;
	int y_;
	bool valid;

};

#endif // MAPPOS_H
