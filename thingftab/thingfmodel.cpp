/*
 * thingfmodel.cpp - Sof Game Psi plugin
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

#include <QMessageBox>

#include "thingfmodel.h"

ThingFiltersModel::ThingFiltersModel(QObject* parent, QList<ThingFilter*>* flp)
{
	Q_UNUSED(parent)
	filtersListPtr = flp;
	columnNames << QString::fromUtf8("Статус") << QString::fromUtf8("Название") << QString::fromUtf8("Кол-во правил");
	roles << StatusRole << NameRole << CountRole;
	filtersListPtr->clear();
	connect(Pers::instance(), SIGNAL(filtersChanged()), this, SLOT(filtersChanged()));
}

ThingFiltersModel::~ThingFiltersModel()
{
}

void ThingFiltersModel::reloadFilters()
{
	// Очищаем старые
	while (!filtersListPtr->isEmpty())
		delete filtersListPtr->takeFirst();
	// Грузим новые
	QList<ThingFilter*> ffl;
	Pers::instance()->getThingsFiltersEx(&ffl);
	while (!ffl.isEmpty()) {
		ThingFilter* ff = new ThingFilter(*ffl.takeFirst());
		filtersListPtr->push_back(ff);
	}
	reset();
}

ThingFilter* ThingFiltersModel::getFilterByRow(int row) const
{
	if (row < 0 || row >= filtersListPtr->size())
		return NULL;
	return filtersListPtr->at(row);
}

int ThingFiltersModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	return filtersListPtr->size();
}

int ThingFiltersModel::columnCount(const QModelIndex &/*parent*/) const
{
	return columnNames.size();
}

QVariant ThingFiltersModel::data(const QModelIndex &index, int role) const
{
	Role columnRole = roles[index.column()];
	if (role == Qt::TextAlignmentRole) {
		switch (columnRole) {
			case NameRole:
				return (int)(Qt::AlignLeft | Qt::AlignVCenter);
			default:
				return (int)(Qt::AlignCenter | Qt::AlignVCenter);
		}
	} else if (role == Qt::DisplayRole) {
		int row = index.row();
		if (row >= 0 && row < filtersListPtr->size()) {
			ThingFilter* filterPtr = filtersListPtr->at(row);
			if (filterPtr) {
				switch (columnRole) {
					case StatusRole:
						if (filterPtr->isActive()) {
							return QString::fromUtf8("Активен");
						} else {
							return QString::fromUtf8("Неактивен");
						}
					case NameRole:
						return filterPtr->name();
					case CountRole:
						return QString::number(filterPtr->rulesCount());
				}
			}
		}
	} else if (role == Qt::UserRole) {
		int row = index.row();
		if (row >= 0 && row < filtersListPtr->size()) {
			if (columnRole == StatusRole) {
				ThingFilter* filterPtr = filtersListPtr->at(row);
				if (filterPtr) {
					return filterPtr->isActive();
				} else {
					return false;
				}
			}
		}
	}
	return QVariant();
}

QVariant ThingFiltersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section >= 0 && section < columnNames.size()) {
			if (orientation == Qt::Horizontal) {
				return columnNames[section];
			}
		}
	}
	return QVariant();
}

bool ThingFiltersModel::insertRows(int row, int count, const QModelIndex &/*parent*/)
{
	if (row < 0 || count <= 0)
		return false;
	int cnt = filtersListPtr->size();
	if (row < cnt) {
		beginInsertRows(QModelIndex(), row, row + count - 1);
		for (int i = 0; i < count; i++)
			filtersListPtr->insert(row, 0);
	} else {
		beginInsertRows(QModelIndex(), cnt, cnt + count - 1);
		for (int i = 0; i < count; i++)
			filtersListPtr->push_back(0);
	}
	endInsertRows();
	return true;
}

bool ThingFiltersModel::setData(const QModelIndex &idx, const QVariant &value, int /*role*/)
{
	int row = idx.row();
	int col = idx.column();
	if (row < 0 || row >= filtersListPtr->size() || col < 0 || col >= columnNames.size())
		return false;
	ThingFilter* ff = filtersListPtr->at(row);
	if (!ff) {
		ff = new ThingFilter();
		filtersListPtr->replace(row, ff);
	}
	Role rowRole = roles.at(col);
	switch (rowRole) {
		case StatusRole:
			ff->setActive(value.toBool());
			break;
		case NameRole:
			ff->setName(value.toString());
			break;
		default:
			return false;
	}
	emit dataChanged(index(row, col), index(row, col));
	return true;
}

