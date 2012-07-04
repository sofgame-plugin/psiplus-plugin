/*
 * pers_info.h - Sof Game Psi plugin
 * Copyright (C) 2010  Aleksey Andreev
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

#ifndef PERS_INFO_H
#define PERS_INFO_H

#include <QtCore>

#include "equipitem.h"

class PersInfo: public QObject
{
  Q_OBJECT

public:
	PersInfo();
	~PersInfo();
	QString getName() const {return persName;}
	void setName(const QString &name) {persName = name;}
	void setLevel(int);
	void setCitizenship(const QString &);
	const QString &citizenship() const {return _citizenship;}
	void setClan(const QString &cl_name) {clan = cl_name;}
	void setRating(int rt) {persRating = rt;}
	void setHealthCurr(int health) {healthCurr = health;}
	void setHealthMax(int health) {healthMax = health;}
	void setEnergyCurr(int energy) {energyCurr = energy;}
	void setEnergyMax(int energy) {energyMax = energy;}
	void setExperienceCurr(qint64 experience) {experienceCurr = experience;}
	void setExperienceRemain(qint64 experience) {experienceRemain = experience;}
	void setForce(int, int);
	bool getForce(int*, int*) const;
	void setDext(int, int);
	bool getDext(int*, int*) const;
	void setIntell(int, int);
	bool getIntell(int*, int*) const;
	void setEquipLossCurr(int loss) {equipLossCurr = loss;}
	int  getEquipLossCalc();
	void setEquipProtectCurr(int prot) {equipProtectCurr = prot;}
	int  getEquipProtectCalc();
	//WeaponEquipItem &weapon() {return _weapon;}
	void setEquip(const EquipItem &eItem);
	void calculateLoss();
	void calculateProtect();
	static QString getSharpening(int calcVal, int currVal);
	QString toString(int ver);

private:
	QString persName;
	int persLevel;
	int persRating;
	qint64 experienceCurr;
	qint64 experienceRemain;
	QString _citizenship;
	QString clan;
	int healthCurr;
	int healthMax;
	int energyCurr;
	int energyMax;
	int force1;
	int force2;
	int dext1;
	int dext2;
	int intell1;
	int intell2;
	int equipLossCalc;
	int equipLossCurr;
	int equipProtectCalc;
	int equipProtectCurr;
	WeaponEquipItem    _weapon;
	ShieldEquipItem    _shield;
	HeadEquipItem      _head;
	NeckEquipItem      _neck;
	ShouldersEquipItem _shoulders;
	BodyEquipItem      _body;
	Hand1EquipItem     _hand1;
	Hand2EquipItem     _hand2;
	StrapEquipItem     _strap;
	FeetEquipItem      _feet;
	ShoesEquipItem     _shoes;

};

#endif
