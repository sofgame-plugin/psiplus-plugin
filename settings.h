/*
 * settings.h - Sof Game Psi plugin
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QHash>
#include <QDomDocument>

class Settings : public QObject
{
Q_OBJECT
public:
	enum SettingKey {
		SettingPersName,
		SettingPersSaveMode,
		SettingMapsSaveMode,
		SettingSavePersParams,
		SettingSaveStatistic,
		SettingSaveBackpack,
		SettingMirrorSwitchMode,
		SettingWatchRestHealthEnergy,
		SettingInKillersCupPopup,
		SettingKillerAttackPopup,
		SettingShowQueryLength,
		SettingResetQueueForUnknowStatus,
		SettingResetQueuePopup,
		SettingServerTextBlocksCount,
		SettingFightTimerMode,
		SettingFightSelectAction,
		SettingFightAutoClose,
		SettingThingDropPopup,
		SettingRegenDurationForPopup,
		SettingGameTextColoring,
		SettingServerTimeout,
		SettingPersPosInCenter
	};
	struct SpecificEnemy {
		QString name;
		bool    mapNotMark;
		bool    resetQueue;
		SpecificEnemy(QString name_, bool notMark, bool reset) : name(name_), mapNotMark(notMark), resetQueue(reset) {};
	};
	static QStringList persSaveModeStrings; // !!! Из за настроек карт
	static Settings *instance();
	static void reset();
	void init(const QString &jid);
	int  getIntSetting(SettingKey key) const;
	void setIntSetting(SettingKey key, int param);
	bool getBoolSetting(SettingKey key) const;
	void setBoolSetting(SettingKey key, bool param);
	QString getStringSetting(SettingKey key) const;
	void setStringSetting(SettingKey key, const QString &param);
	const QDomElement & getSlotsData() const {return slotsSettingsElement;};
	void setSlotsData(const QDomElement xmlData) {slotsSettingsElement = xmlData;};
	const QDomElement & getAliasesData() const {return aliasesSettingsElement;};
	void setAliasesData(const QDomElement xmlData) {aliasesSettingsElement = xmlData;};
	const QDomElement & getBackpackData() const {return backpackSettingsElement;};
	void setBackpackData(const QDomElement xmlData) {backpackSettingsElement = xmlData;};
	const QDomElement & getAppearanceData() const {return appearanceSettingsElement;};
	void setAppearanceData(const QDomElement xmlData) {appearanceSettingsElement = xmlData;};
	const QDomElement & getMapsData() const {return mapsSettingsElement;};
	void setMapsData(const QDomElement xmlData) {mapsSettingsElement = xmlData;};
	bool save();
	const QList<struct SpecificEnemy> & getSpecificEnemies() const {return specificEnemies;};
	void setSpecificEnemies(const QList<struct SpecificEnemy> &enemies) {specificEnemies = enemies;};

private:
	static Settings *instance_;
	static QStringList mirrorChangeModeStrings;
	static QStringList watchRestHealthEnergyStrings;
	static QStringList fightTimerStrings;
	static QStringList fightSelectActionStrings;
	static QStringList fightAutoCloseStrings;
	QString jid_;
	QHash<SettingKey, int> defaultsListInt;
	QHash<SettingKey, bool> defaultsListBool;
	QHash<SettingKey, QString> defaultsListString;
	QHash<SettingKey, int> settingsListInt;
	QHash<SettingKey, bool> settingsListBool;
	QHash<SettingKey, QString> settingsListString;
	QDomElement slotsSettingsElement;
	QDomElement aliasesSettingsElement;
	QDomElement backpackSettingsElement;
	QDomElement appearanceSettingsElement;
	QDomElement mapsSettingsElement;
	QList<struct SpecificEnemy> specificEnemies;

private:
	Settings(QObject *parent = 0);
	~Settings();
	void load();
	void setMainSettings(const QDomElement &);
	void setFightSettings(const QDomElement &);

signals:
	void settingChanged(Settings::SettingKey);

};

#endif // SETTINGS_H
