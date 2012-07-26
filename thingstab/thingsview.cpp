/*
 * thingsview.cpp - Sof Game Psi plugin
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
#include <QContextMenuEvent>
#include <QtGui/QHeaderView>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>

#include "thingsview.h"
#include "thingsmodel.h"
#include "pers.h"

ThingsView::ThingsView( QWidget * parent ) : QTableView(parent)
	, ifaceNum(-1)
{
	horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(horizontalHeader(), SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(headerContentMenu(const QPoint &)));
}

ThingsView::~ThingsView()
{
	Pers::instance()->removeThingsInterface(ifaceNum);
}

void ThingsView::init()
{
	ifaceNum = Pers::instance()->getThingsInterface();
	setModel(Pers::instance()->getThingsModel(ifaceNum));
	connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changeSummary()));
	connect(model(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SIGNAL(changeSummary()));
	connect(model(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SIGNAL(changeSummary()));

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
}

/**
 * Сохраняет настройки таблицы вещей в xml элемент
 */
QDomElement ThingsView::saveSettingsToXml(QDomDocument &xmlDoc) const
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
void ThingsView::loadSettingsFromXml(QDomElement &xml)
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

void ThingsView::contextMenuEvent(QContextMenuEvent *e)
{
	QMenu *menu = new QMenu();
	QAction *actSetPrice = new QAction(QString::fromUtf8("Цена у торговца"), menu);
	connect(actSetPrice, SIGNAL(triggered()), this, SLOT(setPrice()));
	QAction *actParamToConsole = new QAction(QString::fromUtf8("Сбросить параметры в консоль"), menu);
	connect(actParamToConsole, SIGNAL(triggered()), this, SLOT(paramToConsole()));
	QAction *actParamToClipboard = new QAction(QString::fromUtf8("Сбросить параметры в буфер обмена"), menu);
	connect(actParamToClipboard, SIGNAL(triggered()), this, SLOT(paramToClipboard()));

	if (currentIndex().row() >= 0) {
		actSetPrice->setEnabled(true);
		actParamToConsole->setEnabled(true);
		actParamToClipboard->setEnabled(true);
	} else {
		actSetPrice->setEnabled(false);
		actParamToConsole->setEnabled(false);
		actParamToClipboard->setEnabled(false);
	}
	menu->addAction(actSetPrice);
	menu->addAction(actParamToConsole);
	menu->addAction(actParamToClipboard);
	menu->exec(e->globalPos());
	delete menu;
}

void ThingsView::keyPressEvent(QKeyEvent */*e*/)
{
}

/**
 * Формирование меню для шапки таблицы
 */
void ThingsView::headerContentMenu(const QPoint &/*p*/)
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

void ThingsView::setFilter(int filterNum)
{
	Pers::instance()->setThingsInterfaceFilter(ifaceNum, filterNum);
	emit changeSummary();
}

void ThingsView::setPrice()
{
	Pers *pers = Pers::instance();
	// Получаем номер активной строки
	int row = currentIndex().row();
	if (row < 0)
		return;
	// Получаем указатель на вещь
	const Thing *thg = pers->getThingByRow(row, ifaceNum);
	if (!thg || !thg->isValid())
		return;
	QString s_name = QString::fromUtf8("Укажите цену для ") + thg->name() + QString::fromUtf8(", или -1 для сброса цены.");
	int price = thg->price();
	bool fOk = false;
	int new_price = QInputDialog::getInt(this, QString::fromUtf8("Новая цена вещи"), s_name, price, -1, 2147483647, 1, &fOk, 0);
	if (fOk && price != new_price) {
		pers->setThingPrice(ifaceNum, row, new_price);
		//emit changeSummary();
	}
}

void ThingsView::paramToConsole()
{
	int row = currentIndex().row();
	if (row < 0)
		return;
	Thing const *thg = Pers::instance()->getThingByRow(row, ifaceNum);
	if (thg && thg->isValid()) {
		emit writeToConsole(thg->toString(Thing::ShowAll), 3, true);
	}
}

void ThingsView::paramToClipboard()
{
	int row = currentIndex().row();
	if (row < 0)
		return;
	Thing const *thg = Pers::instance()->getThingByRow(row, ifaceNum);
	if (thg && thg->isValid()) {
		qApp->clipboard()->setText(thg->toString(Thing::ShowAll));
	}
}

int ThingsView::summaryCount()
{
	return Pers::instance()->getThingsCount(ifaceNum);
}

int ThingsView::summaryPriceAll()
{
	return Pers::instance()->getPriceAll(ifaceNum);
}

int ThingsView::summaryNoPriceCount()
{
	return Pers::instance()->getNoPriceCount(ifaceNum);
}
