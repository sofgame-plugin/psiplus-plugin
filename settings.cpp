/*
 * settings.cpp - Sof Game Psi plugin
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

#include "settings.h"
#include "utils.h"
#include "pers.h"
#include "aliases/aliases.h"
#include "maps/game_map.h"

QStringList Settings::persSaveModeStrings = (QStringList() << "none" << "save-at-exit" << "each-5-min-if-change");
QStringList Settings::mirrorChangeModeStrings = (QStringList() << "first-accessible" << "auto-select-optimal");
QStringList Settings::watchRestHealthEnergyStrings = (QStringList() << "no-watch" << "intell-watch" << "each-10-secs" << "each-30-secs" << "each-60-secs");
QStringList Settings::fightTimerStrings = (QStringList() << "not-to-display" << "seconds-only" << "minutes-and-seconds");
QStringList Settings::fightSelectActionStrings = (QStringList() << "no-action" << "new-if-queue-not-empty" << "always-new");
QStringList Settings::fightAutoCloseStrings = (QStringList() << "not-close" << "if-one-opposite-mob");

Settings::Settings(QObject *parent) :
	QObject(parent)
{
	defaultsListString[SettingPersName] = QString();
	defaultsListInt[SettingPersSaveMode] = 2;
	defaultsListInt[SettingMapsSaveMode] = 2;
	defaultsListBool[SettingSavePersParams] = true;
	defaultsListBool[SettingSaveBackpack] = true;
	defaultsListBool[SettingSaveStatistic] = true;
	defaultsListInt[SettingMirrorSwitchMode] = 1;
	defaultsListInt[SettingWatchRestHealthEnergy] = 1;
	defaultsListBool[SettingInKillersCupPopup] = true;
	defaultsListBool[SettingKillerAttackPopup] = true;
	defaultsListBool[SettingShowQueryLength] = false;
	defaultsListBool[SettingResetQueueForUnknowStatus] = true;
	defaultsListBool[SettingResetQueuePopup] = false;
	defaultsListInt[SettingServerTextBlocksCount] = 1000;
	defaultsListInt[SettingFightTimerMode] = 1;
	defaultsListInt[SettingFightSelectAction] = 0;
	defaultsListInt[SettingFightAutoClose] = 0;
	defaultsListBool[SettingThingDropPopup] = true;
	defaultsListInt[SettingRegenDurationForPopup] = 0;
	defaultsListBool[SettingGameTextColoring] = false;
	defaultsListInt[SettingServerTimeout] = 20;
}

Settings::~Settings()
{
}

Settings *Settings::instance_ = NULL;

Settings *Settings::instance()
{
	if (Settings::instance_ == NULL) {
		Settings::instance_ = new Settings();
	}
	return Settings::instance_;
}

void Settings::reset()
{
	if (Settings::instance_ != NULL) {
		delete Settings::instance_;
		Settings::instance_ = NULL;
	}
}


void Settings::init(const QString &jid)
{
	jid_ = jid;
	settingsListInt.clear();
	settingsListBool.clear();
	settingsListString.clear();
	slotsSettingsElement.clear();
	aliasesSettingsElement.clear();
	backpackSettingsElement.clear();
	appearanceSettingsElement.clear();
	mapsSettingsElement.clear();
	specificEnemies.clear();
	load();
}

int Settings::getIntSetting(SettingKey key) const
{
	if (settingsListInt.contains(key)) {
		return settingsListInt.value(key);
	}
	return defaultsListInt.value(key);
}

void Settings::setIntSetting(SettingKey key, int param)
{
	if (param != getIntSetting(key)) {
		settingsListInt[key] = param;
		emit settingChanged(key);
	}
}

bool Settings::getBoolSetting(SettingKey key) const
{
	if (settingsListBool.contains(key)) {
		return settingsListBool.value(key);
	}
	return defaultsListBool.value(key, false);
}

void Settings::setBoolSetting(SettingKey key, bool param)
{
	if (param != getBoolSetting(key)) {
		settingsListBool[key] = param;
		emit settingChanged(key);
	}
}

QString Settings::getStringSetting(SettingKey key) const
{
	if (settingsListString.contains(key)) {
		return settingsListString.value(key);
	}
	return defaultsListString.value(key);
}

void Settings::setStringSetting(SettingKey key, const QString &param)
{
	if (param != getStringSetting(key)) {
		settingsListString[key] = param;
		emit settingChanged(key);
	}
}

void Settings::load()
{
	if (jid_.isEmpty())
		return;
	QDomDocument xmlDoc;
	if (!loadPluginXml(&xmlDoc, "sofgame_settings.xml")) {
		return;
	}
	QDomElement eRoot = xmlDoc.documentElement();
	if (eRoot.tagName() != "settings") {
		return;
	}
	QDomElement eChildSettings = eRoot.firstChildElement("account");
	while (!eChildSettings.isNull()) {
		if (eChildSettings.attribute("jid") == jid_) {
			QDomElement eChildJid = eChildSettings.firstChildElement();
			while (!eChildJid.isNull()) {
				QString tagName = eChildJid.tagName();
				if (tagName == "main") {
					setMainSettings(eChildJid);
				} else if (tagName == "fight") {
					setFightSettings(eChildJid);
				} else if (tagName == "slots") {
					slotsSettingsElement = eChildJid;
				} else if (tagName == "aliases") {
					aliasesSettingsElement = eChildJid;
				} else if (tagName == "backpack") {
					backpackSettingsElement = eChildJid;
				} else if (tagName == "appearance") {
					appearanceSettingsElement = eChildJid;
				} else if (tagName == "maps") {
					mapsSettingsElement = eChildJid;
				}
				//
				eChildJid = eChildJid.nextSiblingElement();
			}
			break;
		}
		eChildSettings = eChildSettings.nextSiblingElement("account");
	}
}

void Settings::setMainSettings(const QDomElement &xml)
{
	QDomElement eChild = xml.firstChildElement();
	while (!eChild.isNull()) {
		QString tagName = eChild.tagName();
		if (tagName == "pers-name") {
			settingsListString[SettingPersName] = eChild.text();
		} else if (tagName == "pers-save-mode") {
			int i = persSaveModeStrings.indexOf(eChild.attribute("value"));
			if (i != -1) {
				Settings::settingsListInt[SettingPersSaveMode] = i;
			}
		} else if (tagName == "save-pers-params") {
			settingsListBool[SettingSavePersParams] = (eChild.attribute("value").toLower() != "false");
		} else if (tagName == "save-pers-backpack") {
			settingsListBool[SettingSaveBackpack] = (eChild.attribute("value").toLower() != "false");
		} else if (tagName == "save-statistic") {
			settingsListBool[SettingSaveStatistic] = (eChild.attribute("value").toLower() != "false");

		} else if (tagName == "mirror-change-mode") {
			int i = Settings::mirrorChangeModeStrings.indexOf(eChild.attribute("value"));
			if (i != -1) {
				settingsListInt[SettingMirrorSwitchMode] = i;
			}
		} else if (tagName == "watch-rest-health-energy") {
			int i = Settings::watchRestHealthEnergyStrings.indexOf(eChild.attribute("value"));
			if (i != -1) {
				settingsListInt[SettingWatchRestHealthEnergy] = i;
			}
		} else if (tagName == "finish-rest-popup") {
			bool fOk = false;
			int i = eChild.attribute("rest-duration").toInt(&fOk);
			if (fOk) {
				settingsListInt[SettingRegenDurationForPopup] = i;
			}
		} else if (tagName == "in-killers-cup-popup") {
			settingsListBool[SettingInKillersCupPopup] = (eChild.attribute("value").toLower() == "true");
		} else if (tagName == "killer-attack-popup") {
			settingsListBool[SettingKillerAttackPopup] = (eChild.attribute("value").toLower() == "true");
		} else if (tagName == "show-queue-length") {
			settingsListBool[SettingShowQueryLength] = (eChild.attribute("value").toLower() == "true");
		} else if (tagName == "reset-queue-if-unknow-status") {
			settingsListBool[SettingResetQueueForUnknowStatus] = (eChild.attribute("value").toLower() == "true");
		} else if (tagName == "show-popup-if-reset-queue") {
			settingsListBool[SettingResetQueuePopup] = (eChild.attribute("value").toLower() == "true");
		} else if (tagName == "server-text-max-blocks-count") {
			settingsListInt[SettingServerTextBlocksCount] = eChild.attribute("value").toInt();
		} else if (tagName == "game-text-coloring") {
			settingsListBool[SettingGameTextColoring] = (eChild.attribute("value").toLower() == "true");
		} else if (tagName == "server-timeout") {
			settingsListInt[SettingServerTimeout] = eChild.attribute("value").toInt();
		}
		eChild = eChild.nextSiblingElement();
	}
}

void Settings::setFightSettings(const QDomElement &xml)
{
	bool specEnemF = false;
	QDomElement eChild = xml.firstChildElement();
	while (!eChild.isNull()) {
		QString tagName = eChild.tagName();
		if (tagName == "fight-timer-show") {
			int i = Settings::fightTimerStrings.indexOf(eChild.attribute("value"));
			if (i != -1) {
				settingsListInt[SettingFightTimerMode] = i;
			}
		} else if (tagName == "fight-select-action") {
			int i = Settings::fightSelectActionStrings.indexOf(eChild.attribute("value"));
			if (i != -1) {
				settingsListInt[SettingFightSelectAction] = i;
			}
		} else if (tagName == "fight-auto-close") {
			int i = Settings::fightAutoCloseStrings.indexOf(eChild.attribute("value"));
			if (i != -1) {
				settingsListInt[SettingFightAutoClose] = i;
			}
		} else if (tagName == "fing-drop-popup") {
			settingsListBool[SettingThingDropPopup] = (eChild.attribute("value").toLower() == "true");
		} else if (tagName == "specific-enemies") {
			specEnemF = true;
			QDomElement eChild2 = eChild.firstChildElement("enemy");
			QHash<QString, int> names;
			while (!eChild2.isNull()) {
				const QString name = eChild2.attribute("name").trimmed();
				if (!name.isEmpty()) {
					if (!names.contains(name)) {
						names[name] = 1;
						bool notMark = (eChild2.attribute("not-mark-on-map").toLower() == "true");
						bool resetQueue = (eChild2.attribute("reset-queue").toLower() == "true");
						specificEnemies.append(SpecificEnemy(name, notMark, resetQueue));
					}
				}
				eChild2 = eChild2.nextSiblingElement("enemy");
			}
		}
		eChild = eChild.nextSiblingElement();
	}
	if (!specEnemF) {
		// Нет настроек для особых врагов. Заполняем по умолчанию
		specificEnemies.append(SpecificEnemy(QString::fromUtf8("Смертокрыл"), true, false));
		specificEnemies.append(SpecificEnemy(QString::fromUtf8("Ледяной дракон"), true, false));
	}
}

bool Settings::save()
{
	if (jid_.isEmpty())
		return false;
	// Читаем старые настройки
	QDomDocument xmlDoc;
	QDomElement eRoot;
	QDomElement eOldAccount;
	Pers *pers = Pers::instance();
	bool bReplace = false;
	if (loadPluginXml(&xmlDoc, "sofgame_settings.xml")) {
		// Ищем наш account элемент
		eRoot = xmlDoc.documentElement();
		if (eRoot.tagName() == "settings") {
			eOldAccount = eRoot.firstChildElement("account");
			while (!eOldAccount.isNull()) {
				if (eOldAccount.attribute("jid") == jid_) {
					bReplace = true;
					break;
				}
				eOldAccount = eOldAccount.nextSiblingElement("account");
			}
		}
	}
	// Создаем элемент нашего аккаунта
	QDomElement eNewAccount = xmlDoc.createElement("account");
	eNewAccount.setAttribute("jid", jid_);
	QDomElement eMain = xmlDoc.createElement("main");
	eNewAccount.appendChild(eMain);
	// Имя персонажа
	QString sName = settingsListString.value(SettingPersName);
	if (sName.isEmpty())
		sName = pers->name();
	QDomElement ePersName = xmlDoc.createElement("pers-name");
	if (!sName.isEmpty())
		ePersName.appendChild(xmlDoc.createTextNode(sName));
	eMain.appendChild(ePersName);
	//--
	int svMode = getIntSetting(SettingPersSaveMode);
	if (svMode >= 0 && svMode < Settings::persSaveModeStrings.size()) {
		QDomElement eSaveParamMode = xmlDoc.createElement("pers-save-mode");
		eSaveParamMode.setAttribute("value", Settings::persSaveModeStrings.at(svMode));
		eMain.appendChild(eSaveParamMode);
	}
	QDomElement eSaveParam = xmlDoc.createElement("save-pers-params");
	eSaveParam.setAttribute("value", getBoolSetting(SettingSavePersParams) ? "true" : "false");
	eMain.appendChild(eSaveParam);
	QDomElement eSaveBackpack = xmlDoc.createElement("save-pers-backpack");
	eSaveBackpack.setAttribute("value", getBoolSetting(SettingSaveBackpack) ? "true" : "false");
	eMain.appendChild(eSaveBackpack);
	QDomElement eSaveStat = xmlDoc.createElement("save-statistic");
	eSaveStat.setAttribute("value", getBoolSetting(SettingSaveStatistic) ? "true" : "false");
	eMain.appendChild(eSaveStat);
	int mirMode = getIntSetting(SettingMirrorSwitchMode);
	if (mirMode >= 0 && mirMode < Settings::mirrorChangeModeStrings.size()) {
		QDomElement eMirrorMode = xmlDoc.createElement("mirror-change-mode");
		eMirrorMode.setAttribute("value", Settings::mirrorChangeModeStrings.at(mirMode));
		eMain.appendChild(eMirrorMode);
	}
	int restMode = getIntSetting(SettingWatchRestHealthEnergy);
	if (restMode >= 0 && restMode < Settings::watchRestHealthEnergyStrings.size()) {
		QDomElement eWatchRest = xmlDoc.createElement("watch-rest-health-energy");
		eWatchRest.setAttribute("value", Settings::watchRestHealthEnergyStrings.at(restMode));
		eMain.appendChild(eWatchRest);
	}
	QDomElement eRestPopup = xmlDoc.createElement("finish-rest-popup");
	eRestPopup.setAttribute("rest-duration", getIntSetting(SettingRegenDurationForPopup));
	eMain.appendChild(eRestPopup);
	QDomElement eInKillersCupPopup = xmlDoc.createElement("in-killers-cup-popup");
	eInKillersCupPopup.setAttribute("value", getBoolSetting(SettingInKillersCupPopup) ? "true" : "false");
	eMain.appendChild(eInKillersCupPopup);
	QDomElement eKillerAttackPopup = xmlDoc.createElement("killer-attack-popup");
	eKillerAttackPopup.setAttribute("value", getBoolSetting(SettingKillerAttackPopup) ? "true" : "false");
	eMain.appendChild(eKillerAttackPopup);
	QDomElement eShowQueueLength = xmlDoc.createElement("show-queue-length");
	eShowQueueLength.setAttribute("value", getBoolSetting(SettingShowQueryLength) ? "true" : "false");
	eMain.appendChild(eShowQueueLength);
	QDomElement eResetQueueForUnknow = xmlDoc.createElement("reset-queue-if-unknow-status");
	eResetQueueForUnknow.setAttribute("value", getBoolSetting(SettingResetQueueForUnknowStatus) ? "true" : "false");
	eMain.appendChild(eResetQueueForUnknow);
	QDomElement eResetQueuePopup = xmlDoc.createElement("show-popup-if-reset-queue");
	eResetQueuePopup.setAttribute("value", getBoolSetting(SettingResetQueuePopup) ? "true" : "false");
	eMain.appendChild(eResetQueuePopup);
	int tbCount = getIntSetting(SettingServerTextBlocksCount);
	if (tbCount > 0) {
		QDomElement eServerTextBlocksCount = xmlDoc.createElement("server-text-max-blocks-count");
		eServerTextBlocksCount.setAttribute("value", tbCount);
		eMain.appendChild(eServerTextBlocksCount);
	}
	QDomElement eGameTextColoring = xmlDoc.createElement("game-text-coloring");
	eGameTextColoring.setAttribute("value", getBoolSetting(SettingGameTextColoring) ? "true" : "false");
	eMain.appendChild(eGameTextColoring);
	QDomElement eServerTimeout = xmlDoc.createElement("server-timeout");
	eServerTimeout.setAttribute("value", getIntSetting(SettingServerTimeout));
	eMain.appendChild(eServerTimeout);
	QDomElement eFight = xmlDoc.createElement("fight");
	eNewAccount.appendChild(eFight);
	int fTimer = getIntSetting(SettingFightTimerMode);
	if (fTimer >= 0 && fTimer < Settings::fightTimerStrings.size()) {
		QDomElement eFightTimer = xmlDoc.createElement("fight-timer-show");
		eFightTimer.setAttribute("value", Settings::fightTimerStrings.at(fTimer));
		eFight.appendChild(eFightTimer);
	}
	int fsAct = getIntSetting(SettingFightSelectAction);
	if (fsAct >= 0 && fsAct < Settings::fightSelectActionStrings.size()) {
		QDomElement eFightSelectAction = xmlDoc.createElement("fight-select-action");
		eFightSelectAction.setAttribute("value", Settings::fightSelectActionStrings.at(fsAct));
		eFight.appendChild(eFightSelectAction);
	}
	int faClose = getIntSetting(SettingFightAutoClose);
	if (faClose >= 0 && faClose < Settings::fightAutoCloseStrings.size()) {
		QDomElement eFightAutoClose = xmlDoc.createElement("fight-auto-close");
		eFightAutoClose.setAttribute("value", Settings::fightAutoCloseStrings.at(faClose));
		eFight.appendChild(eFightAutoClose);
	}
	QDomElement eThingDropPopup = xmlDoc.createElement("fing-drop-popup");
	eThingDropPopup.setAttribute("value", getBoolSetting(SettingThingDropPopup) ? "true" : "false");
	eFight.appendChild(eThingDropPopup);
	QDomElement eSpecificEnemies = xmlDoc.createElement("specific-enemies");
	foreach (SpecificEnemy se, specificEnemies) {
		QDomElement eEnemy = xmlDoc.createElement("enemy");
		eEnemy.setAttribute("name", se.name);
		if (se.mapNotMark)
			eEnemy.setAttribute("not-mark-on-map", "true");
		if (se.resetQueue)
			eEnemy.setAttribute("reset-queue", "true");
		eSpecificEnemies.appendChild(eEnemy);
	}
	eFight.appendChild(eSpecificEnemies);
	// Сохраняем настройки слотов
	if (!slotsSettingsElement.isNull()) {
		eNewAccount.appendChild(slotsSettingsElement);
	}
	// Сохраняем настройки внешнего вида (цвета, шрифты, размеры)
	if (!appearanceSettingsElement.isNull()) {
		eNewAccount.appendChild(appearanceSettingsElement);
	}
	// Сохраняем настройки алиасов
	aliasesSettingsElement = Aliases::instance()->saveToDomElement(xmlDoc);
	if (!aliasesSettingsElement.isNull()) {
		eNewAccount.appendChild(aliasesSettingsElement);
	}
	// Сохраняем настройки рюкзака
	backpackSettingsElement = Pers::instance()->exportBackpackSettingsToDomElement(xmlDoc);
	if (!backpackSettingsElement.isNull()) {
		eNewAccount.appendChild(backpackSettingsElement);
	}
	// Сохраняем настройки карт
	mapsSettingsElement = GameMap::instance()->exportMapsSettingsToDomElement(xmlDoc);
	if (!mapsSettingsElement.isNull()) {
		eNewAccount.appendChild(mapsSettingsElement);
	}
	// Сохранение в файл
	if (!bReplace) {
		// Если настройки не прочитались или они некорректные, то создаем новый xml документ
		if (eRoot.isNull()) {
			eRoot = xmlDoc.createElement("settings");
			eRoot.setAttribute("version", "0.1");
			xmlDoc.appendChild(eRoot);
		}
		eRoot.appendChild(eNewAccount);
	} else {
		// Заменяем старые настройки новыми
		eRoot.replaceChild(eNewAccount, eOldAccount);
	}
	return savePluginXml(&xmlDoc, "sofgame_settings.xml");
}
