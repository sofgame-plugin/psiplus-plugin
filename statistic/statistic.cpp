/*
 * statistic.cpp - Sof Game Psi plugin
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

#include "statistic.h"

Statistic::Statistic(QObject *parent) :
	QObject(parent)
{
}

Statistic::~Statistic()
{
	QList<StatItem *>items = itemsList.values();
	while (!items.isEmpty()) {
		delete items.takeFirst();
	}
}

Statistic *Statistic::instance_ = NULL;

Statistic *Statistic::instance()
{
	if (!Statistic::instance_)
		Statistic::instance_ = new Statistic();
	return Statistic::instance_;
}

void Statistic::reset()
{
	if (Statistic::instance_) {
		delete Statistic::instance_;
		Statistic::instance_ = NULL;
	}
}

bool Statistic::isEmpty(int type) const
{
	StatItem *item = itemsList.value(type, NULL);
	if (!item)
		return true;
	return item->isEmpty();

}

QVariant Statistic::value(int type) const
{
	StatItem *item = itemsList.value(type, NULL);
	if (!item)
		return QVariant();
	return item->value();
}

void Statistic::setValue(int type, QVariant val)
{
	StatItem *item = itemsList.value(type, NULL);
	if (!item) {
		if (!val.isValid()) // Если значение Null, элемент статистики не создается
			return;
		item = newStatItem(type);
		if (!item)
			return;
		itemsList[type] = item;
	}
	if (item->setValue(val)) {
		emit valueChanged(type);
	}
}

StatItem *Statistic::newStatItem(int type)
{
	StatItem *item = NULL;
	if (type == StatMessagesCount || type == StatFightsCount || type == StatDropMoneys || type == StatThingsDropCount
		|| type == StatKilledEnemies) {
		item = new StatIntZeroItem();
	} else if (type == StatDamageMaxFromPers || type == StatDamageMinFromPers) {
		item = new StatIntItem();
	} else if (type == StatLastGameJid || type == StatLastChatJid || type == StatThingDropLast) {
		item = new StatStringItem();
	} else if (type == StatExperienceDropCount) {
		item = new StatLongZeroItem();
	}
	return item;
}

QString Statistic::toString(int type) const
{
	StatItem *item = itemsList.value(type, NULL);
	if (!item) {
		if (type == StatMessagesCount || type == StatFightsCount || type == StatDropMoneys
			|| type == StatThingsDropCount || type == StatExperienceDropCount || type == StatKilledEnemies) {
			return EmptyStringZero::emptyStringValue();
		}
		return EmptyStringNA::emptyStringValue();
	}
	return item->toString();
}
