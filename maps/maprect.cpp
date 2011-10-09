/*
 * maprect.cpp - Sof Game Psi plugin
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

#include "maprect.h"

/**
 * В игре координата y приростает вверх
 * Инициируются как не валидный
 */
MapRect::MapRect() :
	left_(0),
	right_(-1),
	top_(0),
	bottom_(1)
{
}

MapRect::MapRect(int minX, int maxX, int minY, int maxY) :
	left_(minX),
	right_(maxX),
	top_(maxY),
	bottom_(minY)
{
}

bool MapRect::isValid() const
{
	return (left_ <= right_ && top_ >= bottom_);
}

/**
 * Изменяет координаты квадрата так, чтобы точка входила в него
 * Если квадрат не валиден инициирует его координатами точки
 */
void MapRect::addPoint(const MapPos &point)
{
	const int x = point.x();
	const int y = point.y();
	if (isValid()) {
		if (left_ > x) {
			left_ = x;
		} else if (right_ < x) {
			right_ = x;
		}
		if (top_ < y) {
			top_ = y;
		} else if (bottom_ > y) {
			bottom_ = y;
		}
		return;
	}
	left_ = right_ = x;
	top_ = bottom_ = y;
}

/**
 * Проверяет находится ли точка внутри прямоугольника на или его границах
 * Если это так, то возвращается true иначе false
 * Если прямоугольник не валиден, то возвращается false
 */
bool MapRect::contains(const MapPos &point) const
{
	if (!isValid())
		return false;
	const int x = point.x();
	if (x < left_ || x > right_)
		return false;
	const int y = point.y();
	if (y > top_ || y < bottom_)
		return false;
	return true;
}
