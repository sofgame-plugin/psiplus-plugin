/*
 * pers.h - Sof Game Psi plugin
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

#ifndef PERS_H
#define PERS_H

#include <QtCore>
#include <QDomDocument>

#include "common.h"
#include "thingftab/thingfilter.h"
#include "thingstab/thingsmodel.h"
#include "thingstab/thingsproxymodel.h"

#define FING_APPEND                 1

class Pers: public QObject
{
  Q_OBJECT

	public:
		struct price_item {
			int      type;
			QString  name;
			int      price;
		};
		static Pers *instance();
		static void reset();
		void init();
		void setName(QString);
		QString name();
		void setFingsStart(bool clear);
		void setFingsEnd();
		void setFingElement(int, Thing*);
		int  getFingsCount(int);
		int  getPriceAll(int);
		int  getNoPriceCount(int);
		Thing* getFingByRow(int, int);
		void getFingsFiltersEx(QList<FingFilter*>*);
		void setFingsFiltersEx(QList<FingFilter*>);
		QVector<price_item>* getFingsPrice();
		void exportFingsToDomElement(QDomDocument*, QDomElement*);
		void exportBackpackSettingsToDomElement(QDomDocument*, QDomElement*);
		void loadFingsFromDomNode(QDomNode*);
		void loadBackpackSettingsFromDomNode(QDomNode*);
		void setFingPrice(int, int, int);
		void beginSetPersParams();
		void setPersParams(int, int, int);
		void endSetPersParams();
		bool getIntParamValue(int, int*);
		bool getStringParamValue(int, QString*);
		void setSetting(int, int);
		int  getThingsInterface();
		void setThingsInterfaceFilter(int, int);
		void removeThingsInterface(int);
		QSortFilterProxyModel* getThingsModel(int);

	private:
		QString pers_name;
		bool beginSetPersParamsFlag;
		int  persLevelValue; int persLevelValue_; bool setPersLevelValueFlag;
		PersStatus  persStatus; PersStatus  persStatus_; bool setPersStatusFlag;
		int  persHealthMax; int persHealthMax_; bool setPersHealthMaxFlag;
		int  persHealthCurr; int persHealthCurr_; bool setPersHealthCurrFlag;
		int  persEnergyMax; int persEnergyMax_; bool setPersEnergyMaxFlag;
		int  persEnergyCurr; int persEnergyCurr_; bool setPersEnergyCurrFlag;
		bool setPersLevelFlag;
		bool setPersExperienceMaxFlag;
		bool setPersExperienceCurrFlag;
		bool loadingFings;
		bool fingChanged;
		int  fingsPos;
		int  fingsSize;
		int  settingWatchRestHealthEnergy;
		int  watchHealthStartValue;
		int  watchHealthStartValue2;
		float watchHealthSpeed;
		int  watchHealthSpeedDelta;
		QTimer *watchRestTimer;
		QTimer *watchHealthRestTimer;

		QTime watchHealthStartTime;
		QTime watchHealthStartTime2;
		ThingsModel* things;
		QList<FingFilter*> fingFiltersEx;
		QHash<int, ThingsProxyModel*> thingModels;
		QVector<price_item> fingPrice;
		static Pers *instance_;

	private:
		Pers(QObject *parent = 0);
		~Pers();

	private slots:
		void doWatchRestTime();
		void doWatchHealthRestTime();

	signals:
		void fingsChanged();
		void filtersChanged();
		void persParamChanged(int, int, int);

};

#endif
