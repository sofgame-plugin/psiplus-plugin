/*
 * statitem.cpp - Sof Game Psi plugin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "statitem.h"
#include "utils.h"

StatItem::StatItem()
{
}

bool StatItem::reset()
{
	if (!empty) {
		empty = true;
		return true;
	}
	return false;
}

//----------------------------- StatIntItem -------------------------

StatIntItem::StatIntItem() :
	StatItem(),
	value_(0)
{
}

QVariant StatIntItem::value() const
{
	if (isEmpty())
		return 0;
	return value_;
}

bool StatIntItem::setValue(QVariant &val)
{
	if (!val.isValid())
		return reset();

	int newVal = val.toInt();
	if (!isEmpty() && newVal == value_)
		return false;

	value_ = newVal;
	empty = false;
	return true;
}

//----------------------------- StatIntNaItem -------------------------

QString StatIntNaItem::toString() const
{
	if (isEmpty())
		return EmptyStringNA::emptyStringValue();
	return numToStr(value_, "'");
}

//----------------------------- StatIntZeroItem -------------------------

QString StatIntZeroItem::toString() const
{
	if (isEmpty())
		return EmptyStringZero::emptyStringValue();
	return numToStr(value_, "'");
}

//----------------------------- StatLongZeroItem -------------------------

StatLongZeroItem::StatLongZeroItem() :
	StatItem(),
	value_(0)
{
}

QVariant StatLongZeroItem::value() const
{
	if (isEmpty())
		return 0;
	return value_;
}

bool StatLongZeroItem::setValue(QVariant &val)
{
	if (!val.isValid())
		return reset();

	long long newVal = val.toInt();
	if (!isEmpty() && newVal == value_)
		return false;

	value_ = newVal;
	empty = false;
	return true;
}

QString StatLongZeroItem::toString() const
{
	if (isEmpty())
		return EmptyStringZero::emptyStringValue();
	return numToStr(value_, "'");
}

//----------------------------- StatStringItem -------------------------

StatStringItem::StatStringItem() :
	StatItem()
{
}

QVariant StatStringItem::value() const
{
	if (isEmpty())
		return QString();
	return value_;
}

bool StatStringItem::setValue(QVariant &val)
{
	if (!val.isValid())
		return reset();

	QString newVal = val.toString();
	if (!isEmpty() && newVal == value_)
		return false;

	value_ = newVal;
	empty = false;
	return true;
}

QString StatStringItem::toString() const
{
	if (isEmpty())
		return EmptyStringNA::emptyStringValue();
	return value_;
}
