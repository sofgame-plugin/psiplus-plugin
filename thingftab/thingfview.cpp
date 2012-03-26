/*
 * thingfview.cpp - Sof Game Psi plugin
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
#include <QMessageBox>
#include <QContextMenuEvent>
#include <QtGui/QHeaderView>

#include "thingfview.h"
#include "thingfilterdlg.h"
#include "thingruledlg.h"
#include "pluginhosts.h"

ThingFiltersView::ThingFiltersView(QWidget* parent) :
	QTableView(parent),
	thingFiltersTableModel(NULL)
{
}

void ThingFiltersView::init()
{
	//Создаем модель и связываем модель с представлением
	thingFiltersTableModel = new ThingFiltersModel(this);
	setModel(thingFiltersTableModel);
	thingFiltersTableModel->reloadFilters();
	// Настройки поведения таблицы
	resizeColumnsToContents();
	horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
	horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
	verticalHeader()->setDefaultAlignment( Qt::AlignHCenter );
	verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	// Соединения
	connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(currRowChanged(const QModelIndex&)));
	connect(this, SIGNAL(currFilterChanged(int)), thingFiltersTableModel, SLOT(changeCurrentRow(int)));
}

void ThingFiltersView::contextMenuEvent(QContextMenuEvent *e)
{
	QMenu* popup = new QMenu(this);
	QList<QAction *> actions;
	actions	<< new QAction(QString::fromUtf8("Добавить"), popup)
	<< new QAction(PluginHosts::psiIcon->getIcon("psi/action_templates_edit"), QString::fromUtf8("Редактировать"), popup)
	<< new QAction(PluginHosts::psiIcon->getIcon("psi/remove"), QString::fromUtf8("Удалить"), popup)
	<< new QAction(PluginHosts::psiIcon->getIcon("psi/arrowUp"), QString::fromUtf8("Выше"), popup)
	<< new QAction(PluginHosts::psiIcon->getIcon("psi/arrowDown"), QString::fromUtf8("Ниже"), popup);
	popup->addActions(actions);
	QAction *result = popup->exec(e->globalPos());
	int iresult;
	if (result) {
		iresult = actions.indexOf(result);
		int row = currentIndex().row();
//		if (iresult == 2)
//			emit currFilterChanged(0);
		switch (iresult) {
			case 0: // add
				if (true) {
					bool active = true;
					QString name = "";
					ThingFilterEditDialog* dlg = new ThingFilterEditDialog(this, &active, &name);
					if (dlg) {
						if (dlg->exec() == QDialog::Accepted) {
							int new_row = model()->rowCount();
							model()->insertRows(new_row, 1, QModelIndex());
							model()->setData(model()->index(new_row, 0), QVariant(active));
							model()->setData(model()->index(new_row, 1), QVariant(name));
							if (row >= 0 && row < model()->rowCount()) {
								selectionModel()->select(model()->index(row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
							}
							selectionModel()->select(model()->index(new_row, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
							emit currRowChanged(model()->index(new_row, 0));
						}
						delete dlg;
					}
					break;
				}
			case 1: // edit
				if (row >= 0 && row < model()->rowCount()) {
					bool active = model()->data(model()->index(row, 0), Qt::UserRole).toBool();
					QString name = model()->data(model()->index(row, 1), Qt::DisplayRole).toString();
					ThingFilterEditDialog* dlg = new ThingFilterEditDialog(this, &active, &name);
					if (dlg) {
						if (dlg->exec() == QDialog::Accepted) {
							model()->setData(model()->index(row, 0), QVariant(active));
							model()->setData(model()->index(row, 1), QVariant(name));
						}
						delete dlg;
					}
				}
				break;
			case 2: // delete
				if (row >= 0 && row < model()->rowCount()) {
					if (QMessageBox::question(this, QString::fromUtf8("Удаление фильтра"), QString::fromUtf8("Вы действительно хотите удалить фильтр?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
						model()->removeRows(row, 1, QModelIndex());
						int cnt = model()->rowCount();
						if (cnt > 0) {
							if (row >= cnt)
								row--;
							selectionModel()->select(model()->index(row, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
							emit currRowChanged(model()->index(row, 0));
						} else {
							emit currRowChanged(QModelIndex()); // Для отчистки строк после удаления последней записи
						}
					}
				}
				break;
			case 3: // up
				if (row > 0 && row < model()->rowCount()) {
					thingFiltersTableModel->swapRows(row - 1, row);
					selectionModel()->select(model()->index(row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
					selectionModel()->select(model()->index(row -1, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
					emit currRowChanged(model()->index(row - 1, 0));
				}
				break;
			case 4: // down
				if (row >= 0 && row < model()->rowCount() - 1) {
					thingFiltersTableModel->swapRows(row, row + 1);
					selectionModel()->select(model()->index(row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
					selectionModel()->select(model()->index(row + 1, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
					emit currRowChanged(model()->index(row + 1, 0));
				}
				break;
		}
	}
	delete popup;
}

void ThingFiltersView::keyPressEvent( QKeyEvent * e )
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

QList<ThingFilter*> ThingFiltersView::getFilters() const
{
	QList<ThingFilter*> resList;
	int i = 0;
	while (true) {
		ThingFilter* pf = thingFiltersTableModel->getFilterByRow(i++);
		if (!pf)
			break;
		resList.append(pf);
	}
	return resList;
}

void ThingFiltersView::currRowChanged(const QModelIndex& index)
{
	int row = -1;
	if (index.isValid())
		row = index.row();
	emit currFilterChanged(row);
}

//*********************************************************************************************

ThingRulesView::ThingRulesView(QWidget* parent) :
	QTableView(parent),
	thingRulesTableModel(NULL)
{
	setEnabled(false);
}

void ThingRulesView::init(ThingFiltersView *thfv)
{
	// Связываем модель с представлением
	ThingFiltersModel *parentModel = (ThingFiltersModel *)thfv->model();
	thingRulesTableModel = parentModel->getRulesModel();
	setModel(thingRulesTableModel);

	// Настройки поведения таблицы
	resizeColumnsToContents();
	horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
	horizontalHeader()->setResizeMode(2, QHeaderView::Stretch);
	horizontalHeader()->setResizeMode(3, QHeaderView::ResizeToContents);
	verticalHeader()->setDefaultAlignment( Qt::AlignHCenter );
	verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

	connect(thfv, SIGNAL(currFilterChanged(int)), this, SLOT(currFilterChanged(int)));
}

void ThingRulesView::contextMenuEvent(QContextMenuEvent *e)
{
	QMenu* popup = new QMenu(this);
	QList<QAction *> actions;
	actions	<<new QAction(QString::fromUtf8("Добавить"), popup)
			<<new QAction(PluginHosts::psiIcon->getIcon("psi/action_templates_edit"), QString::fromUtf8("Редактировать"), popup)
			<<new QAction(PluginHosts::psiIcon->getIcon("psi/remove"), QString::fromUtf8("Удалить"), popup)
			<<new QAction(PluginHosts::psiIcon->getIcon("psi/arrowUp"), QString::fromUtf8("Выше"), popup)
			<<new QAction(PluginHosts::psiIcon->getIcon("psi/arrowDown"), QString::fromUtf8("Ниже"), popup);
	popup->addActions(actions);
	QAction *result = popup->exec(e->globalPos());
	int iresult;
	if (result) {
		iresult = actions.indexOf(result);
		int row = currentIndex().row();
		switch (iresult) {
			case 0: // add
				if (true) {
					struct ThingFilter::thing_rule_ex new_rule;
					new_rule.param = ThingFilter::NoParamRole;
					new_rule.negative = false;
					new_rule.operand = ThingFilter::NoOperRole;
					new_rule.int_value = 0;
					new_rule.value = "";
					new_rule.action = ThingFilter::UnknowActionRole;
					ThingRuleEditDialog* dlg = new ThingRuleEditDialog(this, &new_rule);
					if (dlg) {
						if (dlg->exec() == QDialog::Accepted) {
							if (thingRulesTableModel->appendRule(new_rule.param, new_rule.negative, new_rule.operand, new_rule.value, new_rule.action, new_rule.color)) {
								if (row >= 0 && row < model()->rowCount()) {
									selectionModel()->select(model()->index(row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
								}
								selectionModel()->select(model()->index(model()->rowCount() - 1, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
							} else {
								QMessageBox::warning(this, QString::fromUtf8("Добавление правила"), QString::fromUtf8("Произошла ошибка при добавлении правила"), QMessageBox::Ok, QMessageBox::Ok);
							}
						}
						delete dlg;
					}
				}
				break;
			case 1: // edit
				if (row >= 0 && row < model()->rowCount()) {
					struct ThingFilter::thing_rule_ex rule;
					const struct ThingFilter::thing_rule_ex* curr_rule = thingRulesTableModel->getRule(row);
					if (curr_rule) {
						rule = *curr_rule;
						ThingRuleEditDialog* dlg = new ThingRuleEditDialog(this, &rule);
						if (dlg) {
							if (dlg->exec() == QDialog::Accepted) {
								thingRulesTableModel->modifyRule(row, &rule);
							}
							delete dlg;
						}
					}
				}
				break;
			case 2: // delete
				if (row >= 0 && row < model()->rowCount()) {
					if (QMessageBox::question(this, QString::fromUtf8("Удаление правила"), QString::fromUtf8("Вы действительно хотите удалить правило?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
						model()->removeRows(row, 1);
						int cnt = model()->rowCount();
						if (cnt > 0) {
							if (row >= cnt)
								row--;
							selectionModel()->select(model()->index(row, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
						}
					}
				}
				break;
			case 3: // up
				if (row > 0 && row < model()->rowCount()) {
					thingRulesTableModel->upRow(row);
					selectionModel()->select(model()->index(row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
					selectionModel()->select(model()->index(row - 1, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
				}
				break;
			case 4: // down
				if (row >= 0 && row < model()->rowCount() - 1) {
					thingRulesTableModel->downRow(row);
					selectionModel()->select(model()->index(row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
					selectionModel()->select(model()->index(row + 1, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
				}
				break;
		}
	}
	delete popup;
}

void ThingRulesView::keyPressEvent( QKeyEvent * /*e*/ )
{
/*if (e->key() == Qt::Key_Space) {
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

void ThingRulesView::currFilterChanged(int row)
{
	if (row < 0) {
		setEnabled(false);
	} else {
		setEnabled(true);
	}
	thingRulesTableModel->reloadRules(row);
}
