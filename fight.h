/*
 * fight.h - Sof Game Psi plugin
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

#ifndef FIGHT_H
#define FIGHT_H

#include <QtCore>

/*QT_BEGIN_NAMESPACE
	class QGraphicsEllipseItem;
QT_END_NAMESPACE*/


#define FIGHT_MODE_UNDEFINE  0
#define FIGHT_MODE_OPEN      1
#define FIGHT_MODE_CLOSE     2

class Fight: public QObject
{
  Q_OBJECT

	public:
		Fight();
		~Fight();
		bool isActive();
		void start();
		void setMode(int mode);
		void finish();
		void setStep(int);
		void setDamage(int);
		int  minDamage();
		int  maxDamage();
		//void setEnemies();
		int  enemiesCount();
		int  allyCount();
		int  getStep();
		void startAddEnemies();
		void startAddAllyes();
		void stopAddAllyes();
		void startAddAllyAuras();
		void stopAddAllyAuras();
		void setMyPersInFight(bool present);
		bool isPersInFight();
		void setGameMobEnemy(int enemNum, QString &enemName, int enemHealth, int enemHealthMax);
		void setHumanEnemy(int enemNum, QString &enemName, QString &enemCountry, int enemLevel, int enemHealth, int enemHealthMax);
		void setGameMobAlly(QString &allyName, int allyHealth, int allyHealthMax);
		void setGameHumanAlly(QString &allyName, QString &allyCountry, int allyLevel, int allyHealth, int allyHealthMax);
		void setAuraEnemy(QString &auraName, QString &auraParam, int auraValue);
		void setAuraAlly(const QString &auraName, const QString &auraParam, const QString &auraValue);
		void stopAddEnemies();
		int  gameMobEnemyCount();
		int  gameHumanEnemyCount();
		int  gameMobAllyCount();
		int  gameHumanAllyCount();
		int  allyAurasCount() const {return auraAllyList.size();};
		QString getAllyAuraValue(const QString &name) const;
		void setTimeout(int);
		int  timeout();
		QStringList mobEnemiesList();
		// --
		int  damageMin;
		int  damageMax;

	private:
		struct Mob {
			int     num;
			QString name;
			int     health;
			int     healthMax;
		};
		struct Aura
		{
			QString name;
			QString param;
			QString value;
			Aura(const QString &nm, const QString &par, const QString &val) : name(nm), param(par), value(val) {};
		};

		bool active;
		int status;
		int step;
		int humanEnemyCount;
		int mobAllyCount;
		int humanAllyCount;
		int fightMode;
		QDateTime timeoutStart;
		int timeoutValue;
		bool presentPers;
		bool eventSend;
		QList<Mob> gameMobEnemy;
		QList<Aura> auraAllyList;

	signals:
		void fightStart(int mode);
		void enemiesListUpdate();
		void allyesListUpdate();

};

extern Fight* fight;

#endif
