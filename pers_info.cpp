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

bool PersInfo::getForce(int *f1, int *f2) const
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

bool PersInfo::getDext(int *d1, int *d2) const
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

bool PersInfo::getIntell(int *i1, int *i2) const
{
	if (intell1 == QINT32_MIN || intell2 == QINT32_MIN)
		return false;
	*i1 = intell1;
	*i2 = intell2;
	return true;
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

void PersInfo::setEquip(const EquipItem &eItem)
{
	if (eItem.equipType() == EquipItem::EquipTypeWeapon)
		_weapon = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeShield)
		_shield = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeHead)
		_head = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeNeck)
		_neck = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeShoulders)
		_shoulders = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeBody)
		_body = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeHand1)
		_hand1 = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeHand2)
		_hand2 = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeStrap)
		_strap = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeFeet)
		_feet = eItem;
	if (eItem.equipType() == EquipItem::EquipTypeShoes)
		_shoes = eItem;
	equipLossCalc = QINT32_MIN;
	equipProtectCalc = QINT32_MIN;
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
	if (_weapon.isValid())
		loss += _weapon.damageForLevel(persLevel);
	if (_shield.isValid())
		loss += _shield.damageForLevel(persLevel);
	if (_head.isValid())
		loss += _head.damageForLevel(persLevel);
	if (_neck.isValid())
		loss += _neck.damageForLevel(persLevel);
	if (_shoulders.isValid())
		loss += _shoulders.damageForLevel(persLevel);
	if (_body.isValid())
		loss += _body.damageForLevel(persLevel);
	if (_hand1.isValid())
		loss += _hand1.damageForLevel(persLevel);
	if (_hand2.isValid())
		loss += _hand2.damageForLevel(persLevel);
	if (_strap.isValid())
		loss += _strap.damageForLevel(persLevel);
	if (_feet.isValid())
		loss += _feet.damageForLevel(persLevel);
	if (_shoes.isValid())
		loss += _shoes.damageForLevel(persLevel);
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
	if (_weapon.isValid())
		protect += _weapon.protectForLevel(persLevel);
	if (_shield.isValid())
		protect += _shield.protectForLevel(persLevel);
	if (_head.isValid())
		protect += _head.protectForLevel(persLevel);
	if (_neck.isValid())
		protect += _neck.protectForLevel(persLevel);
	if (_shoulders.isValid())
		protect += _shoulders.protectForLevel(persLevel);
	if (_body.isValid())
		protect += _body.protectForLevel(persLevel);
	if (_hand1.isValid())
		protect += _hand1.protectForLevel(persLevel);
	if (_hand2.isValid())
		protect += _hand2.protectForLevel(persLevel);
	if (_strap.isValid())
		protect += _strap.protectForLevel(persLevel);
	if (_feet.isValid())
		protect += _feet.protectForLevel(persLevel);
	if (_shoes.isValid())
		protect += _shoes.protectForLevel(persLevel);
	equipProtectCalc = (int)protect;
}

