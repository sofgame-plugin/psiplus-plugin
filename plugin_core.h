/*
 * plugin_core.h - Sof Game Psi plugin
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

#ifndef PLUGIN_CORE_H
#define PLUGIN_CORE_H

//#include <QtCore>
#include <QDomDocument>
#include <QDateTime>
#include <QtDebug>

#define cVer "0.2.2"

#include "pers.h"
#include "pers_info.h"
#include "main_window.h"
#include "textparsing/gametext.h"

class PluginCore: public QObject
{
  Q_OBJECT

	public:
		static PluginCore *instance();
		static void reset();
		void doShortCut();
		void updateRegExpForPersName();
		void setAccountStatus(int status);
		void setGameJidStatus(int, qint32);
		bool getIntValue(int valueId, int* valuePtr);
		bool getLongValue(int valueId, long long *valuePtr) const;
		bool getTextValue(int valueId, QString* valuePtr);
		void resetStatistic(int valueId);
		bool sendCommandToCore(qint32 commandId);
		bool sendString(const QString &str);
		PersInfo* getPersInfo(const QString &);
		void initPopup(const QString &, int);

	private:
		static PluginCore *instance_;
		SofMainWindow* mainWindow;
		QString accJid;
		QString lastGameJid;
		QString lastChatJid;
		int statMessagesCount;
		int persStatus;
		int statMoneysDropCount;
		int statFightsCount;
		int statFightDamageMin;
		int statFightDamageMax;
		int statThingsDropCount;
		QString statThingDropLast;
		long long statExperienceDropCount;
		int statKilledEnemies;

		QRegExp mapCoordinatesExp;
		QRegExp parPersRegExp;
		QRegExp moneysCountExp;
		QRegExp fightDropMoneyReg2;
		QRegExp secretDropThingReg;
		QRegExp experienceDropReg;
		QRegExp experienceDropReg2;
		QRegExp secretBeforeReg;
		QRegExp secretBeforeReg2;
		QRegExp takeBeforeReg;
		QRegExp commandStrReg;
		QRegExp thingElementReg;
		QRegExp persInfoReg;
		QRegExp persInfoMainReg;
		QRegExp persInfoSitizenshipReg;
		QRegExp persInfoClanReg;
		QRegExp persInfoRatingReg;
		QRegExp persInfoParamsReg;
		QRegExp persInfoLossReg;
		QRegExp persInfoProtectReg;
		QRegExp persInfoAbility;
		QRegExp persInfoEquipReg;
		QRegExp fightShowReg;
		QRegExp otherPersPosReg1;
		QRegExp otherPersPosReg2;
		QRegExp killerAttackReg;
		QRegExp dealerBuyReg;
		QRegExp warehouseShelfReg;
		QRegExp persInListOfTheBestReg;
		QVector<PersInfo*> persInfoList;
		bool persStatusChangedFlag;
		bool persBackpackChangedFlag;
		bool statisticChangedFlag;
		QTimer saveStatusTimer;
		// --
		QRegExp fightOneTimeoutReg;
		QRegExp fightElement0Reg;
		QRegExp fightElement1Reg;
		QRegExp fightElement2Reg;
		QRegExp parPersPower1Reg;
		QRegExp fightDamageFromPersReg1;
		QRegExp fightDamageFromPersReg2;
		QRegExp fightDamageFromPersReg3;
		QRegExp fightDropMoneyReg1;
		QRegExp fightDropThingReg1;
		bool coloring;

	private:
		PluginCore();
		~PluginCore();
		void valueChanged(int valueId, int valueType, int value);
		void setGameText(const GameText &gameText, int type);
		void setConsoleText(const GameText &, int type, bool);
		bool savePersStatus();
		bool loadPersStatus();
		void getStatistics(const QString &commandPtr);
		void mapsCommands(const QStringList &args);
		void persCommands(const QStringList &args);
		void clearCommands(const QStringList &args);
		void thingsCommands(const QStringList &args);
		void aliasesCommands(const QStringList &);
		void settingsCommands(const QStringList &);
		void parseFightGroups(GameText &gameText);
		bool parseFightStepResult(GameText &gameText);
		void searchHorseshoe(const QString &);

	public slots:
		void changeAccountJid(const QString);
		bool textParsing(const QString jid, const QString message);
		void processError(int errorNum);
		void fightStarted(int mode);
		void persParamChanged();
		void persBackpackChanged();
		void statisticsChanged();
		void saveStatusTimeout();

	private slots:
		void updateSetting(Settings::SettingKey);

};

#endif
