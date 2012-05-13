/*
 * pers_info.cpp - Sof Game Psi plugin
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

#include <QtCore>

#include "pers_info.h"
#include "common.h"
#include "utils.h"


PersInfo::PersInfo()
{
	/**
	* Конструктор
	**/
	// Начальная инициализация
	persName = QString();
	persLevel = -1;
	persRating = 0;
	experienceCurr = -1;
	experienceRemain = -1;
	citizenship = QString();
	clan = QString();
	healthCurr = QINT32_MIN;
	healthMax = QINT32_MIN;
	energyCurr = QINT32_MIN;
	energyMax = QINT32_MIN;
	force1 = QINT32_MIN;
	force2 = QINT32_MIN;
	dext1 = QINT32_MIN;
	dext2 = QINT32_MIN;
	intell1 = QINT32_MIN;
	intell2 = QINT32_MIN;
	equipLoss = QINT32_MIN;
	equipLoss1 = QINT32_MIN;
	equipLoss2 = QINT32_MIN;
	equipProtect = QINT32_MIN;
	equipProtect1 = QINT32_MIN;
	equipProtect2 = QINT32_MIN;
	resetEquip(&weapon);
	resetEquip(&shield);
	resetEquip(&head);
	resetEquip(&neck);
	resetEquip(&shoulders);
	resetEquip(&body);
	resetEquip(&hand1);
	resetEquip(&hand2);
	resetEquip(&strap);
	resetEquip(&hand2);
	resetEquip(&feet);
	resetEquip(&shoes);
}

PersInfo::~PersInfo()
{
	/**
	* Деструктор
	**/

}

QString PersInfo::getName() const
{
	/**
	* Возвращает имя персонажа
	**/
	return persName;
}

void PersInfo::setName(QString name)
{
	/**
	* Устанавливает имя персонажа
	**/
	persName = name;
}

void PersInfo::setLevel(int level)
{
	persLevel = level;
	equipLoss1 = QINT32_MIN;
	equipLoss2 = QINT32_MIN;
	equipProtect1 = QINT32_MIN;
	equipProtect2 = QINT32_MIN;
}

bool PersInfo::getLevel(int* level) const
{
	if (persLevel < 0)
		return false;
	*level = persLevel;
	return true;
}

void PersInfo::setCitizenship(QString citiz)
{
	citizenship = citiz;
}

bool PersInfo::getCitizenship(QString* citiz) const
{
	if (citizenship.isEmpty())
		return false;
	*citiz = citizenship;
	return true;
}

void PersInfo::setClan(QString cl_name)
{
	clan = cl_name;
}

bool PersInfo::getClan(QString* cl_name) const
{
	if (clan.isEmpty())
		return false;
	*cl_name = clan;
	return true;
}

void PersInfo::setRating(int rt)
{
	persRating = rt;
}

bool PersInfo::getRating(int* rt) const
{
	if (persRating <= 0)
		return false;
	*rt = persRating;
	return true;
}

void PersInfo::setHealthCurr(int health)
{
	healthCurr = health;
}

void PersInfo::setHealthMax(int health)
{
	healthMax = health;
}

bool PersInfo::getHealthMax(int* health) const
{
	if (healthMax == QINT32_MIN)
		return false;
	*health = healthMax;
	return true;
}

void PersInfo::setEnergyCurr(int energy)
{
	energyCurr = energy;
}

void PersInfo::setEnergyMax(int energy)
{
	energyMax = energy;
}

bool PersInfo::getEnergyMax(int* energy) const
{
	if (energyMax == QINT32_MIN)
		return false;
	*energy = energyMax;
	return true;
}

void PersInfo::setExperienceCurr(qint64 experience)
{
	experienceCurr = experience;
}

bool PersInfo::getExperienceCurr(qint64* experience) const
{
	if (experienceCurr == -1)
		return false;
	*experience = experienceCurr;
	return true;
}

void PersInfo::setExperienceRemain(qint64 experience)
{
	experienceRemain = experience;
}

void PersInfo::setForce(int f1, int f2)
{
	force1 = f1;
	force2 = f2;
}

bool PersInfo::getForce(int* f1, int* f2) const
{
	if (force1 == QINT32_MIN || force2 == QINT32_MIN)
		return false;
	*f1 = force1;
	*f2 = force2;
	return true;
}

