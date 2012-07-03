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


PersInfo::PersInfo() :
	persLevel(-1),
	persRating(0),
	experienceCurr(-1), experienceRemain(-1),
	healthCurr(QINT32_MIN), healthMax(QINT32_MIN),
	energyCurr(QINT32_MIN), energyMax(QINT32_MIN),
	force1(QINT32_MIN), force2(QINT32_MIN),
	dext1(QINT32_MIN), dext2(QINT32_MIN),
	intell1(QINT32_MIN),  intell2(QINT32_MIN),
	equipLossCalc(QINT32_MIN), equipLossCurr(QINT32_MIN),
	equipProtectCalc(QINT32_MIN), equipProtectCurr(QINT32_MIN)
{
	// Начальная инициализация
	resetEquip(weapon);
	resetEquip(shield);
	resetEquip(head);
	resetEquip(neck);
	resetEquip(shoulders);
	resetEquip(body);
	resetEquip(hand1);
	resetEquip(hand2);
	resetEquip(strap);
	resetEquip(feet);
	resetEquip(shoes);
}

PersInfo::~PersInfo()
{
}

void PersInfo::setLevel(int level)
{
	if (persLevel != level)
	{
		persLevel = level;
		equipLossCalc = QINT32_MIN;
		equipProtectCalc = QINT32_MIN;
	}
}

void PersInfo::setCitizenship(const QString &citiz)
{
	if (citiz.toLower() == QString::fromUtf8("нет"))
		_citizenship = QString();
	else
		_citizenship = citiz;
}

void PersInfo::setForce(int f1, int f2)
{
	force1 = f1;
	force2 = f2;
}

void PersInfo::setDext(int d1, int d2)
{
	dext1 = d1;
	dext2 = d2;
}

void PersInfo::setIntell(int i1, int i2)
{
	intell1 = i1;
	intell2 = i2;
}

int PersInfo::getEquipLossCalc()
{
	if (equipLossCalc == QINT32_MIN) {
		calculateLoss();
	}
	return equipLossCalc;
}

int PersInfo::getEquipProtectCalc()
{
	if (equipProtectCalc == QINT32_MIN) {
		calculateProtect();
	}
	return equipProtectCalc;
}

void PersInfo::setEquip(int type, const equip_element &ee)
{
	struct equip_element* eePtr = getEquipElementPointer(type);
	if (!eePtr)
		return;
	if (ee.status == 1) {
		*eePtr = ee;
	} else {
		eePtr->status = 0;
	}
	equipLossCalc = QINT32_MIN;
	equipProtectCalc = QINT32_MIN;
}

/**
 * Проверяет существует ли элемент экипировки
 */
bool PersInfo::isEquipElement(int type) const
{
	return (getEquipElement(type).status == 1);
}

/**
 * Возвращает уровень апдейта именного
 */
int PersInfo::isEquipNamed(int type) const
{
	const equip_element &ee = getEquipElement(type);
	if (ee.status == 1)
	{
		int level = ee.up_level;
		if (level > 0)
			return level;
	}
	return 0;
}

void PersInfo::calculateEquipParams(int type, struct params_info* par_info) const
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
	const struct equip_element &ee = getEquipElement(type);
	// Проверки
	if (ee.status != 1)
		return;
	float lvl;
	if (persLevel == -1)
	{
		lvl = 0.0f;
	}
	else
	{
		lvl = (float)persLevel;
	}
	// Считаем
	par_info->force = floor(lvl * ee.force_mul + 0.01f) + (float)ee.force + (float)ee.force_set;
	par_info->dext = floor(lvl * ee.dext_mul + 0.01f) + (float)ee.dext + (float)ee.dext_set;
	par_info->intell = floor(lvl * ee.intell_mul + 0.01f) + (float)ee.intell + (float)ee.intell_set;
	par_info->equip_loss = floor(lvl * ee.loss_mul + 0.01f) + (float)ee.loss + (float)ee.loss_set;
	par_info->equip_protect = floor(lvl * ee.protect_mul + 0.01f) + (float)ee.protect + (float)ee.protect_set;
}

/**
 * Расчитывает эффективность отдельного элемента экипировки
 */
