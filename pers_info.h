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

struct equip_element {
	int         status;
	int         type;
	QString     name;
	int         up_level;
	int         loss;
	float       loss_mul;
	int         protect;
	float       protect_mul;
	int         force;
	float       force_mul;
	int         dext;
	float       dext_mul;
	int         intell;
	float       intell_mul;
	int         loss_set;
	int         protect_set;
	int         force_set;
	int         dext_set;
	int         intell_set;
};

struct params_info {
	int         force;
	int         dext;
	int         intell;
	int         equip_loss;
	int         equip_protect;
};

class PersInfo: public QObject
{
  Q_OBJECT

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
	void setEquip(int type, const equip_element &ee);
	bool isEquipElement(int) const;
	int  isEquipNamed(int) const;
	void calculateEquipParams(int, struct params_info*) const;
	float calculateEquipEfficiency(int type) const;
	void calculateLoss();
	void calculateProtect();
	static void resetEquip(struct equip_element &equipEl);
	static QString getSharpening(int calcVal, int currVal);
	static bool getEquipFromString(const QString &, struct equip_element*);
	QString getEquipString(int) const;
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
	struct equip_element weapon;
	struct equip_element shield;
	struct equip_element head;
	struct equip_element neck;
	struct equip_element shoulders;
	struct equip_element body;
	struct equip_element hand1;
	struct equip_element hand2;
	struct equip_element strap;
	struct equip_element feet;
	struct equip_element shoes;
	equip_element *getEquipElementPointer(int);
	const equip_element &getEquipElement(int) const;

};

#endif
