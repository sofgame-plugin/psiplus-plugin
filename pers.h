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
#include "settings.h"
#include "maps/mappos.h"

#define THING_APPEND                 1

class Pers: public QObject
{
  Q_OBJECT

public:
	enum PersStatus {
		StatusNotKnow, StatusStand,
		StatusFightMultiSelect, StatusFightOpenBegin, StatusFightCloseBegin, StatusFightFinish,
		StatusMiniforum, StatusSecretBefore, StatusSecretGet, StatusThingsList, StatusPersInform,
		StatusFightShow, StatusOtherPersPos, StatusTakeBefore, StatusTake, StatusKillerAttack,
		StatusYard, StatusMasterRoom1, StatusMasterRoom2, StatusMasterRoom3, StatusDealerBuy,
		StatusDealerSale, StatusBuyOk, StatusHelpMenu, StatusTopList,
		StatusServerStatistic1, StatusServerStatistic2, StatusRealEstate, StatusWarehouse,
		StatusWarehouseShelf, StatusInKillersCup, StatusThingIsTaken, StatusAtHome
	};
	enum PersParams {
		ParamPersName,
		ParamMoneysCount,
		ParamPersLevel,
		ParamPersStatus,
		ParamExperienceCurr, ParamExperienceMax,
		ParamHealthCurr, ParamHealthMax,
		ParamEnergyCurr, ParamEnergyMax,
		ParamCoordinates
	};
	struct price_item {
		int      type;
		QString  name;
		int      price;
	};
	static Pers *instance();
	static void reset();
	void init();
	void setName(const QString &);
	const QString & name() const;
	int  moneysCount() const {return moneys;};
	void setMoneys(int);
	void setThingsStart(bool clear);
	void setThingsEnd();
	void setThingElement(int, Thing*);
	int  getThingsCount(int) const;
	int  getPriceAll(int) const;
	int  getNoPriceCount(int) const;
	const Thing* getThingByRow(int, int) const;
	void getThingsFiltersEx(QList<ThingFilter*>*) const;
	void setThingsFiltersEx(QList<ThingFilter*>);
	const QVector<price_item>* getThingsPrice() const;
	void backpackToXml(QDomElement &eBackpack) const;
	void loadThingsFromDomElement(QDomElement &);
	void setThingPrice(int, int, int);
	void beginSetPersParams();
	void setPersParams(int, int, int);
	void setPersParams(int, int, long long);
	void endSetPersParams();
	bool getIntParamValue(int, int*) const;
	bool getLongParamValue(int, long long*) const;
	bool getStringParamValue(PersParams, QString*) const;
	void setSetting(Settings::SettingKey, int);
	int  getThingsInterface();
	void setThingsInterfaceFilter(int, int);
	void removeThingsInterface(int);
	QSortFilterProxyModel* getThingsModel(int) const;
	QString getPersStatusString();
	const MapPos &getCoordinates() const {return position;};
	void setMapPosition(const MapPos &p);
	const MapPos &getMapPosition() const {return position;};
	QDomElement exportBackpackSettingsToDomElement(QDomDocument &xmlDoc) const;

private:
	QString pers_name;
	bool beginSetPersParamsFlag;
	int  persLevelValue; int persLevelValue_; bool setPersLevelValueFlag;
	long long  persExperienceMax; long long persExperienceMax_; bool setPersExperienceMaxFlag;
	long long  persExperienceCurr; long long persExperienceCurr_; bool setPersExperienceCurrFlag;
	PersStatus  persStatus; PersStatus  persStatus_; bool setPersStatusFlag;
	int  persHealthMax; int persHealthMax_; bool setPersHealthMaxFlag;
	int  persHealthCurr; int persHealthCurr_; bool setPersHealthCurrFlag;
	int  persEnergyMax; int persEnergyMax_; bool setPersEnergyMaxFlag;
	int  persEnergyCurr; int persEnergyCurr_; bool setPersEnergyCurrFlag;
	bool setPersLevelFlag;
	bool loadingThings;
	bool thingChanged;
	int  thingsPos;
	int  thingsSize;
	int  settingWatchRestHealthEnergy;
	int  watchHealthStartValue;
	int  watchHealthStartValue2;
	float watchHealthSpeed;
	int  watchHealthSpeedDelta;
	int  watchEnergyStartValue;
	int  watchEnergyStartValue2;
	float watchEnergySpeed;
	int  watchEnergySpeedDelta;
	QTimer *watchRestTimer;
	QTimer *watchHealthRestTimer;
	QTimer *watchEnergyRestTimer;
	QTime watchHealthStartTime;
	QTime watchHealthStartTime2;
	QTime watchEnergyStartTime;
	QTime watchEnergyStartTime2;
	ThingsModel* things;
	QList<ThingFilter*> thingFiltersEx;
	QHash<int, ThingsProxyModel*> thingModels;
	QVector<price_item> thingPrice;
	static Pers *instance_;
	static QHash<PersStatus, QString> statusStrings;
	MapPos position;
	QDateTime lastNegativeHealthUpdate;
	QDateTime lastNegativeEnergyUpdate;
	int moneys;

private:
	Pers(QObject *parent = 0);
	~Pers();
	void loadBackpackSettingsFromDomNode(const QDomElement &);
	void showRegenEvent(PersParams param);
	QDomElement exportThingsToDomElement(QDomDocument &xmlDoc) const;
	QDomElement exportPriceToDomElement(QDomDocument &xmlDoc) const;

private slots:
	void doWatchRestTime();
	void doWatchHealthRestTime();
	void doWatchEnergyRestTime();

signals:
	void thingsChanged();
	void filtersChanged();
	void persParamChanged(int, int, int);

};

#endif
