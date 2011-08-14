/*
 * fingsview.cpp - Sof Game Psi plugin
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

//#include <QMenu>
//#include <QContextMenuEvent>
#include <QtGui/QHeaderView>

#include "thingsview.h"

FingsView::FingsView( QWidget * parent ) : QTableView(parent)
{

}

void FingsView::init()
{
	resizeColumnsToContents();
	resizeColumnsToContents();

	horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
	horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
	horizontalHeader()->setResizeMode(3, QHeaderView::ResizeToContents);
	//horizontalHeader()->setMinimumSectionSize(75);
	//horizontalHeader()->setCascadingSectionResizes(true);

	//horizontalHeader()->setStretchLastSection(true);

	horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);

	verticalHeader()->setDefaultAlignment( Qt::AlignHCenter );
	verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	//verticalHeader()->setMinimumSectionSize(10);

	connect(horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortByColumn(int)));
}

void FingsView::contextMenuEvent( QContextMenuEvent * e )
{
	Q_UNUSED(e)
/*	QMenu *popup = new QMenu(this);
	QList<QAction *> actions;
	actions <<new QAction(IconsetFactory::icon("psi/cm_check").icon(), tr("Check"), popup)
			<<new QAction(IconsetFactory::icon("psi/cm_uncheck").icon(), tr("Uncheck"), popup)
			<<new QAction(IconsetFactory::icon("psi/cm_invertcheck").icon(), tr("Invert"), popup);
	popup->addActions(actions);
	QAction *result = popup->exec(e->globalPos());
	int iresult;
	if (result) {
		iresult = actions.indexOf(result);
		const QVariant value(2);
		foreach(const QModelIndex &check, selectionModel()->selectedRows(0)) {
			switch (iresult) {
				case 0: //check
					model()->setData(check, 2);
					break;
				case 1: //uncheck
					model()->setData(check, 0);
					break;
				case 2: //invert
					model()->setData(check, 3);
					break;
			}
		}
	}
	delete popup;*/
}

void FingsView::keyPressEvent( QKeyEvent * e )
{
	Q_UNUSED(e)
/*	if (e->key() == Qt::Key_Space) {
		int data = 2; //check
		if (e->modifiers() & Qt::ControlModifier) {
			data = 3; //invert
		} else if (e->modifiers() & Qt::ShiftModifier) {
			data = 0; //uncheck
		}
		foreach(const QModelIndex &check, selectionModel()->selectedRows(0)) {
			model()->setData(check, data);
		}
		e->accept();
	} else {
		QTableView::keyPressEvent(e);
		e->ignore();
	}*/
}
