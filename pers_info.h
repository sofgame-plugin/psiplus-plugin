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

#define PERS_INFO_EQUIP_WEAPON      1
#define PERS_INFO_EQUIP_SHIELD      2
#define PERS_INFO_EQUIP_HEAD        3
#define PERS_INFO_EQUIP_NECK        4
#define PERS_INFO_EQUIP_HAND1       5
#define PERS_INFO_EQUIP_HAND2       6
#define PERS_INFO_EQUIP_SHOULDERS   7
#define PERS_INFO_EQUIP_BODY        8
#define PERS_INFO_EQUIP_STRAP       9
#define PERS_INFO_EQUIP_FEET        10
#define PERS_INFO_EQUIP_SHOES       11


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
		PersInfo();
		~PersInfo();
		QString getName() const;
		void setName(QString);
		void setLevel(int);
		bool getLevel(int*) const;
		void setSitizenship(QString);
		bool getSitizenship(QString*) const;
		void setClan(QString);
		bool getClan(QString*) const;
		void setRating(int);
		bool getRating(int*) const;
		void setHealthCurr(int);
		void setHealthMax(int);
		bool getHealthMax(int*) const;
		void setEnergyCurr(int);
		void setEnergyMax(int);
		bool getEnergyMax(int*) const;
		void setExperienceCurr(qint64);
		bool getExperienceCurr(qint64*) const;
		void setExperienceRemain(qint64);
		void setForce(int, int);
		bool getForce(int*, int*) const;
		void setDext(int, int);
		bool getDext(int*, int*) const;
		void setIntell(int, int);
		bool getIntell(int*, int*) const;
		void setEquipLoss(int);
		bool getEquipLoss1(int*);
		bool getEquipLoss2(int*);
		void setEquipProtect(int);
		bool getEquipProtect1(int*);
		bool getEquipProtect2(int*);
		void setEquip(int, struct equip_element*);
		bool isEquipElement(int);
		int  isEquipNamed(int);
		void calculateEquipParams(int, struct params_info*);
		float calculateEquipEfficiency(int type);
		void calculateLoss();
		void calculateProtect();
		QString getEquipString(int);

	private:
		QString persName;
		int persLevel;
		int persRating;
		qint64 experienceCurr;
		qint64 experienceRemain;
		QString sitizenship;
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
		int equipLoss;
		int equipLoss1;
		int equipLoss2;
		int equipProtect;
		int equipProtect1;
		int equipProtect2;
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
		struct equip_element* getEquipElementPointer(int);

	protected:

};

void resetEquip(struct equip_element*);
bool getEquipFromString(QString, struct equip_element*);

//extern QStringList fingTypeStrings;

#endif
