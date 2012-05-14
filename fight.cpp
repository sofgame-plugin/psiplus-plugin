/*
 * fight.cpp - Sof Game Psi plugin
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

#include "fight.h"

Fight* fight;

Fight::Fight()
{
	active = false;
	status = 0;
	step = 0;
	humanEnemyCount = 0;
	mobAllyCount = 0;
	humanAllyCount = 0;
	fightMode = FIGHT_MODE_UNDEFINE;
	presentPers = false;
	eventSend = false;
	damageMin = -1;
	damageMax = -1;
}

Fight::~Fight()
{
}

bool Fight::isActive()
{
	return active;
}

void Fight::start()
{
	if (!active) {
		active = true;
		step = 0;
		eventSend = false;
		damageMin = -1;
		damageMax = -1;
	}
}

void Fight::finish()
{
	active = false;
	fightMode = FIGHT_MODE_UNDEFINE;
}

void Fight::setStep(int nStep)
{
	if (active) {
		step = nStep;
	}
}

void Fight::setDamage(int damage)
{
	if (!active)
		return;
	if (damageMin == -1 || damageMin > damage)
		damageMin = damage;
	if (damageMax < damage)
		damageMax = damage;
}

int Fight::minDamage()
{
	return damageMin;
}

int Fight::maxDamage()
{
	return damageMax;
}

int Fight::enemiesCount()
{
	return gameMobEnemyCount() + gameHumanEnemyCount();
}

int Fight::allyCount()
{
	return gameMobAllyCount() + gameHumanAllyCount();
}

void Fight::setMode(int mode)
{
	if (active) {
		fightMode = mode;
	}
}

int Fight::getStep()
{
	return step;
}

void Fight::startAddEnemies()
{
	if (active) {
		gameMobEnemy.clear();
		humanEnemyCount = 0;
	}
}

void Fight::startAddAllyes()
{
	/**
	* Начало добавления списка союзников
	**/
	if (active) {
		mobAllyCount = 0;
		humanAllyCount = 0;
	}
}

void Fight::stopAddAllyes()
{
	/**
	* Завершение добавления списка союзников
	**/
	if (active) {
		// Уведомление об изменении списка союзников
		emit allyesListUpdate();
	}
}

void Fight::startAddAllyAuras()
{
	if (active)
	{
		auraAllyList.clear();
	}
}

void Fight::stopAddAllyAuras()
{
	if (active)
	{
		//
	}
}

/**
 * Устанавливает присутствие персонажа в бою
 */
void Fight::setMyPersInFight(bool present)
{
	presentPers = present;
}

/**
 * Возвращает присутствие персонажа в бою
 */
bool Fight::isPersInFight()
{
	return (presentPers && active);
}

void Fight::setGameMobEnemy(int enemNum, QString &enemName, int enemHealth, int enemHealthMax)
{
	if (active) {
		struct Mob mob;
		mob.num = enemNum;
		mob.name = enemName;
		mob.health = enemHealth;
		mob.healthMax = enemHealthMax;
		gameMobEnemy.push_back(mob);
	}
}

void Fight::setHumanEnemy(int enemNum, QString &enemName, QString &enemCountry, int enemLevel, int enemHealth, int enemHealthMax)
{
	Q_UNUSED(enemNum);
	Q_UNUSED(enemName);
	Q_UNUSED(enemCountry);
	Q_UNUSED(enemLevel);
	Q_UNUSED(enemHealth);
	Q_UNUSED(enemHealthMax);

	if (active) {
		humanEnemyCount++;
	}
}

void Fight::setGameMobAlly(QString &allyName, int allyHealth, int allyHealthMax)
{
	Q_UNUSED(allyName);
	Q_UNUSED(allyHealth);
	Q_UNUSED(allyHealthMax);

	if (active) {
		mobAllyCount++;
	}
}

void Fight::setGameHumanAlly(QString &allyName, QString &allyCountry, int allyLevel, int allyHealth, int allyHealthMax)
{
	Q_UNUSED(allyName);
	Q_UNUSED(allyCountry);
	Q_UNUSED(allyLevel);
	Q_UNUSED(allyHealth);
	Q_UNUSED(allyHealthMax);

	if (active) {
		humanAllyCount++;
	}
}

void Fight::setAuraEnemy(QString &auraName, QString &auraParam, int auraValue)
{
	Q_UNUSED(auraName);
	Q_UNUSED(auraParam);
	Q_UNUSED(auraValue);

	if (active) {

	}
}

void Fight::setAuraAlly(const QString &auraName, const QString &auraParam, const QString &auraValue)
{
	if (active) {
		auraAllyList.append(Aura(auraName, auraParam, auraValue));
	}
}

void Fight::stopAddEnemies()
{
	/**
	* Завершает добавление противников
	**/
	if (active) {
		if (step == 1) {
			// Уведомление о начале режима боя
			if (!eventSend) {
				emit fightStart(fightMode);
				eventSend = true;
			}
		}
		// Уведомление об изменении списка противников
		emit enemiesListUpdate();
	}
}

int Fight::gameMobEnemyCount()
{
	/**
	* Возвращает текущее количество игровых мобов в команде врагов
	**/
	if (!active) {
		return 0;
	}
	return gameMobEnemy.size();
}

int Fight::gameHumanEnemyCount()
{
	/**
	* Возвращает текущее количество персонажей людей в команде врагов
	**/
	if (!active) {
		return 0;
	}
	return humanEnemyCount;
}

int Fight::gameMobAllyCount()
{
	/**
	* Возвращает текущее количество игровых мобов в команде союзников
	**/
	if (!active) {
		return 0;
	}
	return mobAllyCount;
}

int Fight::gameHumanAllyCount()
{
	/**
	* Возвращает текущее количество персонажей людей в команде союзников
	**/
	if (!active) {
		return 0;
	}
	return humanAllyCount;
}

QString Fight::getAllyAuraValue(const QString &name) const
{
	foreach (const Aura &aura, auraAllyList)
	{
		if (aura.name == name)
			return aura.value;
	}
	return QString();
}

void Fight::setTimeout(int timeout)
{
	if (active) {
		timeoutStart = QDateTime::currentDateTime();
		timeoutValue = timeout;
	}
}

int Fight::timeout()
{
	if (!active || !timeoutStart.isValid())
		return 0;
	int time_delta = timeoutStart.secsTo(QDateTime::currentDateTime());
	time_delta = timeoutValue - time_delta;
	return time_delta;
}

QStringList Fight::mobEnemiesList()
{
	QStringList resList;
	for (int i = 0; i < gameMobEnemy.size(); i++) {
		resList.push_back(gameMobEnemy.at(i).name);
	}
	return resList;
}