bool ThingFiltersModel::swapRows(int row1, int row2)
{
	int cnt = filtersListPtr->size();
	if (row1 < 0 || row2 < 0 || row1 >= cnt || row2 >= cnt || row1 == row2)
		return false;
	if (row1 > row2) {
		int row3 = row1;
		row1 = row2;
		row2 = row3;
	}
	filtersListPtr->swap(row1, row2);
	emit dataChanged(index(row1, 0), index(row2, columnCount()));
	return true;
}

bool ThingFiltersModel::removeRows(int row, int count, const QModelIndex &/*parent*/)
{
	if (count > 0) {
		int cntFltrs = filtersListPtr->size();
		if (row >= 0 && row < cntFltrs) {
			int endRow = row + count - 1;
			if (endRow >= cntFltrs)
				endRow = cntFltrs - 1;
			beginRemoveRows(QModelIndex(), row, endRow);
			while (row < filtersListPtr->size()) {
				ThingFilter* ff = filtersListPtr->takeAt(row);
				if (ff)
					delete ff;
				endRow--;
				if (endRow < row)
					break;
			}
			endRemoveRows();
			return true;
		}
	}
	return false;
}

void ThingFiltersModel::clear()
{
	filtersListPtr->clear();
}

Qt::ItemFlags ThingFiltersModel::flags(const QModelIndex &/*index*/) const
{
	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return flags;
}

void ThingFiltersModel::filtersChanged()
{
	reloadFilters();
}

//****************************************************************************************************************

ThingRulesModel::ThingRulesModel(QObject* /*parent*/, QList<ThingFilter*>* flp)
{
	filtersListPtr = flp;
	currFilterIndex = -1;
	columnNames << QString::fromUtf8("Параметр") << QString::fromUtf8("Операнд") << QString::fromUtf8("Значение") << QString::fromUtf8("Действие");
	roles << FieldRole << OperandRole << ValueRole << ActionRole;
}

void ThingRulesModel::reloadRules(int filterIndex)
{
	currFilterIndex = filterIndex;
	reset();
}

const struct ThingFilter::thing_rule_ex* ThingRulesModel::getRule(int row) const
{
	if (currFilterIndex >= 0 && currFilterIndex < filtersListPtr->size()) {
		ThingFilter* ff = filtersListPtr->at(currFilterIndex);
		if (ff) {
			return ff->getRule(row);
		}
	}
	return 0;
}

int ThingRulesModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	if (currFilterIndex >= 0 && currFilterIndex < filtersListPtr->size()) {
		ThingFilter* ff = filtersListPtr->at(currFilterIndex);
		if (ff) {
			int cnt = ff->rulesCount();
			return cnt;
		}
	}
	return 0;
}

int ThingRulesModel::columnCount(const QModelIndex &/*parent*/) const
{
	return columnNames.size();
}

QVariant ThingRulesModel::data(const QModelIndex &index, int role) const
{
	Role columnRole = roles[index.column()];
	if (role == Qt::TextAlignmentRole) {
		switch (columnRole) {
			case FieldRole:
				return (int)(Qt::AlignLeft | Qt::AlignVCenter);
			default:
				return (int)(Qt::AlignCenter | Qt::AlignVCenter);
		}
	} else if (role == Qt::DisplayRole) {
		int row = index.row();
		if (currFilterIndex >= 0 && currFilterIndex < filtersListPtr->size()) {
			ThingFilter* ff = filtersListPtr->at(currFilterIndex);
			if (ff) {
				if (row >= 0 && row < ff->rulesCount()) {
					const struct ThingFilter::thing_rule_ex* rule = ff->getRule(row);
					if (rule) {
						QString str1;
						ThingFilter::ParamRole nParam;
						ThingFilter::OperandRole nOper;
						ThingFilter::ActionRole nAction;
						switch (columnRole) {
						case FieldRole:
							nParam = rule->param;
							if (nParam == ThingFilter::NameRole) {
								return QString::fromUtf8("Имя вещи");
							} else if (nParam == ThingFilter::TypeRole) {
								return QString::fromUtf8("Тип вещи");
							} else if (nParam == ThingFilter::NamedRole) {
								return QString::fromUtf8("Уровень именной");
							} else if (nParam == ThingFilter::DressedRole) {
								return QString::fromUtf8("Одета");
							} else if (nParam == ThingFilter::PriceRole) {
								return QString::fromUtf8("Цена вещи");
							} else if (nParam == ThingFilter::CountRole) {
								return QString::fromUtf8("Количество");
							} else {
								return QString::fromUtf8("?");
							}
						case OperandRole:
							nOper = rule->operand;
							if (rule->param == ThingFilter::DressedRole)
								return "";
							if (rule->negative) {
								str1 = QString::fromUtf8("не ");
							} else {
								str1 = "";
							}
							if (nOper == ThingFilter::EqualRole) {
								str1.append(QString::fromUtf8("равно"));
							} else if (nOper == ThingFilter::ContainsRole) {
								str1.append(QString::fromUtf8("содержит"));
							} else if (nOper == ThingFilter::AboveRole) {
								str1.append(QString::fromUtf8("больше"));
							} else if (nOper == ThingFilter::LowRole) {
								str1.append(QString::fromUtf8("меньше"));
							}
							return str1;
						case ValueRole:
							if (rule->param == ThingFilter::DressedRole) {
								if (rule->negative) {
									return QString::fromUtf8("нет");
								} else {
									return QString::fromUtf8("да");
								}
							}
							return rule->value;
						case ActionRole:
							nAction = rule->action;
							if (nAction == ThingFilter::YesRole) {
								return QString::fromUtf8("отображать");
							} else if (nAction == ThingFilter::NoRole) {
								return QString::fromUtf8("не отображать");
							} else if (nAction == ThingFilter::NextRole) {
								return QString::fromUtf8("следующее");
							}
						}
					}
				}
			}
		}
	}
	return QVariant();
}

