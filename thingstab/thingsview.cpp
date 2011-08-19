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

#include <QMenu>
//#include <QContextMenuEvent>
#include <QtGui/QHeaderView>

#include "thingsview.h"
#include "thingsmodel.h"

FingsView::FingsView( QWidget * parent ) : QTableView(parent)
{
	connect(horizontalHeader(), SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(headerContentMenu(const QPoint &)));
}

FingsView::~FingsView()
{
}

void FingsView::init()
{
	resizeColumnsToContents();

	QHeaderView *hHeader = horizontalHeader();
	QHeaderView *vHeader = verticalHeader();
	hHeader->setResizeMode(ThingsModel::NumberRole, QHeaderView::ResizeToContents);
	hHeader->setResizeMode(ThingsModel::NameRole, QHeaderView::Stretch);
	hHeader->setResizeMode(ThingsModel::TypeRole, QHeaderView::ResizeToContents);
	hHeader->setResizeMode(ThingsModel::CountRole, QHeaderView::ResizeToContents);
	hHeader->setResizeMode(ThingsModel::PriceRole, QHeaderView::ResizeToContents);

	hHeader->setSortIndicator(-1, Qt::AscendingOrder);

	vHeader->setDefaultAlignment( Qt::AlignHCenter );
	vHeader->setResizeMode(QHeaderView::ResizeToContents);

	connect(hHeader, SIGNAL(sectionClicked(int)), this, SLOT(sortByColumn(int)));

	hHeader->setContextMenuPolicy(Qt::CustomContextMenu);
}

/**
 * Сохраняет настройки таблицы вещей в xml элемент
 */
QDomElement FingsView::saveSettingsToXml(QDomDocument &xmlDoc) const
{
	QDomElement eThingsTable = xmlDoc.createElement("things-table");
	QDomElement eColumns = xmlDoc.createElement("columns");
	eThingsTable.appendChild(eColumns);
	QHeaderView *header = horizontalHeader();
	QAbstractItemModel *model_ = model();
	for (int i = 0, cnt = header->count(); i < cnt; i++) {
		QDomElement eCol = xmlDoc.createElement("column");
		eColumns.appendChild(eCol);
		eCol.setAttribute("id", model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString());
		eCol.setAttribute("show", header->isSectionHidden(i) ? "false" : "true");
	}
	return eThingsTable;
}

/**
 * Загружает настройки таблицы из xml элемента
 * Если xml пустой, то применяются дефолтные настройки
 */
void FingsView::loadSettingsFromXml(QDomElement &xml)
{
	QHash<QString, bool>colSet;
	if (!xml.isNull()) {
		QDomElement eColumns = xml.firstChildElement("columns");
		if (!eColumns.isNull()) {
			// Последовательно читаем настройки колонок
			QDomElement eCol = eColumns.firstChildElement("column");
			while (!eCol.isNull()) {
				QString id = eCol.attribute("id");
				if (!id.isEmpty()) {
					colSet[id] = (eCol.attribute("show") == "true");
				}
				eCol = eCol.nextSiblingElement("column");
			}
		}
	}
	// Применяем найденные настройки
	QHeaderView *header = horizontalHeader();
	QAbstractItemModel *model_ = model();
	for (int i = 0, cnt = header->count(); i < cnt; i++) {
		QString id = model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString();
		if (colSet.contains(id)) {
			// Настройка для колонки найдена
			header->setSectionHidden(i, !colSet.value(id));
		} else {
			// Настройка для колонки не найдена
			// Для специальных столбцов по умолчению включено
			header->setSectionHidden(i, !(id == "number" || id == "name" || id == "type" || id == "count"));
		}
	}
}

void FingsView::contextMenuEvent(QContextMenuEvent */*e*/)
{
}

void FingsView::keyPressEvent(QKeyEvent */*e*/)
{
}

/**
 * Формирование меню для шапки таблицы
 */
void FingsView::headerContentMenu(const QPoint &/*p*/)
{
	QMenu *menu = new QMenu();
	QHeaderView *header = horizontalHeader();
	QAbstractItemModel *model_ = model();
	for (int i = 0, cnt = model_->columnCount(); i < cnt; i++) {
		if (i != ThingsModel::NameRole) { // Меню для всех столбцов кроме имени
			QString name = model_->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
			QAction *act = new QAction(name, menu);
			act->setCheckable(true);
			act->setChecked(!header->isSectionHidden(i));
			act->setData(i);
			menu->addAction(act);
		}
	}
	QAction *res = menu->exec(QCursor::pos());
	if (res != NULL) {
		int col = res->data().toInt();
		header->setSectionHidden(col, !res->isChecked());
	}
	delete menu;
}
