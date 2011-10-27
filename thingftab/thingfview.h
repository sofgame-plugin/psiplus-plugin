/*
 * thingfview.h - Sof Game Psi plugin
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

#ifndef THINGFVIEW_H
#define THINGFVIEW_H

#include <QTableView>

#include "thingfmodel.h"
#include "pers.h"

class ThingFiltersView : public QTableView
{
	Q_OBJECT

	public:
		//ThingFiltersView( QWidget*, QList<FingFilter*>*);
		ThingFiltersView(QWidget*);
		void init(QList<ThingFilter*>*);
		QList<ThingFilter*> getFilters() const;

	protected:
		ThingFiltersModel* thingFiltersTableModel;
		void contextMenuEvent(QContextMenuEvent* e);
		void keyPressEvent(QKeyEvent* e);

	private slots:
		void currRowChanged(const QModelIndex&);

	signals:
		void currFilterChanged(int);

};

#endif // THINGFVIEW_H

//*********************************************************************************************

#ifndef THINGFVIEW2_H
#define THINGFVIEW2_H

#include <QTableView>

class ThingRulesView : public QTableView
{
	Q_OBJECT
	public:
		ThingRulesView( QWidget*);
		void init(QList<ThingFilter*>*);

	protected:
		ThingRulesModel* thingRulesTableModel;
		void contextMenuEvent( QContextMenuEvent * e );
		void keyPressEvent( QKeyEvent * e );

	public slots:
		void currFilterChanged(int);

	signals:
		void actionEvent(QWidget*, int, int);

};

#endif // THINGFVIEW2_H