QVariant ThingRulesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (section >= 0 && section < columnNames.size()) {
			if (orientation == Qt::Horizontal) {
				return columnNames[section];
			}
		}
	}
	return QVariant();
}

bool ThingRulesModel::appendRule(ThingFilter::ParamRole param, bool negative, ThingFilter::OperandRole operand, const QString &value, ThingFilter::ActionRole action) {
	bool res = false;
	if (currFilterIndex >= 0 && currFilterIndex < filtersListPtr->size()) {
		ThingFilter* ff = filtersListPtr->at(currFilterIndex);
		if (ff) {
			int row = ff->rulesCount();
			beginInsertRows(QModelIndex(), row, row);
			res = ff->appendRule(param, negative, operand, value, action);
			endInsertRows();
		}
	}
	return res;
}

bool ThingRulesModel::modifyRule(int ruleIndex, const struct ThingFilter::thing_rule_ex* rule) {
	if (currFilterIndex >= 0 && currFilterIndex < filtersListPtr->size()) {
		ThingFilter* ff = filtersListPtr->at(currFilterIndex);
		if (ff) {
			ff->modifyRule(ruleIndex, rule);
			emit dataChanged(index(ruleIndex, 0), index(ruleIndex, columnNames.size()));
			return true;
		}
	}
	return false;
}

bool ThingRulesModel::setData(const QModelIndex &/*index*/, const QVariant &/*value*/, int /*role*/)
{
	return false;
}

bool ThingRulesModel::upRow(int row)
{
	if (currFilterIndex >= 0 && currFilterIndex < filtersListPtr->size()) {
		ThingFilter* ff = filtersListPtr->at(currFilterIndex);
		if (ff) {
			if (ff->moveRuleUp(row)) {
				emit dataChanged(index(row -1, 0), index(row, rowCount()));
				return true;
			}
		}
	}
	return false;
}

bool ThingRulesModel::downRow(int row)
{
	if (currFilterIndex >= 0 && currFilterIndex < filtersListPtr->size()) {
		ThingFilter* ff = filtersListPtr->at(currFilterIndex);
		if (ff) {
			if (ff->moveRuleDown(row)) {
				emit dataChanged(index(row, 0), index(row + 1, rowCount()));
				return true;
			}
		}
	}
	return false;
}

bool ThingRulesModel::removeRows(int row, int count, const QModelIndex &/*parent*/)
{
	if (count > 0) {
		if (currFilterIndex >= 0 && currFilterIndex < filtersListPtr->size()) {
			ThingFilter* ff = filtersListPtr->at(currFilterIndex);
			if (ff) {
				int rulesCnt = ff->rulesCount();
				if (row >= 0 && row < rulesCnt) {
					int endRow = row + count - 1;
					if (endRow >= rulesCnt)
						endRow = rulesCnt - 1;
					beginRemoveRows(QModelIndex(), row, endRow);
					while (row < ff->rulesCount()) {
						ff->removeRule(row);
						endRow--;
						if (endRow < row)
							break;
					}
					endRemoveRows();
					return true;
				}
			}
		}
	}
	return false;
}

Qt::ItemFlags ThingRulesModel::flags(const QModelIndex & /*index*/) const
{
	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return flags;
}
