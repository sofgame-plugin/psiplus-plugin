/*
 * thingfilter.h - Sof Game Psi plugin
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

#ifndef THINGFILTER_H
#define THINGFILTER_H

#include <QtCore>
#include <QColor>

#include "../thingstab/thing.h"

class ThingFilter
{
public:
	enum ParamRole {
		NoParamRole,
		NameRole,
		TypeRole,
		NamedRole,
		DressedRole,
		PriceRole,
		CountRole
	};
	enum OperandRole {
		NoOperRole,
		EqualRole,
		ContainsRole,
		AboveRole,
		LowRole
	};
	enum ActionRole {
		UnknowActionRole,
		YesRole,
		NoRole,
		NextRole,
		NullRole
	};
	struct thing_rule_ex {
		ParamRole    param;
		bool         negative;
		OperandRole  operand;
		int          int_value;
		QString      value;
		ActionRole   action;
		QColor       color;
	};
	ThingFilter();
	ThingFilter(const ThingFilter&);
	~ThingFilter();
	QString name() const {return filterName;};
	void setName(const QString &);
	bool isActive() const {return enabled;};
	void setActive(bool);
	int  rulesCount() const;
	bool appendRule(ParamRole, bool, OperandRole, const QString &, ActionRole, const QColor &);
	void modifyRule(int, const struct thing_rule_ex*);
	void removeRule(int);
	bool moveRuleUp(int index);
	bool moveRuleDown(int index);
	const struct thing_rule_ex* getRule(int) const;
	bool isThingShow(const Thing*) const;
	QColor color(const Thing*) const;

private:
	bool implementRulesForThings(const Thing*, QColor *color) const;

private:
	bool enabled;
	QString filterName;
	QList<struct thing_rule_ex> rules;
};

//typedef QList<ThingFilter*> FilterList;

class ThingFiltersList: private QList<ThingFilter*>
{
public:
	ThingFiltersList();
	~ThingFiltersList();
	int  indexByName(const QString &name) const;
	bool isActive(int fltrNum) const;
	void clear();
	inline int size() const {return QList::size();};
	inline ThingFilter *at(int i) const {return QList::at(i);};
	inline void append(ThingFilter *fltr) {QList::append(fltr);};
	inline void insert(int i, ThingFilter *fltr) {QList::insert(i, fltr);};
	inline void replace(int i, ThingFilter *fltr) {QList::replace(i, fltr);};
	inline void swap(int i, int j) {QList::swap(i, j);};
	inline ThingFilter *takeAt(int i) {return QList::takeAt(i);};
};

#endif // THINGFILTER_H
