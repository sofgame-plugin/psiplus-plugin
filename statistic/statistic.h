/*
 * statistic.h - Sof Game Psi plugin
 * Copyright (C) 2011  Aleksey Andreev
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

#ifndef STATISTIC_H
#define STATISTIC_H

#include <QObject>

#include "statitem.h"

class Statistic : public QObject
{
Q_OBJECT
public:
	enum {StatLastGameJid, StatLastChatJid, StatMessagesCount,
		StatDamageMaxFromPers, StatDamageMinFromPers, StatFightsCount,
		StatDropMoneys, StatThingsDropCount, StatThingDropLast,
		StatExperienceDropCount, StatKilledEnemies};

	static Statistic *instance();
	static void reset();
	bool isEmpty(int type) const;
	QVariant value(int type) const;
	void setValue(int type, QVariant val);
	QString toString(int type) const;

private:
	Statistic(QObject *parent = 0);
	~Statistic();
	static StatItem *newStatItem(int type);

private:
	static Statistic *instance_;
	QHash<int, StatItem *> itemsList;

signals:
	void valueChanged(int);

};

#endif // STATISTIC_H
