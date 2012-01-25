/*
 * thingsmodel.cpp - Sof Game Psi plugin
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

#include "thingsmodel.h"
#include "../utils.h"
#include "../pers_info.h"

ThingsModel::ThingsModel(QObject* parent) :
	QAbstractTableModel(parent)
{
	columnsList[NumberRole] = QPair<QString, QString>("number", QString::fromUtf8("Ном."));
	columnsList[NameRole] = QPair<QString, QString>("name", QString::fromUtf8("Наименование"));
	columnsList[TypeRole] = QPair<QString, QString>("type", QString::fromUtf8("Тип"));
	columnsList[CountRole] = QPair<QString, QString>("count", QString::fromUtf8("Кол-во"));
	columnsList[PriceRole] = QPair<QString, QString>("price", QString::fromUtf8("Цена"));
}

ThingsModel::~ThingsModel()
{
	thingsList.clear();
}

Thing* ThingsModel::getThingByRow(int row)
{
	if (row >= 0 && row < thingsList.size()) {
		return thingsList.at(row);
	}
	return NULL;
}

int ThingsModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	return thingsList.size();
}

int ThingsModel::columnCount(const QModelIndex& /*parent*/) const
{
	return columnsList.size();
}

QVariant ThingsModel::data(const QModelIndex &index, int role) const
{
	//if (currFilterIndex >= 0) {
		int columnRole = index.column();
		if (role == Qt::TextAlignmentRole) {
			switch (columnRole) {
			case NumberRole:
			case TypeRole:
			case CountRole:
				return (int)(Qt::AlignCenter | Qt::AlignVCenter);
			case PriceRole:
				return (int)(Qt::AlignRight | Qt::AlignVCenter);
			default:
				return (int)(Qt::AlignLeft | Qt::AlignVCenter);
			}
		} else if (role == Qt::DisplayRole) {
			int row = index.row();
			if (row >= 0 && row < thingsList.size()) {
				Thing* thing = thingsList.at(row);
				if (thing) {
					QString str1;
					switch (columnRole) {
					case NumberRole:
						return thing->number();
					case NameRole:
						return thing->toString(Thing::ShowName);
					case TypeRole:
						return thing->toString(Thing::ShowType);
					case CountRole:
						return thing->count();
					case PriceRole:
						int num1 = thing->price();
						if (num1 == -1) {
							str1 = QString::fromUtf8("нет цены");
						} else {
							str1 = numToStr(num1, "'");
						}
						return str1;
					}
				}
			}
		} else if (role == Qt::TextColorRole) {
			int row = index.row();
			if (row >= 0 && row < thingsList.size()) {
				Thing* thing = thingsList.at(row);
				if (thing) {
					if (thing->isDressed()) {
						return Qt::blue;
					}
				}
			}
		} else if (role == Qt::ToolTipRole) {
			int row = index.row();
			if (row >= 0 && row < thingsList.size()) {
				Thing* thing = thingsList.at(row);
				if (thing) {
					return thing->toTip();
				}
			}
		}
	//}
	return QVariant();
}

QVariant ThingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal) {
		if (role == Qt::DisplayRole) {
			// Текст заголовка
			return columnsList.value(section, QPair<QString, QString>(QString(), QString())).second;
		} else if (role == Qt::UserRole) {
			// Id заголовка для настроек
			return columnsList.value(section, QPair<QString, QString>(QString(), QString())).first;
		}
	}
	return QVariant();
}

bool ThingsModel::insertThing(Thing* thing, int row)
{
	if (row < 0) return false;
	int cnt = thingsList.size();
	if (row > cnt) row = cnt;
	beginInsertRows(QModelIndex(), row, row);
	if (row < cnt) {
		thingsList.insert(row, thing);
	} else {
		thingsList.push_back(thing);
	}
	endInsertRows();
	return true;
}

//bool ThingsModel::setData(const QModelIndex& /*index*/, const QVariant& /*value*/, int /*role*/)
//{
//	return false;
//}

/**
 * Заменяет объект вещь другим. В том числе и тем же.
 * Если указатели идентичны, объект не удаляется.
 */
bool ThingsModel::setThing(Thing* thing, int row)
{
	int cnt = thingsList.size();
	if (row < 0 || row >= cnt) return false;
	Thing* old_thing = thingsList.at(row);
	thingsList[row] = thing;
	if (old_thing && old_thing != thing) {
		delete old_thing;
	}
	emit dataChanged(index(row, 0), index(row, columnCount() - 1));
	return true;
}

bool ThingsModel::removeRows(int row, int count, const QModelIndex& /*parent*/)
{
	if (count > 0) {
		int cntThings = thingsList.size();
		if (row >= 0 && row < cntThings) {
			int endRow = row + count - 1;
			if (endRow >= cntThings)
				endRow = cntThings - 1;
			beginRemoveRows(QModelIndex(), row, endRow);
			while (row < thingsList.size()) {
				Thing* thing = thingsList.takeAt(row);
				if (thing)
					delete thing;
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

void ThingsModel::clear()
{
	int cnt = thingsList.size();
	if (cnt > 0) {
		beginRemoveRows(QModelIndex(), 0, cnt - 1);
		while (!thingsList.isEmpty()) {
			delete thingsList.takeFirst();
		}
		endRemoveRows();
	}
}

Qt::ItemFlags ThingsModel::flags(const QModelIndex & index) const
{
	Q_UNUSED(index)

	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return flags;
}
