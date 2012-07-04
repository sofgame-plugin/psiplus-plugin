/*
 * equipitem.cpp - Sof Game Psi plugin
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

#include <QtCore>

#include "equipitem.h"
#include "utils.h"

EquipItem::EquipItem()
{
	reset();
}

EquipItem::EquipItem(const EquipItem &other)
{
	status = other.status;
	_name = other._name;
	type = other.type;
	_upLevel = other._upLevel;
	damage = other.damage;
	damageMul = other.damageMul;
	damageSet = other.damageSet;
	protect = other.protect;
	protectMul = other.protectMul;
	protectSet = other.protectSet;
	force = other.force;
	forceMul = other.forceMul;
	forceSet = other.forceSet;
	dext = other.dext;
	dextMul = other.dextMul;
	dextSet = other.dextSet;
	intell = other.intell;
	intellMul = other.intellMul;
	intellSet = other.intellSet;
}

EquipItem::EquipItem(const QString &s)
{
	loadFromString(s);
}

bool EquipItem::isValid() const
{
	if (status == Unknow)
	{
		status = Invalid;
		if (!_name.isEmpty() && type != -1)
		{
			if (damage != 0 || damageMul != 0.0f || protect != 0 || protectMul != 0.0f
					|| force != 0 || forceMul != 0.0f || dext != 0 || dextMul != 0.0f || intell != 0 || intellMul != 0.0f)
				status = Valid;
		}
	}
	return (status == Valid);
}

void EquipItem::reset()
{
	status = Invalid;
	_name = QString();
	type = -1;
	_upLevel = 0;
	damage = 0; damageMul = 0.0f; damageSet = 0;
	protect = 0; protectMul = 0.0f; protectSet = 0;
	force = 0; forceMul = 0.0f; forceSet = 0;
	dext = 0; dextMul = 0.0f; dextSet = 0;
	intell = 0; intellMul = 0.0f; intellSet = 0;
}

QRegExp EquipItem::equipElementReg(QString::fromUtf8("^(.+)\\((\\w+)\\)(.+)\\{(.+)\\}(И:([0-9]+)ур\\.)?$"));
QRegExp EquipItem::equipParamElementReg(QString::fromUtf8("^(\\w+):([0-9.]+)(\\*\\w+)?$"));

/**
 * @brief Загружает параметры экипировки из строки
 * @param s - строка с параметрами экипировки
 */
void EquipItem::loadFromString(const QString &s)
{
	// Обруч ярости Демона(амулет)сила:1.8*ур;ловк:4.2*ур;инт:1.8*ур;урон:10.5*ур;защ:6.2*ур;{Треб:Ур13Сил39}И:8ур.
	if (equipElementReg.indexIn(s, 0) == -1)
		return;

	_name = equipElementReg.cap(1).trimmed();
	type = thingTypeFromString(equipElementReg.cap(2));
	_upLevel = (equipElementReg.cap(6).isEmpty()) ? 0 : equipElementReg.cap(6).toInt();

	QStringList aParams = equipElementReg.cap(3).trimmed().split(";", QString::SkipEmptyParts);
	for (int i = 0, cnt = aParams.size(); i < cnt; ++i)
	{
		if (equipParamElementReg.indexIn(aParams[i], 0) != -1)
		{
			QString sParam = equipParamElementReg.cap(1);
			if (sParam == QString::fromUtf8("урон"))
			{
				if (equipParamElementReg.cap(3).isEmpty())
					damage += equipParamElementReg.cap(2).toInt();
				else
					damageMul += equipParamElementReg.cap(2).toFloat();
			}
			else if (sParam == QString::fromUtf8("защ"))
			{
				if (equipParamElementReg.cap(3).isEmpty())
					protect += equipParamElementReg.cap(2).toInt();
				else
					protectMul += equipParamElementReg.cap(2).toFloat();
			}
			else if (sParam == QString::fromUtf8("сила"))
			{
				if (equipParamElementReg.cap(3).isEmpty())
					force += equipParamElementReg.cap(2).toInt();
				else
					forceMul += equipParamElementReg.cap(2).toFloat();
			}
			else if (sParam == QString::fromUtf8("ловк"))
			{
				if (equipParamElementReg.cap(3).isEmpty())
					dext += equipParamElementReg.cap(2).toInt();
				else
					dextMul += equipParamElementReg.cap(2).toFloat();
			}
			else if (sParam == QString::fromUtf8("инт"))
			{
				if (equipParamElementReg.cap(3).isEmpty())
					intell += equipParamElementReg.cap(2).toInt();
				else
					intellMul += equipParamElementReg.cap(2).toFloat();
			}
		}
	}
	status = Unknow;
}

/**
 * @brief Добавляет бонус экипировки к существующим, извлекая данные из строки
 * @param s - строка, описывающая бонус экипировки вида: защ:78
 */
void EquipItem::setEquipBonus(const QString &s)
{
	QStringList bonusSplit = s.split(":");
	if (bonusSplit.size() == 2)
	{
		const QString sBonus = bonusSplit.at(0).trimmed().toLower();
		int val = bonusSplit.at(1).toInt();
		if (sBonus == QString::fromUtf8("урон"))
		{
			damageSet += val;
		}
		else if (sBonus == QString::fromUtf8("защ"))
		{
			protectSet += val;
		}
		else if (sBonus == QString::fromUtf8("сила"))
		{
			forceSet += val;
		}
		else if (sBonus == QString::fromUtf8("ловк"))
		{
			dextSet += val;
		}
		else if (sBonus == QString::fromUtf8("инт"))
		{
			intellSet += val;
		}
	}
}

float EquipItem::damageForLevel(int level) const
{
	float res = floor((float)level * damageMul + 0.01f);
	res += damage + damageSet;
	return res;
}

float EquipItem::protectForLevel(int level) const
{
	float res = floor((float)level * protectMul + 0.01f);
	res += protect + protectSet;
	return res;
}

float EquipItem::forceForLevel(int level) const
{
	float res = floor((float)level * forceMul + 0.01f);
	res += force + forceSet;
	return res;
}

float EquipItem::dextForLevel(int level) const
{
	float res = floor((float)level * dextMul + 0.01f);
	res += dext + dextSet;
	return res;
}

float EquipItem::intellForLevel(int level) const
{
	float res = floor((float)level * intellMul + 0.01f);
	res += intell + intellSet;
	return res;
}

/**
 * @brief Расчитывает эффективность элемента экипировки
 * @return
 */
float EquipItem::efficiency() const
{
	float res = 0.0f;
	if (_upLevel >= 7)
	{
		res = 7.0f - (float)_upLevel;
		res += damageMul + protectMul + forceMul * 10.0f + dextMul * 10.0f + intellMul * 10.0f;
		int i1 = damage + damageSet + protect + protectSet;
		int i2 = force + forceSet + dext + dextSet + intell + intellSet;
		res += (float)i1 / 25.0f + (float)i2 / 2.5f;
	}
	return res;
}

/**
 * @brief Формирует строку элемента экипировки
 * @return
 */
QString EquipItem::toString() const
{
	QString sRes = QString();
	if (isValid())
	{
		sRes.append(_name + QString::fromUtf8(" урон:"));
		float mul = damageMul;
		int abs = damage;
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
		mul = protectMul;
		abs = protect;
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
		mul = forceMul;
		abs = force;
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
		mul = dextMul;
		abs = dext;
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
		mul = intellMul;
		abs = intell;
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
