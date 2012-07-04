/*
 * equipitem.h - Sof Game Psi plugin
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

#ifndef EQUIPITEM_H
#define EQUIPITEM_H

#include <QString>

class EquipItem
{
public:
	enum {
		EquipTypeWeapon,
		EquipTypeShield,
		EquipTypeHead,
		EquipTypeNeck,
		EquipTypeHand1,
		EquipTypeHand2,
		EquipTypeShoulders,
		EquipTypeBody,
		EquipTypeStrap,
		EquipTypeFeet,
		EquipTypeShoes
	};
	EquipItem();
	EquipItem(const EquipItem &other);
	EquipItem(const QString &s);
	virtual ~EquipItem() {}
	virtual int equipType() const = 0;
	bool isValid() const;
	const QString &name() const {return _name;}
	void loadFromString(const QString &s);
	void setEquipBonus(const QString &s);
	float damageForLevel(int level) const;
	float protectForLevel(int level) const;
	float forceForLevel(int level) const;
	float dextForLevel(int level) const;
	float intellForLevel(int level) const;
	float efficiency() const;
	int  upLevel() const {return _upLevel;}
	QString toString() const;

private:
	void reset();

private:
	enum {Unknow, Valid, Invalid};
	mutable int status;
	QString _name;
	int     type;
	int     _upLevel;
	int     damage;
	float   damageMul;
	int     damageSet;
	int     protect;
	float   protectMul;
	int     protectSet;
	int     force;
	float   forceMul;
	int     forceSet;
	int     dext;
	float   dextMul;
	int     dextSet;
	int     intell;
	float   intellMul;
	int     intellSet;

	static QRegExp equipElementReg;
	static QRegExp equipParamElementReg;

};

class WeaponEquipItem : public EquipItem
{
public:
	WeaponEquipItem() : EquipItem() {}
	WeaponEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeWeapon;}
};

class ShieldEquipItem : public EquipItem
{
public:
	ShieldEquipItem() : EquipItem() {}
	ShieldEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeShield;}
};

class HeadEquipItem : public EquipItem
{
public:
	HeadEquipItem() : EquipItem() {}
	HeadEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeHead;}
};

class NeckEquipItem : public EquipItem
{
public:
	NeckEquipItem() : EquipItem() {}
	NeckEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeNeck;}
};

class Hand1EquipItem : public EquipItem
{
public:
	Hand1EquipItem() : EquipItem() {}
	Hand1EquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeHand1;}
};

class Hand2EquipItem : public EquipItem
{
public:
	Hand2EquipItem() : EquipItem() {}
	Hand2EquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeHand2;}
};

class ShouldersEquipItem : public EquipItem
{
public:
	ShouldersEquipItem() : EquipItem() {}
	ShouldersEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeShoulders;}
};

class BodyEquipItem : public EquipItem
{
public:
	BodyEquipItem() : EquipItem() {}
	BodyEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeBody;}
};

class StrapEquipItem : public EquipItem
{
public:
	StrapEquipItem() : EquipItem() {}
	StrapEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeStrap;}
};

class FeetEquipItem : public EquipItem
{
public:
	FeetEquipItem() : EquipItem() {}
	FeetEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeFeet;}
};

class ShoesEquipItem : public EquipItem
{
public:
	ShoesEquipItem() : EquipItem() {}
	ShoesEquipItem(const EquipItem &other) : EquipItem(other) {}
	int equipType() const {return EquipTypeShoes;}
};

#endif // EQUIPITEM_H
