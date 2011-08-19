/*
 * fingfilter.cpp - Sof Game Psi plugin
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

#include "thingfilter.h"
#include "utils.h"

FingFilter::FingFilter()
{
	enabled = true;
}

FingFilter::FingFilter(const FingFilter& from)
{
	enabled = from.enabled;
	filterName = from.filterName;
	rules = from.rules;
}

FingFilter::~FingFilter()
{

}

QString FingFilter::name()
{
	return filterName;
}

void FingFilter::setName(QString newName)
{
	if (!newName.isEmpty())
		filterName = newName;
}

bool FingFilter::isActive()
{
	return enabled;
}

void FingFilter::setActive(bool active)
{
	enabled = active;
}

int FingFilter::rulesCount()
{
	return rules.size();
}

bool FingFilter::appendRule(ParamRole param, bool negative, OperandRole operand, QString value, ActionRole action)
{
	if (param == NoParamRole || action == NoActionRole)
		return false;
	struct fing_rule_ex fr;
	fr.param = param;
	fr.negative = negative;
	fr.operand = operand;
	fr.int_value = 0;
	fr.value = value;
	fr.action = action;
	if (param == NamedRole || param == PriceRole || param == CountRole) {
		bool fOk;
		fr.int_value = value.toInt(&fOk);
		if (!fOk)
			return false;
		if (param == PriceRole && fr.int_value < -1)
			return false;
	} else if (param == TypeRole) {
		fr.int_value = thingTypeFromString(value);
		if (fr.int_value == -1)
			return false;
	} else if (param == DressedRole) {
		operand = NoOperRole;
		value = "";
	}
	if (operand == NoOperRole) {
		if (param != DressedRole)
			return false;
	} else if (operand == ContainsRole) {
		if (param != NameRole)
			return false;
	} else if (operand == AboveRole || operand == LowRole) {
		if (param != NameRole && param != PriceRole && param != NamedRole && param != CountRole)
			return false;
	}
	rules.push_back(fr);
	return true;
}

void FingFilter::modifyRule(int index, const struct fing_rule_ex* new_rule) {
	// TODO Сделать проверку параметров !!!
	if (index >= 0 && index < rules.size())
		rules[index] = *new_rule;
}

void FingFilter::removeRule(int index)
{
	if (index >= 0 && index < rules.size())
		rules.removeAt(index);
}

bool FingFilter::moveRuleUp(int index)
{
	if (index < 1 || index >= rules.size())
		return false;
	rules.swap(index - 1, index);
	return true;
}

bool FingFilter::moveRuleDown(int index)
{
	if (index < 0 || index >= rules.size() - 1)
		return false;
	rules.swap(index, index + 1);
	return true;
}

const struct FingFilter::fing_rule_ex* FingFilter::getRule(int rule_index)
{
	if (rule_index >= 0 && rule_index < rules.size()) {
		return &rules.at(rule_index);
	}
	return 0;
}

/**
 * Возвращает результат прохождения вещи (fingEl) про правилам rules
 */
bool FingFilter::isFingShow(const Thing* thing)
{
	int cnt = rules.size();
	bool skeepNext = false;
	bool ruleOk = false;
	for (int i = 0; i < cnt; i++) {
		ActionRole nAction = rules.at(i).action;
		// проверка на необходимость пропустить правило
		if (skeepNext) {
			if (nAction == NextRole) {
				if (!ruleOk)
					continue;
			} else {
				skeepNext = false;
				if (!ruleOk)
					continue;
			}
		}
		// проверка правила
		ruleOk = false;
		ParamRole nParam = rules.at(i).param;
		if (nParam == NameRole) {
			OperandRole nOper = rules.at(i).operand;
			if (nOper == ContainsRole) {
				ruleOk = (thing->name().contains(rules.at(i).value, Qt::CaseInsensitive));
			} else if (nOper == EqualRole) {
				ruleOk = (thing->name().toLower() == rules.at(i).value);
			}
		} else if (nParam == TypeRole) {
			if (rules.at(i).operand == EqualRole) {
				ruleOk = (thing->type() == rules.at(i).int_value);
			}
		} else if (nParam == NamedRole) {
			OperandRole nOper = rules.at(i).operand;
			if (nOper == EqualRole) {
				ruleOk = (thing->uplevel() == rules.at(i).int_value);
			} else if (nOper == AboveRole) {
				ruleOk = (thing->uplevel() > rules.at(i).int_value);
			} else if (nOper == LowRole) {
				ruleOk = (thing->uplevel() < rules.at(i).int_value);
			}
		} else if (nParam == DressedRole) {
			ruleOk = thing->isDressed();
		} else if (nParam == PriceRole) {
			OperandRole nOper = rules.at(i).operand;
			if (nOper == EqualRole) {
				ruleOk = (thing->price() == rules.at(i).int_value);
			} else if (nOper == AboveRole) {
				ruleOk = (thing->price() > rules.at(i).int_value);
			} else if (nOper == LowRole) {
				ruleOk = (thing->price() < rules.at(i).int_value);
			}
		} else if (nParam == CountRole) {
			OperandRole nOper = rules.at(i).operand;
			if (nOper == EqualRole) {
				ruleOk = (thing->count() == rules.at(i).int_value);
			} else if (nOper == AboveRole) {
				ruleOk = (thing->count() > rules.at(i).int_value);
			} else if (nOper == LowRole) {
				ruleOk = (thing->count() < rules.at(i).int_value);
			}
		}
		if (rules.at(i).negative)
			ruleOk = !ruleOk;
		if (nAction != NextRole) {
			if (ruleOk) {
				if (nAction == YesRole)
					return true;
				if (nAction == NoRole)
					return false;
			}
		} else {
			skeepNext = true;
		}
	}
	return false;
}
