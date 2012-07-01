/*
 * statitem.h - Sof Game Psi plugin
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

#ifndef STATITEM_H
#define STATITEM_H

#include <QVariant>

class EmptyStringNA
{
public:
	static QString emptyStringValue() {return "n/a";}
};

class EmptyStringZero
{
public:
	static QString emptyStringValue() {return "0";}
};

class StatItem
{
public:
	StatItem();
	bool isEmpty() const {return empty;}
	bool reset();

	virtual ~StatItem();
	virtual QVariant value() const = 0;
	virtual bool setValue(QVariant &val) = 0;
	virtual QString toString() const = 0;

protected:
	bool empty;

};

class StatIntItem: public StatItem
{
public:
	StatIntItem();

	QVariant value() const;
	bool setValue(QVariant &val);

protected:
	int value_;
};

class StatIntNaItem: public StatIntItem
{
public:
	QString toString() const;
};

class StatIntZeroItem: public StatIntItem
{
public:
	QString toString() const;
};

class StatLongZeroItem: public StatItem
{
public:
	StatLongZeroItem();

	QVariant value() const;
	bool setValue(QVariant &val);
	QString toString() const;

private:
	long long value_;
};

class StatStringItem: public StatItem
{
public:
	StatStringItem();

	QVariant value() const;
	bool setValue(QVariant &val);
	QString toString() const;

private:
	QString value_;
};

#endif // STATITEM_H
