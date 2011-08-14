/*
 * thingsproxymodel.cpp - Sof Game Psi plugin
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


#include "thingsproxymodel.h"

ThingsProxyModel::ThingsProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
	thingsSource = NULL;
	thingsFilter = NULL;
	//setDynamicSortFilter(true);
}

ThingsProxyModel::~ThingsProxyModel()
{

}

bool ThingsProxyModel::filterAcceptsColumn ( int /*source_column*/, const QModelIndex & /*source_parent*/ ) const
{
	return true;
}

bool ThingsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &/*source_parent*/) const
{
	if (!thingsFilter)
		return true; // Фильтр не задан
	Thing* thg = thingsSource->getThingByRow(source_row);
	if (thg) {
		if (thingsFilter->isFingShow(thg)) {
			return true;
		}
	}
	return false;
}

bool ThingsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	Thing *left_thg = thingsSource->getThingByRow(left.row());
	Thing *right_thg = thingsSource->getThingByRow(right.row());
	ThingsModel::Role col_role = thingsSource->roles[left.column()];
	bool b_res = false;
	switch (col_role) {
		case ThingsModel::NumberRole:
			if (left_thg->number() < right_thg->number())
				b_res = true;
			break;
		case ThingsModel::CountRole:
			if (left_thg->count() < right_thg->count())
				b_res = true;
			break;
		case ThingsModel::PriceRole:
			if (left_thg->price() < right_thg->price())
				b_res = true;
			break;
		case ThingsModel::NameRole:
			if (left_thg->name().toLower() < right_thg->name().toLower())
				b_res = true;
			break;
	}
	return b_res;
}

void ThingsProxyModel::setFingsSource(ThingsModel* things_model) {
	thingsSource = things_model;
	if (thingsSource)
		setSourceModel(thingsSource);
}

Thing* ThingsProxyModel::getThingByRow(int row)
{
	if (thingsSource) {
		QModelIndex proxy_index = index(row, 1);
		if (proxy_index.isValid()) {
			QModelIndex source_index = mapToSource(proxy_index);
			if (source_index.isValid()) {
				return thingsSource->getThingByRow(source_index.row());
			}
		}
	}
	return NULL;
}

void ThingsProxyModel::setFilter(FingFilter* filter)
{
	thingsFilter = filter;
	invalidateFilter();
}

void ThingsProxyModel::setPrice(int row, int price)
{
	if (thingsSource) {
		QModelIndex proxy_index = index(row, 1);
		if (proxy_index.isValid()) {
			QModelIndex source_index = mapToSource(proxy_index);
			if (source_index.isValid()) {
				int s_row = source_index.row();
				Thing *thg = thingsSource->getThingByRow(s_row);
				if (thg) {
					thg->setPrice(price);
					thingsSource->setThing(thg, s_row);
				}
			}
		}
	}
}