void PersInfo::setDext(int d1, int d2)
{
	dext1 = d1;
	dext2 = d2;
}

bool PersInfo::getDext(int* d1, int* d2) const
{
	if (dext1 == QINT32_MIN || dext2 == QINT32_MIN)
		return false;
	*d1 = dext1;
	*d2 = dext2;
	return true;
}

void PersInfo::setIntell(int i1, int i2)
{
	intell1 = i1;
	intell2 = i2;
}

bool PersInfo::getIntell(int* i1, int* i2) const
{
	if (intell1 == QINT32_MIN || intell2 == QINT32_MIN)
		return false;
	*i1 = intell1;
	*i2 = intell2;
	return true;
}

void PersInfo::setEquipLoss(int loss)
{
	equipLoss = loss;
}

bool PersInfo::getEquipLoss1(int* loss)
{
	if (equipLoss1 == QINT32_MIN) {
		calculateLoss();
	}
	if (equipLoss1 == QINT32_MIN)
		return false;
	*loss = equipLoss1;
	return true;
}

bool PersInfo::getEquipLoss2(int* loss)
{
	if (equipLoss1 == QINT32_MIN) {
		calculateLoss();
	}
	if (equipLoss2 == QINT32_MIN)
		return false;
	*loss = equipLoss2;
	return true;
}

void PersInfo::setEquipProtect(int prot)
{
	equipProtect = prot;
}

bool PersInfo::getEquipProtect1(int* prot)
{
	if (equipProtect1 == QINT32_MIN) {
		calculateProtect();
	}
	if (equipProtect1 == QINT32_MIN)
		return false;
	*prot = equipProtect1;
	return true;
}

bool PersInfo::getEquipProtect2(int* prot)
{
	if (equipProtect1 == QINT32_MIN) {
		calculateProtect();
	}
	if (equipProtect2 == QINT32_MIN)
		return false;
	*prot = equipProtect2;
	return true;
}

void PersInfo::setEquip(int type, struct equip_element* ee)
{
	struct equip_element* eePtr = getEquipElementPointer(type);
	if (!eePtr)
		return;
	if (ee->status == 1) {
		*eePtr = *ee;
	} else {
		eePtr->status = 0;
	}
	equipLoss1 = QINT32_MIN;
	equipLoss2 = QINT32_MIN;
	equipProtect1 = QINT32_MIN;
	equipProtect2 = QINT32_MIN;
}

bool PersInfo::isEquipElement(int type)
{
	/**
	* Проверяет существует ли элемент экипировки
	**/
	struct equip_element* ee = getEquipElementPointer(type);
	if (ee) {
		if (ee->status == 1) {
			return true;
		}
	}
	return false;
}

/**
 * Возвращает уровень апдейта именного
 */
int PersInfo::isEquipNamed(int type)
{
	struct equip_element* ee = getEquipElementPointer(type);
	if (ee) {
		if (ee->status == 1) {
			int level = ee->up_level;
			if (level > 0) {
				return level;
			}
		}
	}
	return 0;
}

void PersInfo::calculateEquipParams(int type, struct params_info* par_info)
{
	/**
	* Расчитывает параметры конкретного элемента экипировки
	**/
	par_info->force = 0;
	par_info->dext = 0;
	par_info->intell = 0;
	par_info->equip_loss = 0;
	par_info->equip_protect = 0;
	// Получаем указатель на структуру экипировки
	struct equip_element* ee = getEquipElementPointer(type);
	// Проверки
	if (!ee) return;
	if (ee->status != 1) return;
	float lvl;
	if (persLevel == -1) {
		lvl = 0.0f;
	} else {
		lvl = (float)persLevel;
	}
	// Считаем
	par_info->force = floor(lvl * ee->force_mul + 0.01f) + (float)ee->force + (float)ee->force_set;
	par_info->dext = floor(lvl * ee->dext_mul + 0.01f) + (float)ee->dext + (float)ee->dext_set;
	par_info->intell = floor(lvl * ee->intell_mul + 0.01f) + (float)ee->intell + (float)ee->intell_set;
	par_info->equip_loss = floor(lvl * ee->loss_mul + 0.01f) + (float)ee->loss + (float)ee->loss_set;
	par_info->equip_protect = floor(lvl * ee->protect_mul + 0.01f) + (float)ee->protect + (float)ee->protect_set;
}

