/*
 * thingsproxymodel.h - Sof Game Psi plugin
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

#ifndef THINGSPROXYMODEL_H
#define THINGSPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "thing.h"
#include "thingsmodel.h"
#include "thingftab/thingfilter.h"

class ThingsProxyModel : public QSortFilterProxyModel
{
public:
	ThingsProxyModel(QObject *parent = 0);
	~ThingsProxyModel();
	// --
	void setThingsSource(ThingsModel*);
	const Thing* getThingByRow(int) const;
	void setFilter(ThingFilter const *);
	void setPrice(int /*row*/, int /*price*/);
	QVariant data(const QModelIndex &index, int role) const;
	QColor color(int row) const;

private:

private:
	ThingsModel* thingsSource;
	ThingFilter const *thingsFilter;

protected:
	virtual bool filterAcceptsColumn ( int source_column, const QModelIndex &source_parent ) const;
	virtual bool filterAcceptsRow ( int source_row, const QModelIndex &source_parent ) const;
	virtual bool lessThan ( const QModelIndex & left, const QModelIndex &right ) const;
};

#endif // THINGSPROXYMODEL_H
