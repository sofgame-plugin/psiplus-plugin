/*
 * fingfmodel.h - Sof Game Psi plugin
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

#ifndef FINGFMODEL_H
#define FINGFMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

#include "../pers.h"

class FingFiltersModel : public QAbstractTableModel
{
	Q_OBJECT
	public:
		enum Role {
			// DisplayRole / EditRole
			StatusRole,
			NameRole,
			CountRole
		};
		FingFiltersModel(QObject* parent, QList<FingFilter*>*);
		~FingFiltersModel();
		void reloadFilters();
		FingFilter* getFilterByRow(int);
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex & parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		bool insertRows(int, int, const QModelIndex&  = QModelIndex());
		bool setData(const QModelIndex& idx, const QVariant& value, int role = Qt::EditRole);
		bool swapRows(int, int);
		bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
		void clear();
		Qt::ItemFlags flags(const QModelIndex & index) const;

	private:
		enum Status {
			Active,
			NoActive
		};
		struct filter_element {
			Status   status;
			QString  name;
			int      rulesCount;
		};
		QList<FingFilter*>* filtersListPtr;
		QStringList columnNames;
		QList<Role> roles;

	protected:

	private slots:
		void filtersChanged();

};

#endif // FINGSMODEL_H

//**********************************************************************************

#ifndef FINGFMODEL2_H
#define FINGFMODEL2_H

#include <QAbstractTableModel>
#include <QStringList>

#include "../pers.h"

class FingRulesModel : public QAbstractTableModel
{
	Q_OBJECT
	public:
		enum Role {
			FieldRole,
			OperandRole,
			ValueRole,
			ActionRole
		};
		FingRulesModel(QObject* parent, QList<FingFilter*>*);
		void reloadRules(int);
		const struct FingFilter::fing_rule_ex* getRule(int);
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex & parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		bool appendRule(FingFilter::ParamRole, bool, FingFilter::OperandRole, QString, FingFilter::ActionRole);
		bool modifyRule(int, const struct FingFilter::fing_rule_ex*);
		bool setData (const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
		bool upRow(int);
		bool downRow(int);
		bool removeRows(int, int, const QModelIndex& = QModelIndex());
		Qt::ItemFlags flags(const QModelIndex & index) const;


	private:
		//QList<struct fing_rule> rulesList;
		QList<FingFilter*>* filtersListPtr;
		int currFilterIndex;
		QStringList columnNames;
		QList<Role> roles;

	protected:

//	private slots:
//		void currFilterChanged(const QModelIndex&);

	public slots:
		//void doAction(QWidget*, int, int);
};

#endif // FINGSMODEL2_H