/**
 * Расчитывает эффективность отдельного элемента экипировки
 */
float PersInfo::calculateEquipEfficiency(int type)
{
	float res_val = 0.0f;
	// Получаем указатель на структуру экипировки
	struct equip_element* eep = getEquipElementPointer(type);
	if (eep) {
		int level = eep->up_level;
		if (level >= 7) {
			res_val = 7 - level;
			res_val += eep->loss_mul + eep->protect_mul + eep->force_mul * 10.0f + eep->dext_mul * 10.0f + eep->intell_mul * 10.0f;
			res_val += (float)eep->loss_set / 25.0f + (float)eep->protect_set / 25.0f + (float)eep->force_set / 2.5f + (float)eep->dext_set / 2.5f + (float)eep->intell_set / 2.5f;
		}
	}
	return res_val;
}

void PersInfo::calculateLoss()
{
	/**
	* Расчитывает урон экипировки
	**/
	if (persLevel == -1) {
		equipLoss1 = QINT32_MIN;
		equipLoss2 = QINT32_MIN;
		return;
	}
	float loss = 0.0f;
	float lvl = (float)persLevel;
	if (weapon.status == 1) {
		loss += floor(lvl * weapon.loss_mul + 0.01f);
		loss += weapon.loss + weapon.loss_set;
	}
	if (shield.status == 1) {
		loss += floor(lvl * shield.loss_mul + 0.01f);
		loss += shield.loss + shield.loss_set;
	}
	if (head.status == 1) {
		loss += floor(lvl * head.loss_mul + 0.01f);
		loss += head.loss + head.loss_set;
	}
	if (neck.status == 1) {
		loss += floor(lvl * neck.loss_mul + 0.01f);
		loss += neck.loss + neck.loss_set;
	}
	if (shoulders.status == 1) {
		loss += floor(lvl * shoulders.loss_mul + 0.01f);
		loss += shoulders.loss + shoulders.loss_set;
	}
	if (body.status == 1) {
		loss += floor(lvl * body.loss_mul + 0.01f);
		loss += body.loss + body.loss_set;
	}
	if (hand1.status == 1) {
		loss += floor(lvl * hand1.loss_mul + 0.01f);
		loss += hand1.loss + hand1.loss_set;
	}
	if (hand2.status == 1) {
		loss += floor(lvl * hand2.loss_mul + 0.01f);
		loss += hand2.loss + hand2.loss_set;
	}
	if (strap.status == 1) {
		loss += floor(lvl * strap.loss_mul + 0.01f);
		loss += strap.loss + strap.loss_set;
	}
	if (feet.status == 1) {
		loss += floor(lvl * feet.loss_mul + 0.01f);
		loss += feet.loss + feet.loss_set;
	}
	if (shoes.status == 1) {
		loss += floor(lvl * shoes.loss_mul + 0.01f);
		loss += shoes.loss + shoes.loss_set;
	}
	equipLoss1 = (int)loss;
	equipLoss2 = (float)equipLoss1 * 1.3f;
}

void PersInfo::calculateProtect()
{
	/**
	* Расчитывает защиту экипировки
	**/
	if (persLevel == -1) {
		equipProtect1 = QINT32_MIN;
		equipProtect2 = QINT32_MIN;
		return;
	}
	float protect = 0.0f;
	float lvl = (float)persLevel;
	if (weapon.status == 1) {
		protect += floor(lvl * weapon.protect_mul + 0.01f);
		protect += weapon.protect + weapon.protect_set;
	}
	if (shield.status == 1) {
		protect += floor(lvl * shield.protect_mul + 0.01f);
		protect += shield.protect + shield.protect_set;
	}
	if (head.status == 1) {
		protect += floor(lvl * head.protect_mul + 0.01f);
		protect += head.protect + head.protect_set;
	}
	if (neck.status == 1) {
		protect += floor(lvl * neck.protect_mul + 0.01f);
		protect += neck.protect + neck.protect_set;
	}
	if (shoulders.status == 1) {
		protect += floor(lvl * shoulders.protect_mul + 0.01f);
		protect += shoulders.protect + shoulders.protect_set;
	}
	if (body.status == 1) {
		protect += floor(lvl * body.protect_mul + 0.01f);
		protect += body.protect + body.protect_set;
	}
	if (hand1.status == 1) {
		protect += floor(lvl * hand1.protect_mul + 0.01f);
		protect += hand1.protect + hand1.protect_set;
	}
	if (hand2.status == 1) {
		protect += floor(lvl * hand2.protect_mul + 0.01f);
		protect += hand2.protect + hand2.protect_set;
	}
	if (strap.status == 1) {
		protect += floor(lvl * strap.protect_mul + 0.01f);
		protect += strap.protect + strap.protect_set;
	}
	if (feet.status == 1) {
		protect += floor(lvl * feet.protect_mul + 0.01f);
		protect += feet.protect + feet.protect_set;
	}
	if (shoes.status == 1) {
		protect += floor(lvl * shoes.protect_mul + 0.01f);
		protect += shoes.protect + shoes.protect_set;
	}
	equipProtect1 = (int)protect;
	equipProtect2 = (float)equipProtect1 * 1.3f;
}

