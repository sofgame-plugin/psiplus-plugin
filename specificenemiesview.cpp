/*
 * specificenemiesview.cpp - Sof Game Psi plugin
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

#include <QHeaderView>

#include "specificenemiesview.h"
#include "settings.h"

SpecificEnemiesModel::SpecificEnemiesModel(QObject *parent) :
	QAbstractTableModel(parent)
{
	columnNames << QString::fromUtf8("Имя") << QString::fromUtf8("Не отмечать") << QString::fromUtf8("Сброс очереди");
	enemyList = Settings::instance()->getSpecificEnemies();
	enemyList.append(Settings::SpecificEnemy(QString(), false, false)); // Для ввода новой строки
}

void SpecificEnemiesModel::save()
{
	QList<Settings::SpecificEnemy> forSave;
	for (int i = 0, cnt = enemyList.size(); i < cnt; i++) {
		const Settings::SpecificEnemy *se = &enemyList.at(i);
		QString name = se->name;
		if (!name.isEmpty()) {
			forSave.append(Settings::SpecificEnemy(se->name, se->mapNotMark, se->resetQueue));
		}
	}
	Settings::instance()->setSpecificEnemies(forSave);
}

int SpecificEnemiesModel::rowCount(const QModelIndex &/*parent*/) const
{
	return enemyList.size();
}

int SpecificEnemiesModel::columnCount(const QModelIndex &/*parent*/) const
{
	return columnNames.size();
}

QVariant SpecificEnemiesModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid()) {
		const int row = index.row();
		if (row >=0 && row < enemyList.size()) {
			const int col = index.column();
			if (col == 0) {
				if (role == Qt::DisplayRole) {
					return enemyList.at(row).name;
				}
			} else {
				if (role == Qt::CheckStateRole) {
					if (col == 1) {
						return enemyList.at(row).mapNotMark ? Qt::Checked : Qt::Unchecked;
					}
					if (col == 2) {
						return enemyList.at(row).resetQueue ? Qt::Checked : Qt::Unchecked;
					}
				} else if (role == Qt::TextAlignmentRole) {
					return (int)(Qt::AlignRight | Qt::AlignVCenter);
				}
			}
		}
	}
	return QVariant();
}

QVariant SpecificEnemiesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		if (section >= 0 && section < columnNames.size()) {
			return columnNames.at(section);
		}
	}
	return QVariant();
}

Qt::ItemFlags SpecificEnemiesModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	const int col = index.column();
	if (col == 1 || col == 2) {
		flags |= Qt::ItemIsUserCheckable;
	} else {
		flags |= Qt::ItemIsEditable;
	}
	return flags;
}

bool SpecificEnemiesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid()) {
		const int row = index.row();
		const int col = index.column();
		if (row >= 0 && row < enemyList.size() && col >= 0 && col < columnNames.size()) {
			if (col == 0) {
				if (role == Qt::EditRole) {
					QString name = value.toString().trimmed();
					if (!name.isEmpty()) {
						// Проверяем противников с таким же имененм
						for (int i = 0, cnt = enemyList.size(); i < cnt; i++) {
							if (enemyList.at(i).name == name)
								return false;
						}
						enemyList[row].name = name;
						if (row + 1 == enemyList.size()) {
							const int newRow = row + 1;
							// Добавляем пустую строчку, т.к. заполнили последнюю строку
							beginInsertRows(QModelIndex(), newRow, newRow);
							enemyList.append(Settings::SpecificEnemy(QString(), false, false));
							endInsertRows();
						}
						emit dataChanged(index, index);
						return true;
					} else if (row + 1 < enemyList.size()) {
						// Не последняя строка и имя пустое. Удаляем.
						beginRemoveRows(QModelIndex(), row, row);
						enemyList.removeAt(row);
						endRemoveRows();
					}
				}
			} else if (role == Qt::CheckStateRole) {
				bool flag = (value.toInt() == Qt::Checked);
				if (col == 1) {
					enemyList[row].mapNotMark = flag;
				} else if (col == 2) {
					enemyList[row].resetQueue = flag;
				}
				return true;
			}
		}
	}
	return false;
}

//----------------------------------------------------------------------

SpecificEnemiesView::SpecificEnemiesView(QWidget *parent) :
	QTableView(parent),
	model_(NULL)
{
}

/**
 * init должен вызываться для каждого переключения аккаунта, так что старые данные нужно обновлять
 */
void SpecificEnemiesView::init()
{
	// Прописываем модель
	SpecificEnemiesModel *oldModel = model_;
	model_ = new SpecificEnemiesModel(this);
	setModel(model_);
	if (oldModel != NULL)
		delete oldModel;
	// Настройки поведения таблицы
	resizeColumnsToContents();
	horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
	horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
	verticalHeader()->setDefaultAlignment( Qt::AlignHCenter );
	verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
}

void SpecificEnemiesView::save()
{
	if (model_ != NULL)
		model_->save();
}
