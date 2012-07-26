/*
 * thingsmodel.h - Sof Game Psi plugin
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

#ifndef THINGSMODEL_H
#define THINGSMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

#include "thing.h"

class ThingsModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum ColumnRole {
		NumberRole = 0,
		NameRole   = 1,
		TypeRole   = 2,
		CountRole  = 3,
		PriceRole  = 4
	};
	ThingsModel(QObject* parent);
	~ThingsModel();
	Thing* getThingByRow(int);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex & parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	bool insertThing(Thing*, int);
	//bool setData (const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
	bool setThing(Thing* thing, int row);
	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
	void clear();
	Qt::ItemFlags flags(const QModelIndex & index) const;


private:
	QList<Thing*> thingsList;
	QHash<int, QPair<QString, QString> > columnsList;

protected:

private slots:
	//void filtersChanged(); // ???? Отдавать на прокси !!!

};

#endif // THINGSMODEL_H