QString PersInfo::getEquipString(int type)
{
	/**
	* Формирует строку элемента экипировки
	**/
	struct equip_element* ee = getEquipElementPointer(type);
	QString sRes = "";
	if (ee) {
		if (ee->status == 1) {
			sRes.append(ee->name + QString::fromUtf8(" урон:"));
			float mul = ee->loss_mul;
			int abs = ee->loss;
			if (mul != 0.0 || abs != 0) {
				if (mul != 0.0) {
					sRes.append(QString::number(mul) + QString::fromUtf8("*ур"));
					if (abs != 0) {
						sRes.append("+");
					}
				}
				if (abs != 0) {
					sRes.append(QString::number(abs));
				}
			} else {
				sRes.append("0");
			}
			sRes.append(QString::fromUtf8("; защ:"));
			mul = ee->protect_mul;
			abs = ee->protect;
			if (mul != 0.0 || abs != 0) {
				if (mul != 0.0) {
					sRes.append(QString::number(mul) + QString::fromUtf8("*ур"));
					if (abs != 0) {
						sRes.append("+");
					}
				}
				if (abs != 0) {
					sRes.append(QString::number(abs));
				}
			} else {
				sRes.append("0");
			}
			sRes.append(QString::fromUtf8("; сила:"));
			mul = ee->force_mul;
			abs = ee->force;
			if (mul != 0.0 || abs != 0) {
				if (mul != 0.0) {
					sRes.append(QString::number(mul) + QString::fromUtf8("*ур"));
					if (abs != 0) {
						sRes.append("+");
					}
				}
				if (abs != 0) {
					sRes.append(QString::number(abs));
				}
			} else {
				sRes.append("0");
			}
			sRes.append(QString::fromUtf8("; ловк:"));
			mul = ee->dext_mul;
			abs = ee->dext;
			if (mul != 0.0 || abs != 0) {
				if (mul != 0.0) {
					sRes.append(QString::number(mul) + QString::fromUtf8("*ур"));
					if (abs != 0) {
						sRes.append("+");
					}
				}
				if (abs != 0) {
					sRes.append(QString::number(abs));
				}
			} else {
				sRes.append("0");
			}
			sRes.append(QString::fromUtf8("; инт:"));
			mul = ee->intell_mul;
			abs = ee->intell;
			if (mul != 0.0 || abs != 0) {
				if (mul != 0) {
					sRes.append(QString::number(mul) + QString::fromUtf8("*ур"));
					if (abs != 0) {
						sRes.append("+");
					}
				}
				if (abs != 0) {
					sRes.append(QString::number(abs));
				}
			} else {
				sRes.append("0");
			}
		}
	}
	return sRes;
}

struct equip_element* PersInfo::getEquipElementPointer(int type)
{
	/**
	* Возвращает указатель на структуру элемента экипировки с указанным типом
	**/
	if (type == PERS_INFO_EQUIP_WEAPON) {
		return &weapon;
	}
	if (type == PERS_INFO_EQUIP_SHIELD) {
		return &shield;
	}
	if (type == PERS_INFO_EQUIP_HEAD) {
		return &head;
	}
	if (type == PERS_INFO_EQUIP_NECK) {
		return &neck;
	}
	if (type == PERS_INFO_EQUIP_HAND1) {
		return &hand1;
	}
	if (type == PERS_INFO_EQUIP_HAND2) {
		return &hand2;
	}
	if (type == PERS_INFO_EQUIP_SHOULDERS) {
		return &shoulders;
	}
	if (type == PERS_INFO_EQUIP_BODY) {
		return &body;
	}
	if (type == PERS_INFO_EQUIP_STRAP) {
		return &strap;
	}
	if (type == PERS_INFO_EQUIP_FEET) {
		return &feet;
	}
	if (type == PERS_INFO_EQUIP_SHOES) {
		return &shoes;
	}
	return 0;
}

