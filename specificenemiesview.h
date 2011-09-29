/*
 * specificenemiesview.h - Sof Game Psi plugin
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef SPECIFICENEMIESMODEL_H
#define SPECIFICENEMIESMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

#include "settings.h"

class SpecificEnemiesModel : public QAbstractTableModel
{
Q_OBJECT
public:
	SpecificEnemiesModel(QObject *parent);
	void save();
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
	QStringList columnNames;
	QList<Settings::SpecificEnemy> enemyList;

};

#endif // SPECIFICENEMIESMODEL_H

//----------------------------------------------------------------------

#ifndef SPECIFICENEMIESVIEW_H
#define SPECIFICENEMIESVIEW_H

#include <QTableView>

class SpecificEnemiesView : public QTableView
{
Q_OBJECT
public:
	SpecificEnemiesView(QWidget *parent = 0);
	void init();
	void save();

private:
	SpecificEnemiesModel *model_;

private slots:
	void showContextMenu(QPoint pos);

};

#endif // SPECIFICENEMIESVIEW_H