float PersInfo::calculateEquipEfficiency(int type) const
{
	float res_val = 0.0f;
	// Получаем указатель на структуру экипировки
	const equip_element &ee = getEquipElement(type);
	int level = ee.up_level;
	if (level >= 7) {
		res_val = 7 - level;
		res_val += ee.loss_mul + ee.protect_mul + ee.force_mul * 10.0f + ee.dext_mul * 10.0f + ee.intell_mul * 10.0f;
		res_val += (float)ee.loss_set / 25.0f + (float)ee.protect_set / 25.0f + (float)ee.force_set / 2.5f + (float)ee.dext_set / 2.5f + (float)ee.intell_set / 2.5f;
	}
	return res_val;
}

void PersInfo::calculateLoss()
{
	/**
	* Расчитывает урон экипировки
	**/
	if (persLevel == -1) {
		equipLossCalc = QINT32_MIN;
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
	equipLossCalc = (int)loss;
}

void PersInfo::calculateProtect()
{
	/**
	* Расчитывает защиту экипировки
	**/
	if (persLevel == -1) {
		equipProtectCalc = QINT32_MIN;
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
	equipProtectCalc = (int)protect;
}

QString PersInfo::getEquipString(int type) const
{
	/**
	* Формирует строку элемента экипировки
	**/
	const equip_element &ee = getEquipElement(type);
	QString sRes = "";
	if (ee.status == 1) {
		sRes.append(ee.name + QString::fromUtf8(" урон:"));
		float mul = ee.loss_mul;
		int abs = ee.loss;
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
		mul = ee.protect_mul;
		abs = ee.protect;
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
		mul = ee.force_mul;
		abs = ee.force;
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
		mul = ee.dext_mul;
		abs = ee.dext;
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
		mul = ee.intell_mul;
		abs = ee.intell;
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
	return sRes;
}

struct equip_element *PersInfo::getEquipElementPointer(int type)
{
	/**
	* Возвращает указатель на структуру элемента экипировки с указанным типом
	**/
	if (type == EquipTypeWeapon)
		return &weapon;
	if (type == EquipTypeShield)
		return &shield;
	if (type == EquipTypeHead)
		return &head;
	if (type == EquipTypeNeck)
		return &neck;
	if (type == EquipTypeHand1)
		return &hand1;
	if (type == EquipTypeHand2)
		return &hand2;
	if (type == EquipTypeShoulders)
		return &shoulders;
	if (type == EquipTypeBody)
		return &body;
	if (type == EquipTypeStrap)
		return &strap;
	if (type == EquipTypeFeet)
		return &feet;
	if (type == EquipTypeShoes)
		return &shoes;
	return 0;
}

const equip_element &PersInfo::getEquipElement(int type) const
{
	if (type == EquipTypeWeapon)
		return weapon;
	if (type == EquipTypeShield)
		return shield;
	if (type == EquipTypeHead)
		return head;
	if (type == EquipTypeNeck)
		return neck;
	if (type == EquipTypeHand1)
		return hand1;
	if (type == EquipTypeHand2)
		return hand2;
	if (type == EquipTypeShoulders)
		return shoulders;
	if (type == EquipTypeBody)
		return body;
	if (type == EquipTypeStrap)
		return strap;
	if (type == EquipTypeFeet)
		return feet;
	//if (type == EquipTypeShoes)
		return shoes;
}

void PersInfo::resetEquip(struct equip_element &equipEl)
{
	/**
	* Инициирует структуру экипировки начальными значениями
	**/
	equipEl.status = 0;
	equipEl.type = -1;
	equipEl.name = "";
	equipEl.up_level = 0;
	equipEl.loss = 0;
	equipEl.loss_mul = 0.0f;
	equipEl.protect = 0;
	equipEl.protect_mul = 0.0f;
	equipEl.force = 0;
	equipEl.force_mul = 0.0f;
	equipEl.dext = 0;
	equipEl.dext_mul = 0.0f;
	equipEl.intell = 0;
	equipEl.intell_mul = 0.0f;
	equipEl.loss_set = 0;
	equipEl.protect_set = 0;
	equipEl.force_set = 0;
	equipEl.dext_set = 0;
	equipEl.intell_set = 0;
}

QRegExp equipElementReg(QString::fromUtf8("^(.+)\\((\\w+)\\)(.+)\\{(.+)\\}(И:([0-9]+)ур\\.)?$"));
QRegExp equipParamElementReg(QString::fromUtf8("^(\\w+):([0-9.]+)(\\*\\w+)?$"));

bool PersInfo::getEquipFromString(const QString &thingStr, struct equip_element* equipElementPtr)
{
	/**
	* Разбирает строку экипировки в структуру
	**/
	// Обруч ярости Демона(амулет)сила:1.8*ур;ловк:4.2*ур;инт:1.8*ур;урон:10.5*ур;защ:6.2*ур;{Треб:Ур13Сил39}И:8ур.
	if (equipElementReg.indexIn(thingStr, 0) == -1)
		return false;
	struct equip_element equipEl;
	resetEquip(equipEl);
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

QString PersInfo::getSharpening(int calcVal, int currVal)
{
	QString res;
	if (calcVal != QINT32_MIN && currVal != QINT32_MIN)
	{
		float nSharpening = (float)currVal / (float)calcVal;
		if (nSharpening > 1.01f)
		{
			res = QString::fromUtf8("[заточка %1%]").arg(floor((nSharpening - 1.0f + 0.01f) * 100));
		}
		else {
			res = QString::fromUtf8("[без заточки]");
		}
	}
	return res;
}

QString PersInfo::toString(int ver)
{
	QString resStr = QString::fromUtf8("\nИмя персонажа: ") + persName;
	resStr.append(QString::fromUtf8("\nУровень: %1, Здоровье: %2, Энергия: %3, Опыт: %4")
		.arg((persLevel < 0) ? "n/a" : QString::number(persLevel))
		.arg((healthMax == QINT32_MIN) ? "n/a" : QString::number(healthMax))
		.arg((energyMax == QINT32_MIN) ? "n/a" : QString::number(energyMax))
		.arg((experienceCurr == -1) ? "n/a" : numToStr(experienceCurr, "'")));
	resStr.append(QString::fromUtf8("\nГражданство: %1")
		.arg((_citizenship.isEmpty()) ? QString::fromUtf8("нет") : _citizenship));
	resStr.append(QString::fromUtf8("\nКлан: %1")
		.arg((clan.isEmpty()) ? QString::fromUtf8("нет") : clan));
	resStr.append(QString::fromUtf8("\nРейтинг: %1")
		.arg((persRating > 0)  ? QString::number(persRating) : "n/a"));

	int num1, num2;
	int nForce = 0;
	int nDext = 0;
	int nIntell = 0;
	resStr.append(QString::fromUtf8("\nСила: "));
	if (getForce(&num1, &num2))
	{
		resStr.append(QString("%1[%2]").arg(num1).arg(num2));
		nForce = num1;
	}
	else {
		resStr.append("n/a");
	}
	resStr.append(QString::fromUtf8("\nЛовкость: "));
	if (getDext(&num1, &num2))
	{
		resStr.append(QString("%1[%2]").arg(num1).arg(num2));
		nDext = num1;
	}
	else {
		resStr.append("n/a");
	}
	resStr.append(QString::fromUtf8("\nИнтеллект: "));
	if (getIntell(&num1, &num2))
	{
		resStr.append(QString("%1[%2]").arg(num1).arg(num2));
		nIntell = num1;
	}
	else {
		resStr.append("n/a");
	}
	// Экипировка
	resStr.append(QString::fromUtf8("\nСуммарный урон экипировки: "));
	if (equipLossCalc == QINT32_MIN)
		calculateLoss();
	if (equipLossCalc != QINT32_MIN)
	{
		resStr.append(QString("%1 / %2").arg(equipLossCalc).arg((equipLossCurr == QINT32_MIN) ? "?" : QString::number(equipLossCurr)));
		QString shpText = getSharpening(equipLossCalc, equipLossCurr);
		if (!shpText.isEmpty())
			resStr.append(" " + shpText);
	}
	else {
		resStr.append("n/a");
	}
	resStr.append(QString::fromUtf8("\nСуммарная защита экипировки: "));
	if (equipProtectCalc == QINT32_MIN)
		calculateProtect();
	if (equipProtectCalc != QINT32_MIN)
	{
		resStr.append(QString("%1 / %2").arg(equipProtectCalc).arg((equipProtectCurr == QINT32_MIN) ? "?" : QString::number(equipProtectCurr)));
		QString shpText = getSharpening(equipProtectCalc, equipProtectCurr);
		if (!shpText.isEmpty())
			resStr.append(" " + shpText);
	}
	else {
		resStr.append("n/a");
	}
	if (ver == 2)
		resStr.append(QString::fromUtf8("\n--- Экипировка ---"));
	int equipCount = 0;
	int namedCount = 0;
	int namedLevelAll = 0;
	int namedEffectCount = 0;
	float namedEffectAll = 0.0f;

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nОружие: "));
	if (isEquipElement(EquipTypeWeapon))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeWeapon);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeWeapon) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeWeapon);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nЩит: "));
	if (isEquipElement(EquipTypeShield))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeShield);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeShield) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeShield);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nГолова: "));
	if (isEquipElement(EquipTypeHead))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeHead);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeHead) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeHead);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nШея: "));
	if (isEquipElement(EquipTypeNeck))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeNeck);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeNeck) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeNeck);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nПлечи: "));
	if (isEquipElement(EquipTypeShoulders))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeShoulders);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeShoulders) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeShoulders);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nРука 1: "));
	if (isEquipElement(EquipTypeHand1))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeHand1);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeHand1) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeHand1);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nРука 2: "));
	if (isEquipElement(EquipTypeHand2))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeHand2);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeHand2) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeHand2);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nКорпус: "));
	if (isEquipElement(EquipTypeBody))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeBody);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeBody) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeBody);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nПояс: "));
	if (isEquipElement(EquipTypeStrap))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeStrap);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeStrap) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeStrap);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nНоги: "));
	if (isEquipElement(EquipTypeFeet))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeFeet);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeFeet) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeFeet);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	if (ver == 2)
		resStr.append(QString::fromUtf8("\nОбувь: "));
	if (isEquipElement(EquipTypeShoes))
	{
		float namedEffect = calculateEquipEfficiency(EquipTypeShoes);
		if (ver == 2)
			resStr.append(getEquipString(EquipTypeShoes) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = isEquipNamed(EquipTypeShoes);
		if (level > 0)
		{
			namedCount++;
			namedLevelAll += level;
			if (level >= 7)
			{
				namedEffectCount++;
				namedEffectAll += namedEffect;
			}
		}
	}
	else {
		if (ver == 2)
			resStr.append(QString::fromUtf8("нет"));
	}

	resStr.append(QString::fromUtf8("\nВещей одето: %1").arg(equipCount));
	if (equipCount > 0)
		resStr.append(QString::fromUtf8(", из них именных: %1").arg(namedCount));
	resStr.append(QString::fromUtf8("\nОбщий уровень именных: %1").arg(namedLevelAll));
	resStr.append(QString::fromUtf8("\nОбщая эффективность именных: %1 / %2").arg(numToStr(floor(namedEffectAll + 0.51f), "'")).arg(namedEffectCount));

	if (ver == 2)
	{
		resStr.append(QString::fromUtf8("\n--- Долевой вклад экипировки ---"));
		int force_sum = nForce;
		int dext_sum = nDext;
		int intell_sum = nIntell;
		int equip_loss_sum = 0;
		int equip_protect_sum = 0;
		// Оружие
		struct params_info weaponParams;
		calculateEquipParams(EquipTypeWeapon, &weaponParams);
		force_sum += weaponParams.force;
		dext_sum += weaponParams.dext;
		intell_sum += weaponParams.intell;
		equip_loss_sum += weaponParams.equip_loss;
		equip_protect_sum += weaponParams.equip_protect;
		// Щит
		struct params_info shieldParams;
		calculateEquipParams(EquipTypeShield, &shieldParams);
		force_sum += shieldParams.force;
		dext_sum += shieldParams.dext;
		intell_sum += shieldParams.intell;
		equip_loss_sum += shieldParams.equip_loss;
		equip_protect_sum += shieldParams.equip_protect;
		// Голова
		struct params_info headParams;
		calculateEquipParams(EquipTypeHead, &headParams);
		force_sum += headParams.force;
		dext_sum += headParams.dext;
		intell_sum += headParams.intell;
		equip_loss_sum += headParams.equip_loss;
		equip_protect_sum += headParams.equip_protect;
		// Шея
		struct params_info neckParams;
		calculateEquipParams(EquipTypeNeck, &neckParams);
		force_sum += neckParams.force;
		dext_sum += neckParams.dext;
		intell_sum += neckParams.intell;
		equip_loss_sum += neckParams.equip_loss;
		equip_protect_sum += neckParams.equip_protect;
		// Плечи
		struct params_info shouldersParams;
		calculateEquipParams(EquipTypeShoulders, &shouldersParams);
		force_sum += shouldersParams.force;
		dext_sum += shouldersParams.dext;
		intell_sum += shouldersParams.intell;
		equip_loss_sum += shouldersParams.equip_loss;
		equip_protect_sum += shouldersParams.equip_protect;
		// Рука 1
		struct params_info hand1Params;
		calculateEquipParams(EquipTypeHand1, &hand1Params);
		force_sum += hand1Params.force;
		dext_sum += hand1Params.dext;
		intell_sum += hand1Params.intell;
		equip_loss_sum += hand1Params.equip_loss;
		equip_protect_sum += hand1Params.equip_protect;
		// Рука 2
		struct params_info hand2Params;
		calculateEquipParams(EquipTypeHand2, &hand2Params);
		force_sum += hand2Params.force;
		dext_sum += hand2Params.dext;
		intell_sum += hand2Params.intell;
		equip_loss_sum += hand2Params.equip_loss;
		equip_protect_sum += hand2Params.equip_protect;
		// Корпус
		struct params_info bodyParams;
		calculateEquipParams(EquipTypeBody, &bodyParams);
		force_sum += bodyParams.force;
		dext_sum += bodyParams.dext;
		intell_sum += bodyParams.intell;
		equip_loss_sum += bodyParams.equip_loss;
		equip_protect_sum += bodyParams.equip_protect;
		// Пояс
		struct params_info strapParams;
		calculateEquipParams(EquipTypeStrap, &strapParams);
		force_sum += strapParams.force;
		dext_sum += strapParams.dext;
		intell_sum += strapParams.intell;
		equip_loss_sum += strapParams.equip_loss;
		equip_protect_sum += strapParams.equip_protect;
		// Ноги
		struct params_info feetParams;
		calculateEquipParams(EquipTypeFeet, &feetParams);
		force_sum += feetParams.force;
		dext_sum += feetParams.dext;
		intell_sum += feetParams.intell;
		equip_loss_sum += feetParams.equip_loss;
		equip_protect_sum += feetParams.equip_protect;
		// Обувь
		struct params_info shoesParams;
		calculateEquipParams(EquipTypeShoes, &shoesParams);
		force_sum += shoesParams.force;
		dext_sum += shoesParams.dext;
		intell_sum += shoesParams.intell;
		equip_loss_sum += shoesParams.equip_loss;
		equip_protect_sum += shoesParams.equip_protect;

		// Теперь считаем доли
		// Урон экипировки
		if (equip_loss_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Урон экипировки -"));
			int equip_loss = weaponParams.equip_loss;
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = shieldParams.equip_loss;
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = headParams.equip_loss;
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = neckParams.equip_loss;
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = shouldersParams.equip_loss;
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = hand1Params.equip_loss;
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = hand2Params.equip_loss;
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = bodyParams.equip_loss;
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = strapParams.equip_loss;
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = feetParams.equip_loss;
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = shoesParams.equip_loss;
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
		}
		// Защита экипировки
		if (equip_protect_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Защита экипировки -"));
			int equip_protect = weaponParams.equip_protect;
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = shieldParams.equip_protect;
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = headParams.equip_protect;
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = neckParams.equip_protect;
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = shouldersParams.equip_protect;
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = hand1Params.equip_protect;
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = hand2Params.equip_protect;
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = bodyParams.equip_protect;
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = strapParams.equip_protect;
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = feetParams.equip_protect;
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = shoesParams.equip_protect;
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
		}
		// Сила
		if (force_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Сила -"));
			resStr.append(QString::fromUtf8("\nРаспределение: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = weaponParams.force;
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = shieldParams.force;
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = headParams.force;
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = neckParams.force;
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = shouldersParams.force;
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = hand1Params.force;
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = hand2Params.force;
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = bodyParams.force;
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = strapParams.force;
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = feetParams.force;
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = shoesParams.force;
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
		}
		// Ловкость
		if (dext_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Ловкость -"));
			resStr.append(QString::fromUtf8("\nРаспределение: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = weaponParams.dext;
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = shieldParams.dext;
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = headParams.dext;
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = neckParams.dext;
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = shouldersParams.dext;
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = hand1Params.dext;
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = hand2Params.dext;
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = bodyParams.dext;
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = strapParams.dext;
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = feetParams.dext;
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = shoesParams.dext;
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
		}
		// Интеллект
		if (intell_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Интеллект -"));
			resStr.append(QString::fromUtf8("\nРаспределение: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = weaponParams.intell;
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = shieldParams.intell;
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = headParams.intell;
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = neckParams.intell;
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = shouldersParams.intell;
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = hand1Params.intell;
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = hand2Params.intell;
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = bodyParams.intell;
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = strapParams.intell;
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = feetParams.intell;
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = shoesParams.intell;
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
		}
	}
	return resStr;
}