void resetEquip(struct equip_element* equipEl)
{
	/**
	* Инициирует структуру экипировки начальными значениями
	**/
	equipEl->status = 0;
	equipEl->type = -1;
	equipEl->name = "";
	equipEl->up_level = 0;
	equipEl->loss = 0;
	equipEl->loss_mul = 0.0f;
	equipEl->protect = 0;
	equipEl->protect_mul = 0.0f;
	equipEl->force = 0;
	equipEl->force_mul = 0.0f;
	equipEl->dext = 0;
	equipEl->dext_mul = 0.0f;
	equipEl->intell = 0;
	equipEl->intell_mul = 0.0f;
	equipEl->loss_set = 0;
	equipEl->protect_set = 0;
	equipEl->force_set = 0;
	equipEl->dext_set = 0;
	equipEl->intell_set = 0;
}

QRegExp equipElementReg(QString::fromUtf8("^(.+)\\((\\w+)\\)(.+)\\{(.+)\\}(И:([0-9]+)ур\\.)?$"));
QRegExp equipParamElementReg(QString::fromUtf8("^(\\w+):([0-9.]+)(\\*\\w+)?$"));

bool getEquipFromString(const QString &thingStr, struct equip_element* equipElementPtr)
{
	/**
	* Разбирает строку экипировки в структуру
	**/
	// Обруч ярости Демона(амулет)сила:1.8*ур;ловк:4.2*ур;инт:1.8*ур;урон:10.5*ур;защ:6.2*ур;{Треб:Ур13Сил39}И:8ур.
	if (equipElementReg.indexIn(thingStr, 0) == -1)
		return false;
	struct equip_element equipEl;
	resetEquip(&equipEl);
	equipEl.type = thingTypeFromString(equipElementReg.cap(2));
	equipEl.name = equipElementReg.cap(1).trimmed();
	if (!equipElementReg.cap(6).isEmpty()) {
		equipEl.up_level = equipElementReg.cap(6).toInt();
	} else {
		equipEl.up_level = 0;
	}
	QStringList aParams = equipElementReg.cap(3).trimmed().split(";", QString::SkipEmptyParts);
	int cnt = aParams.size();
	for (int i = 0; i < cnt; i++) {
		if (equipParamElementReg.indexIn(aParams[i], 0) != -1) {
			QString sParam = equipParamElementReg.cap(1);
			if (sParam == QString::fromUtf8("урон")) {
				if (equipParamElementReg.cap(3).isEmpty()) {
					equipEl.loss += equipParamElementReg.cap(2).toInt();
				} else {
					equipEl.loss_mul += equipParamElementReg.cap(2).toFloat();
				}
			} else if (sParam == QString::fromUtf8("защ")) {
				if (equipParamElementReg.cap(3).isEmpty()) {
					equipEl.protect += equipParamElementReg.cap(2).toInt();
				} else {
					equipEl.protect_mul += equipParamElementReg.cap(2).toFloat();
				}
			} else if (sParam == QString::fromUtf8("сила")) {
				if (equipParamElementReg.cap(3).isEmpty()) {
					equipEl.force += equipParamElementReg.cap(2).toInt();
				} else {
					equipEl.force_mul += equipParamElementReg.cap(2).toFloat();
				}
			} else if (sParam == QString::fromUtf8("ловк")) {
				if (equipParamElementReg.cap(3).isEmpty()) {
					equipEl.dext += equipParamElementReg.cap(2).toInt();
				} else {
					equipEl.dext_mul += equipParamElementReg.cap(2).toFloat();
				}
			} else if (sParam == QString::fromUtf8("инт")) {
				if (equipParamElementReg.cap(3).isEmpty()) {
					equipEl.intell += equipParamElementReg.cap(2).toInt();
				} else {
					equipEl.intell_mul += equipParamElementReg.cap(2).toFloat();
				}
			}
		}
	}
	*equipElementPtr = equipEl;
	equipElementPtr->status = 1;
	return true;
}