QString PersInfo::getSharpening(int calcVal, int currVal)
{
	QString res;
	if (calcVal != QINT32_MIN && currVal != QINT32_MIN)
	{
		if (calcVal != 0)
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
	if (_weapon.isValid())
	{
		float namedEffect = _weapon.efficiency();
		if (ver == 2)
			resStr.append(_weapon.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _weapon.upLevel();
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
	if (_shield.isValid())
	{
		float namedEffect = _shield.efficiency();
		if (ver == 2)
			resStr.append(_shield.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _shield.upLevel();
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
	if (_head.isValid())
	{
		float namedEffect = _head.efficiency();
		if (ver == 2)
			resStr.append(_head.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _head.upLevel();
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
	if (_neck.isValid())
	{
		float namedEffect = _neck.efficiency();
		if (ver == 2)
			resStr.append(_neck.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _neck.upLevel();
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
	if (_shoulders.isValid())
	{
		float namedEffect = _shoulders.efficiency();
		if (ver == 2)
			resStr.append(_shoulders.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _shoulders.upLevel();
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
	if (_hand1.isValid())
	{
		float namedEffect = _hand1.efficiency();
		if (ver == 2)
			resStr.append(_hand1.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _hand1.upLevel();
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
	if (_hand2.isValid())
	{
		float namedEffect = _hand2.efficiency();
		if (ver == 2)
			resStr.append(_hand2.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _hand2.upLevel();
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
	if (_body.isValid())
	{
		float namedEffect = _body.efficiency();
		if (ver == 2)
			resStr.append(_body.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _body.upLevel();
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
	if (_strap.isValid())
	{
		float namedEffect = _strap.efficiency();
		if (ver == 2)
			resStr.append(_strap.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _strap.upLevel();
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
	if (_feet.isValid())
	{
		float namedEffect = _feet.efficiency();
		if (ver == 2)
			resStr.append(_feet.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _feet.upLevel();
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
	if (_shoes.isValid())
	{
		float namedEffect = _shoes.efficiency();
		if (ver == 2)
			resStr.append(_shoes.toString() + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
		equipCount++;
		int level = _shoes.upLevel();
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
	if (namedCount > 0)
	{
		resStr.append(QString::fromUtf8("\nОбщий уровень именных: %1").arg(namedLevelAll));
		resStr.append(QString::fromUtf8("\nОбщая эффективность именных: %1 / %2").arg(numToStr(floor(namedEffectAll + 0.51f), "'")).arg(namedEffectCount));
	}

	if (equipCount > 0 && ver == 2)
	{
		resStr.append(QString::fromUtf8("\n--- Долевой вклад экипировки ---"));
		int force_sum = nForce;
		int dext_sum = nDext;
		int intell_sum = nIntell;
		int equip_loss_sum = 0;
		int equip_protect_sum = 0;
		// Оружие
		equip_loss_sum += _weapon.damageForLevel(persLevel);
		equip_protect_sum += _weapon.protectForLevel(persLevel);
		force_sum += _weapon.forceForLevel(persLevel);
		dext_sum += _weapon.dextForLevel(persLevel);
		intell_sum += _weapon.intellForLevel(persLevel);
		// Щит
		equip_loss_sum += _shield.damageForLevel(persLevel);
		equip_protect_sum += _shield.protectForLevel(persLevel);
		force_sum += _shield.forceForLevel(persLevel);
		dext_sum += _shield.dextForLevel(persLevel);
		intell_sum += _shield.intellForLevel(persLevel);
		// Голова
		equip_loss_sum += _head.damageForLevel(persLevel);
		equip_protect_sum += _head.protectForLevel(persLevel);
		force_sum += _head.forceForLevel(persLevel);
		dext_sum += _head.dextForLevel(persLevel);
		intell_sum += _head.intellForLevel(persLevel);
		// Шея
		equip_loss_sum += _neck.damageForLevel(persLevel);
		equip_protect_sum += _neck.protectForLevel(persLevel);
		force_sum += _neck.forceForLevel(persLevel);
		dext_sum += _neck.dextForLevel(persLevel);
		intell_sum += _neck.intellForLevel(persLevel);
		// Плечи
		equip_loss_sum += _shoulders.damageForLevel(persLevel);
		equip_protect_sum += _shoulders.protectForLevel(persLevel);
		force_sum += _shoulders.forceForLevel(persLevel);
		dext_sum += _shoulders.dextForLevel(persLevel);
		intell_sum += _shoulders.intellForLevel(persLevel);
		// Рука 1
		equip_loss_sum += _hand1.damageForLevel(persLevel);
		equip_protect_sum += _hand1.protectForLevel(persLevel);
		force_sum += _hand1.forceForLevel(persLevel);
		dext_sum += _hand1.dextForLevel(persLevel);
		intell_sum += _hand1.intellForLevel(persLevel);
		// Рука 2
		equip_loss_sum += _hand2.damageForLevel(persLevel);
		equip_protect_sum += _hand2.protectForLevel(persLevel);
		force_sum += _hand2.forceForLevel(persLevel);
		dext_sum += _hand2.dextForLevel(persLevel);
		intell_sum += _hand2.intellForLevel(persLevel);
		// Корпус
		equip_loss_sum += _body.damageForLevel(persLevel);
		equip_protect_sum += _body.protectForLevel(persLevel);
		force_sum += _body.forceForLevel(persLevel);
		dext_sum += _body.dextForLevel(persLevel);
		intell_sum += _body.intellForLevel(persLevel);
		// Пояс
		equip_loss_sum += _strap.damageForLevel(persLevel);
		equip_protect_sum += _strap.protectForLevel(persLevel);
		force_sum += _strap.forceForLevel(persLevel);
		dext_sum += _strap.dextForLevel(persLevel);
		intell_sum += _strap.intellForLevel(persLevel);
		// Ноги
		equip_loss_sum += _feet.damageForLevel(persLevel);
		equip_protect_sum += _feet.protectForLevel(persLevel);
		force_sum += _feet.forceForLevel(persLevel);
		dext_sum += _feet.dextForLevel(persLevel);
		intell_sum += _feet.intellForLevel(persLevel);
		// Обувь
		equip_loss_sum += _shoes.damageForLevel(persLevel);
		equip_protect_sum += _shoes.protectForLevel(persLevel);
		force_sum += _shoes.forceForLevel(persLevel);
		dext_sum += _shoes.dextForLevel(persLevel);
		intell_sum += _shoes.intellForLevel(persLevel);

		// Теперь считаем доли
		// Урон экипировки
		if (equip_loss_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Урон экипировки -"));
			int equip_loss = _weapon.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _shield.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _head.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _neck.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _shoulders.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _hand1.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _hand2.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _body.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _strap.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _feet.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
			equip_loss = _shoes.damageForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(equip_loss).arg(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f));
		}
		// Защита экипировки
		if (equip_protect_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Защита экипировки -"));
			int equip_protect = _weapon.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _shield.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _head.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _neck.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _shoulders.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _hand1.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _hand2.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _body.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _strap.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _feet.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
			equip_protect = _shoes.protectForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(equip_protect).arg(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f));
		}
		// Сила
		if (force_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Сила -"));
			resStr.append(QString::fromUtf8("\nРаспределение: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _weapon.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _shield.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _head.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _neck.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _shoulders.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _hand1.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _hand2.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _body.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _strap.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _feet.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
			nForce = _shoes.forceForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(nForce).arg(floor((float)nForce / (float)force_sum * 1000.0f + 0.5f) / 10.0f));
		}
		// Ловкость
		if (dext_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Ловкость -"));
			resStr.append(QString::fromUtf8("\nРаспределение: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _weapon.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _shield.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _head.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _neck.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _shoulders.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _hand1.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _hand2.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _body.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _strap.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _feet.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
			nDext = _shoes.dextForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(nDext).arg(floor((float)nDext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f));
		}
		// Интеллект
		if (intell_sum > 0)
		{
			resStr.append(QString::fromUtf8("\n- Интеллект -"));
			resStr.append(QString::fromUtf8("\nРаспределение: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _weapon.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОружие: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _shield.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nЩит:    %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _head.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nГолова: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _neck.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nШея:    %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _shoulders.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПлечи:  %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _hand1.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 1: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _hand2.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nРука 2: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _body.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nКорпус: %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _strap.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nПояс:   %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _feet.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nНоги:   %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
			nIntell = _shoes.intellForLevel(persLevel);
			resStr.append(QString::fromUtf8("\nОбувь:  %1 [%2%]").arg(nIntell).arg(floor((float)nIntell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f));
		}
	}
	return resStr;
}
