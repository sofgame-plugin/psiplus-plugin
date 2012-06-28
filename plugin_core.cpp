/*
 *
 * plugin_core.cpp - Sof Game Psi plugin
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

#include <QtCore>
#include <QDomDocument>
//#include <QDebug>

#include "common.h"
#include "plugin_core.h"
#include "sender.h"
#include "utils.h"
#include "maps/game_map.h"
#include "fight.h"
#include "textparsing/fightparse.h"
#include "aliases/aliases.h"
#include "settings.h"
#include "pluginhosts.h"
#include "statistic/statistic.h"

Q_DECLARE_METATYPE(Settings::SettingKey)

PluginCore *PluginCore::instance_ = NULL;

PluginCore *PluginCore::instance()
{
	if (PluginCore::instance_ == NULL) {
		PluginCore::instance_ = new PluginCore();
	}
	return PluginCore::instance_;
}

void PluginCore::reset()
{
	if (PluginCore::instance_ != NULL) {
		delete PluginCore::instance_;
		PluginCore::instance_ = NULL;
	}
}

PluginCore::PluginCore()
{
	// Начальная инициализация
	accJid = "";
	fight = new Fight();
	connect(fight, SIGNAL(fightStart(int)), this, SLOT(fightStarted(int)));
	// Сброс статусов изменений
	persStatusChangedFlag = false;
	persBackpackChangedFlag = false;
	statisticChangedFlag = false;
	// Настройка регулярок
	mapCoordinatesExp.setPattern(QString::fromUtf8("^(-?[0-9]+):(-?[0-9]+)$"));
	parPersRegExp.setPattern(QString::fromUtf8("^(\\w+)\\[у:([0-9]{1,2})\\|з:(-?[0-9]{1,6})/([0-9]{1,6})\\|э:(-?[0-9]{1,6})/([0-9]{1,6})\\|о:([0-9]{1,20})/(-?[0-9]{1,20})\\]$"));
	moneysCountExp.setPattern(QString::fromUtf8("^деньги:? (-?[0-9]+) дринк$")); //деньги: 100000 дринк
	fightDropMoneyReg2.setPattern(QString::fromUtf8("(нашли|выпало) ([0-9]+) дринк"));
	secretDropThingReg.setPattern(QString::fromUtf8("(забрали|нашли|выпала) \"(.+)\"")); // забрали "панцирь раритет" и ушли
	experienceDropReg.setCaseSensitivity(Qt::CaseInsensitive);
	experienceDropReg.setPattern(QString::fromUtf8("^Очки опыта:? \\+([0-9]+)$")); // очки опыта +1500 || Очки опыта: +50000
	experienceDropReg2.setPattern(QString::fromUtf8("^\\+([0-9]+) опыта\\.?")); // +4800 опыта за разбитый ящик.
	secretBeforeReg.setPattern(QString::fromUtf8("^[0-9]- обыскать( тайник| сокровищницу)?$"));  // 1- обыскать тайник
	secretBeforeReg2.setPattern(QString::fromUtf8("^[0-9]- разбить ящик$"));  // 1- разбить ящик
	takeBeforeReg.setPattern(QString::fromUtf8("^[0-9]- забрать$"));  // 1- забрать
	commandStrReg.setPattern(QString::fromUtf8("^([0-9])- (.+)$")); // Команды перемещения
	thingElementReg.setPattern(QString::fromUtf8("^([0-9]+)- (.+)\\((\\w+)\\)((.+)\\{(.+)\\})?(И:([0-9]+)ур\\.)?(- ([0-9]+)шт\\.)?$")); // Вещь
	persInfoReg.setPattern(QString::fromUtf8("^Характеристики (героя|героини): (\\w+)( \\{\\w+\\})?$"));
	persInfoMainReg.setPattern(QString::fromUtf8("^Уровень:([0-9]+), Здоровье:(-?[0-9]+)/([0-9]+), Энергия:(-?[0-9]+)/([0-9]+), Опыт:([0-9]+)(\\.|, Ост. до след уровня:([0-9]+))$")); // Уровень:25, Здоровье:6051/6051, Энергия:7956/7956, Опыт:186821489, Ост. до след уровня:133178511
	persInfoCitizenshipReg.setPattern(QString::fromUtf8("^Гражданство: (.+)$")); // Гражданство: город "Вольный"
	persInfoClanReg.setPattern(QString::fromUtf8("^Клан: (.+)$")); //  Клан: Лига Теней/ЛТ/
	persInfoRatingReg.setPattern(QString::fromUtf8("^Рейтинг: ([0-9]+) место.$")); // Рейтинг: 5 место.
	persInfoParamsReg.setPattern(QString::fromUtf8("^(\\w+):([0-9]+)\\[([0-9]+)\\]$")); // Сила:55[637] // Ловкость:44[517] // Интеллект:36[514]
	persInfoLossReg.setPattern(QString::fromUtf8("^Суммарный урон экипировки: ([0-9]+)$")); // Суммарный урон экипировки: 3452
	persInfoProtectReg.setPattern(QString::fromUtf8("^Суммарная защита экипировки: ([0-9]+)$")); // Суммарная защита экипировки: 3361
	persInfoAbility.setPattern(QString::fromUtf8("^(.+). ур.(.+)$")); // двойной удар. ур.:14;удар:35/55; защ.:16/18; подг.2хода; эн.36.
	persInfoEquipReg.setPattern(QString::fromUtf8("^(\\w+): (.+)$")); // Оружие: Когти Демона(кинжал)
	fightShowReg.setPattern(QString::fromUtf8("^Ход боя № [0-9]+$")); // Просмотр чужого боя
	otherPersPosReg1.setPattern(QString::fromUtf8("^(здесь никого нет\\.)|(всего:[0-9]+\\. \\(05 ник- посмотреть характеристики игрока\\))$")); // Первая строчка 05
	otherPersPosReg2.setPattern(QString::fromUtf8("^([^/]+)(/.+/(./)?)?\\[у:([0-9]+),з:(-?[0-9]+)/([0-9]+)\\]( Раст-е:[0-9]+ x:(-?[0-9]+) y:(-?[0-9]+))?$")); // Описание найденных игроков по 05
	killerAttackReg.setPattern(QString::fromUtf8("^Перед Вами возник туман, и за мгновение уплотнившись превратился в (\\w+), который не раздумывая кинулся в Вашу сторону...$")); // Нападение убийцы
	dealerBuyReg.setPattern(QString::fromUtf8("^в наличии:(-?[0-9]+) дринк$")); // Первая строка при покупке у торговца
	warehouseShelfReg.setPattern(QString::fromUtf8("^На (\\w+) полках Вы видите:$"));
	saveStatusTimer.setSingleShot(true);
	ratingInfoReg.setPattern(QString::fromUtf8("^Рейтинг до -?[1-9][0-9]* уровня:$")); // Рейтинг до 10 уровня:
	// --
	fightOneTimeoutReg.setPattern((QString::fromUtf8("^до завершения хода№([0-9]+): ([0-9]+)мин\\.([0-9]+)сек\\.$"))); // Номер хода и таймаут
	fightElement0Reg.setPattern(QString::fromUtf8("([^/]+)(/(.+)/)?\\[(у:([0-9]+)\\,з:)?([0-9]+)/([0-9]+)\\]")); // Боевые единицы (союзники)
	fightElement1Reg.setPattern(QString::fromUtf8("([0-9]+)- ([^/]+)(/(.+)/)?\\[(у:([0-9]+)\\,з:)?([0-9]+)/([0-9]+)\\]")); // Боевые единицы (противники)
	fightElement2Reg.setPattern(QString::fromUtf8("(.+)\\[(.+):(-?[0-9]+.*)\\]")); // Ауры в бою
	fightElement0Reg.setMinimal(true);
	fightElement1Reg.setMinimal(true);
	fightElement2Reg.setMinimal(true);
	parPersPower1Reg.setPattern(QString::fromUtf8("^Энергия:(-?[0-9]{1,6})/([0-9]{1,6})$"));
	// Соединения
	Settings *settings = Settings::instance();
	qRegisterMetaType<Settings::SettingKey>("Settings::SettingKey");
	connect(settings, SIGNAL(settingChanged(Settings::SettingKey)), this, SLOT(updateSetting(Settings::SettingKey)));
	Sender *sender = Sender::instance();
	connect(sender, SIGNAL(errorOccurred(int)), this, SLOT(processError(int)));
	connect(sender, SIGNAL(accountChanged(const QString)), this, SLOT(changeAccountJid(const QString)));
	Pers *pers = Pers::instance();
	connect(pers, SIGNAL(persParamChanged(int, int, int)), this, SLOT(persParamChanged()));
	connect(pers, SIGNAL(thingsChanged()), this, SLOT(persBackpackChanged()));
	connect(&saveStatusTimer, SIGNAL(timeout()), this, SLOT(saveStatusTimeout()));
	connect(Statistic::instance(), SIGNAL(valueChanged(int)), this, SLOT(statisticsChanged()));
	// Разное
	fightColoring = true;
}

PluginCore::~PluginCore()
{
	if (saveStatusTimer.isActive())
		saveStatusTimer.stop();
	while (!mainWindowList.isEmpty())
	{
		delete mainWindowList.takeLast();
	}
	GameMap::reset();
	if (fight) {
		delete fight;
	}
	savePersStatus();
	Pers::reset();
	Aliases::reset();
	Settings::reset();
	Statistic::reset();
	int cnt = persInfoList.size();
	for (int i = 0; i < cnt; i++) {
		PersInfo *pinf = persInfoList[i];
		if (pinf != NULL)
			delete pinf;
	}
}

void PluginCore::updateRegExpForPersName()
{
	// Функция настраивает регулярные выражения, зависящие от имени персонажа
	QString sName = QRegExp::escape(Pers::instance()->name());
	fightDamageFromPersReg1.setPattern("^" + sName + QString::fromUtf8(" .+ Повреждения: ([0-9]+)$"));
	fightDamageFromPersReg2.setPattern("^(.+, )?" + sName + QString::fromUtf8("(\\*([0-9]+))?,? .+/повр:([0-9]+)$"));
	fightDamageFromPersReg3.setPattern("^" + sName + QString::fromUtf8(" .+ нет повреждений$"));
	fightDropMoneyReg1.setPattern("^" + sName + QString::fromUtf8(" ([-+][0-9]+) дринк$")); // xxxxx +5 дринк;
	fightDropThingReg1.setPattern("^" + sName + QString::fromUtf8(" \\+(.+)$")); // xxxxx +шкура мат. волка
	persInListOfTheBestReg.setPattern("^ *[0-9]+\\. +" + sName + QString::fromUtf8("(/.+/)?\\[у:[0-9]+\\,з:[0-9]+/[0-9]+\\]")); //99. demon/В/XX/[у:27,з:xxxxx/xxxxxx] Опыт: xxxxxx сейчас в игре
}

void PluginCore::doShortCut()
{
	SofMainWindow *wnd;
	if (mainWindowList.isEmpty())
	{
		wnd = new SofMainWindow();
		mainWindowList.append(wnd);
	}
	else {
		wnd = mainWindowList.first();
	}
	wnd->show();
}

void PluginCore::changeAccountJid(const QString newJid)
{
	// *** В настройках плагина сменился игровой аккаунт ***
	// Сохраняем статус текущего персонажа
	savePersStatus();
	accJid = newJid;
	Settings *settings = Settings::instance();
	// Загрузить новые настройки
	settings->init(newJid);
	updateSetting(Settings::SettingGameTextColoring);
	// Настройки модуля отправки
	Sender *sender = Sender::instance();
	sender->setGameMirrorsMode(settings->getIntSetting(Settings::SettingMirrorSwitchMode));
	if (!sender->setServerTimeoutDuration(settings->getIntSetting(Settings::SettingServerTimeout))) {
		settings->setIntSetting(Settings::SettingServerTimeout, sender->getServerTimeoutDuration());
	}
	// Сбрасываем объект персонажа
	Pers *pers = Pers::instance();
	pers->init();
	// Сбрасываем модуль статистики
	Statistic::instance()->clear();
	// Загрузить новые данные персонажа
	loadPersStatus();
	// Обновить регулярки зависящие он имени персонажа
	updateRegExpForPersName();
	// Перегрузить карты
	GameMap::instance()->init(accJid);
	// Обновить алиасы
	Aliases::instance()->init();
	// Уведомить главное окно о переключении персонажа
	if (!mainWindowList.isEmpty())
		mainWindowList.first()->init();
	// Остальные окна закрыть
	while (mainWindowList.size() > 1)
		delete mainWindowList.takeLast();
}

void PluginCore::setAccountStatus(int status)
{
	// *** Смена статуса игрового аккаунта *** //  И как это радостное событие отловить, а?
	// Если аккаунт отключен, то устанавливаем игровым jid-ам статус отключенных
	if (status == 0) {
		setConsoleText(GameText(QString::fromUtf8("### Аккаунт отключен ###"), false), 3, false);
	} else {
		setConsoleText(GameText(QString::fromUtf8("### Аккаунт активен ###"), false), 3, false);
	}
	Sender::instance()->setAccountStatus(status);
}

void PluginCore::setGameJidStatus(int jid_index, qint32 status)
{
	Sender *sender = Sender::instance();
	if (sender->setGameJidStatus(jid_index, status)) {
		const Sender::jid_status* jstat = sender->getGameJidInfo(jid_index);
		QString str1 = "### " + jstat->jid + " - ";
		if (status == 0) {
			str1.append(QString::fromUtf8("отключен"));
		} else {
			str1.append(QString::fromUtf8("подключен"));
		}
		str1.append(" ###");
		setConsoleText(GameText(str1, false), 3, false);
	}
}

void PluginCore::doTextParsing(const QString &jid, const QString &message)
{
	bool myMessage = false; // Пока считаем что сообщение послано не нами
//	if (mySender->doGameAsk(&jid, &message)) { // Сначала сообщение обрабатывает Sender
//		return true;
//	}
	// Проверяем открыто ли окно
	foreach (SofMainWindow *wnd, mainWindowList)
	{
		if (wnd->isVisible())
		{
			myMessage = true;
			break;
		}
	}
	//--
	Statistic *stat = Statistic::instance();
	stat->setValue(Statistic::StatLastGameJid, jid);
	stat->setValue(Statistic::StatMessagesCount, stat->value(Statistic::StatMessagesCount).toInt() + 1);
	// Распускаем полученное сообщение построчно
	GameText gameText(message, false);
	// Просматриваем данные построчно
	Pers::PersStatus nPersStatus = Pers::StatusNotKnow;
	//int nParamLine = -1;
	bool fName = false; QString sName = 0;
	bool fLevel = false; int nLevel = 0;
	bool fExperience = false; long long nExperience = 0; long long nExperienceFull = 0;
	bool fHealth = false; int nHealthC = 0; int nHealthF = 0;
	bool fPower = false; int nPowerC = 0; int nPowerF = 0;
	bool fDropMoneys = false; int nDropMoneys = 0;
	int nThingsDropCount = 0; QString sThingDropLast;
	int nTimeout = -1;
	//bool fFight = false;
	int nCmdStatus = 0;
	//int nCount = aMessage.count();
	QString sMessage;
	bool fNewFight = false;
	// Проверяем наличие координат // 210:290
	MapPos persPos;
	Pers *pers = Pers::instance();
	if (!gameText.isEnd() && mapCoordinatesExp.indexIn(gameText.currentLine(), 0) != -1) {
		// Найдены координаты местоположения на карте
		persPos.setPos(mapCoordinatesExp.cap(1).toInt(), mapCoordinatesExp.cap(2).toInt());
		pers->setMapPosition(persPos);
		gameText.removeLine();
	}
	//--
	while (!gameText.isEnd()) {
		sMessage = gameText.currentLine().trimmed(); // Убираем пробельные символы в начале и в конце строки
		if (gameText.isFirst()) { // Первая строка в выборке
			if (parPersRegExp.indexIn(sMessage, 0) != -1) { // Проверяем наличие строки с полными параметрами персонажа
				fName = true; sName = parPersRegExp.cap(1);
				fLevel = true; nLevel = parPersRegExp.cap(2).toInt();
				fExperience = true; nExperience = parPersRegExp.cap(7).toLongLong(); nExperienceFull = parPersRegExp.cap(8).toLongLong() + nExperience;
				fHealth = true; nHealthC = parPersRegExp.cap(3).toInt(); nHealthF = parPersRegExp.cap(4).toInt();
				fPower = true; nPowerC = parPersRegExp.cap(5).toInt(); nPowerF = parPersRegExp.cap(6).toInt();
				//nParamLine = i; // Запомнили строку с полными параметрами персонажа
				nPersStatus = Pers::StatusStand;
			} else if (sMessage.startsWith(QString::fromUtf8("Бой открыт"))) {
				nPersStatus = Pers::StatusFightOpenBegin;
			} else if (sMessage.startsWith(QString::fromUtf8("Бой закрыт"))) {
				nPersStatus = Pers::StatusFightCloseBegin;
			} else if (persInfoReg.indexIn(sMessage, 0) != -1) {
				// Информация о персонаже
				QString sPersName = persInfoReg.cap(2).trimmed();
				int nLevel = -1;
				int nHealthCurr = QINT32_MIN; int nHealthMax = QINT32_MIN;
				int nEnergyCurr = QINT32_MIN; int nEnergyMax = QINT32_MIN;
				long long nExperienceCurr = -1; long long nExperienceRemain = -1; bool fExperienceRemain = false;
				int nPower1 = QINT32_MIN; int nPower2 = QINT32_MIN; bool fPower = false;
				int nDext1 = QINT32_MIN; int nDext2 = QINT32_MIN; bool fDext = false;
				int nIntell1 = QINT32_MIN; int nIntell2 = QINT32_MIN; bool fIntell = false;
				int nLoss = -1; int nProtect = -1;
				int nLossLine = -1; int nProtectLine = -1;
				QString sCitizenship;
				int nRating = QINT32_MIN;
				QString sClan;
				sMessage = gameText.nextLine().trimmed();
				if (persInfoMainReg.indexIn(sMessage, 0) != -1) {
					// Уровень:27, Здоровье:12464/12464, Энергия:11802/11802, Опыт:851081201.
					// Уровень:25, Здоровье:6051/6051, Энергия:7956/7956, Опыт:186821489, Ост. до след уровня:133178511
					nLevel = persInfoMainReg.cap(1).toInt();
					nHealthCurr = persInfoMainReg.cap(2).toInt();
					nHealthMax = persInfoMainReg.cap(3).toInt();
					nEnergyCurr = persInfoMainReg.cap(4).toInt();
					nEnergyMax = persInfoMainReg.cap(5).toInt();
					nExperienceCurr = persInfoMainReg.cap(6).toLongLong();
					if (!persInfoMainReg.cap(8).isEmpty()) {
						nExperienceRemain = persInfoMainReg.cap(8).toLongLong();
						fExperienceRemain = true;
					}
					sMessage = gameText.nextLine().trimmed();
				}
				if (persInfoCitizenshipReg.indexIn(sMessage, 0) != -1) {
					// Гражданство: город "Вольный"
					sCitizenship = persInfoCitizenshipReg.cap(1).trimmed();
					sMessage = gameText.nextLine().trimmed();
				}
				if (persInfoClanReg.indexIn(sMessage, 0) != -1) {
					//  Клан: Лига Теней/ЛТ/
					sClan = persInfoClanReg.cap(1).trimmed();
					sMessage = gameText.nextLine().trimmed();
				}
				if (persInfoRatingReg.indexIn(sMessage, 0) != -1) {
					// Рейтинг: 5 место.
					nRating = persInfoRatingReg.cap(1).toInt();
					sMessage = gameText.nextLine().trimmed();
				}
				if (sMessage == QString::fromUtf8("Распределение:")) {
					sMessage = gameText.nextLine().trimmed();
					while (persInfoParamsReg.indexIn(sMessage, 0) != -1) {
						// Сила:55[637]
						// Ловкость:44[517]
						// Интеллект:36[514]
						QString sParam = persInfoParamsReg.cap(1).trimmed();
						if (sParam == QString::fromUtf8("Сила")) {
							nPower1 = persInfoParamsReg.cap(2).toInt();
							nPower2 = persInfoParamsReg.cap(3).toInt();
							fPower = true;
						} else if (sParam == QString::fromUtf8("Ловкость")) {
							nDext1 = persInfoParamsReg.cap(2).toInt();
							nDext2 = persInfoParamsReg.cap(3).toInt();
							fDext = true;
						} else if (sParam == QString::fromUtf8("Интеллект")) {
							nIntell1 = persInfoParamsReg.cap(2).toInt();
							nIntell2 = persInfoParamsReg.cap(3).toInt();
							fIntell = true;
						}
						sMessage = gameText.nextLine().trimmed();
					}
					if (persInfoLossReg.indexIn(sMessage, 0) != -1) {
						// Суммарный урон экипировки: 3452
						nLoss = persInfoLossReg.cap(1).toInt();
						nLossLine = gameText.currentPos();
						sMessage = gameText.nextLine().trimmed();
					}
					if (persInfoProtectReg.indexIn(sMessage, 0) != -1) {
						// Суммарная защита экипировки: 3361
						nProtect = persInfoProtectReg.cap(1).toInt();
						nProtectLine = gameText.currentPos();
						sMessage = gameText.nextLine().trimmed();
					}
				}
				if (sMessage == QString::fromUtf8("Умения:")) {
					sMessage = gameText.nextLine().trimmed();
					// простой удар. ур.1; поражение:2; эн.4.
					// двойной удар. ур.:14;удар:35/55; защ.:16/18; подг.2хода; эн.36.
					// аура ярости. ур.:7;урон команды:+52;удар:8; защ.:8; подг.4хода; эн.109.
					// аура защитника. ур.:4;защита команды:+45;удар:7; защ.:7; подг.5хода; эн.104.
					while (persInfoAbility.indexIn(sMessage, 0) != -1) {

						sMessage = gameText.nextLine().trimmed();
					}
					// деньги: 100000 дринк
					if (moneysCountExp.indexIn(sMessage, 0) != -1) {
						Pers::instance()->setMoneys(moneysCountExp.cap(1).toInt());
						sMessage = gameText.nextLine().trimmed();
					}
				}
				// Смотрим экипу
				/*
				Оружие: Когти Демона(кинжал) урон:26.8*ур;сила:2.3*ур;ловк:1.7*ур;инт:1.9*ур;защ:3.6*ур;{Треб:Ур7Сил20Ловк15Инт10}И:8ур.
				Щит: Призрачный щит Демона(щит) защ:18.5*ур;сила:0.9*ур;ловк:1.4*ур;инт:1.2*ур;урон:12.1*ур;{Треб:Ур7Сил20Ловк15Инт10}И:8ур.
				Голова: Дыхание Демона(шлем) защ:6.2*ур;инт:1.8*ур;урон:9.7*ур;сила:0.8*ур;ловк:0.8*ур;{Треб:Ур17Сил71Ловк48Инт42}И:7ур.
				Шея: Обруч ярости Демона(амулет) сила:1.8*ур;ловк:4.2*ур;инт:1.8*ур;урон:9.5*ур;защ:6.2*ур;{Треб:Ур13Сил39}И:7ур.
				Руки: Призрачные наручи Демона(браслет) урон:6.1*ур;защ:8.2*ур;сила:1.1*ур;ловк:1.4*ур;инт:0.8*ур;{Треб:Ур7Сил20Ловк15Инт10}И:6ур.
				Руки: Аура повиновения Демона(браслет) сила:9.7*ур;ловк:3.8*ур;инт:3.8*ур;урон:9.2*ур;защ:6.5*ур;{Треб:Ур1Сил1Ловк20Инт10}И:7ур.
				Плечи: Железные наплечники Демона(наплечники) сила:1.1*ур;ловк:0.9*ур;инт:0.9*ур;защ:4.0*ур;урон:3.5*ур;{Треб:Ур14Сил27Ловк23Инт30}И:6ур.
				Корпус: Легкая чешуйчатая броня Демона(броня) защ:11.6*ур;сила:1.0*ур;ловк:1.1*ур;инт:0.9*ур;урон:2.6*ур;{Треб:Ур7Сил20Ловк15Инт10}И:7ур.
				Пояс: Огненный пояс Демона(пояс) защ:8.9*ур;сила:0.9*ур;ловк:1.8*ур;инт:0.6*ур;урон:3.8*ур;{Треб:Ур12Сил25Ловк25Инт20}И:7ур.
				Ноги: Хвост Демона(штаны) защ:9.3*ур;сила:1.7*ур;инт:1.1*ур;урон:7.4*ур;ловк:0.8*ур;{Треб:Ур12}И:7ур.
				Обувь: Боевые сапоги Демона(обувь) защ:11.6*ур;сила:0.9*ур;ловк:1.1*ур;инт:0.9*ур;урон:6.6*ур;{Треб:Ур18Сил77Ловк54Инт48}И:7ур.
				*/
				int equipCount = 0;
				QVector<equip_element> equipList(11);
				for (int idx = 0; idx < 11; idx++)
					resetEquip(&equipList[idx]);
				int hand = 1;
				while (persInfoEquipReg.indexIn(sMessage, 0) != -1) {
					struct equip_element* ee;
					bool fEe = false;
					QString sEquipType = persInfoEquipReg.cap(1).trimmed();
					if (sEquipType == QString::fromUtf8("Оружие")) {
						ee = &equipList[0];
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Щит")) {
						ee = &equipList[1];
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Голова")) {
						ee = &equipList[2];
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Шея")) {
						ee = &equipList[3];
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Руки")) {
						if (hand == 1) {
							hand++;
							ee = &equipList[4];
						} else {
							ee = &equipList[5];
						}
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Плечи")) {
						ee = &equipList[6];
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Корпус")) {
						ee = &equipList[7];
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Пояс")) {
						ee = &equipList[8];
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Ноги")) {
						ee = &equipList[9];
						fEe = true;
					} else if (sEquipType == QString::fromUtf8("Обувь")) {
						ee = &equipList[10];
						fEe = true;
					}
					if (fEe) {
						getEquipFromString(persInfoEquipReg.cap(2).trimmed(), ee);
					}

					sMessage = gameText.nextLine().trimmed();
					equipCount++;
				}
				if (sMessage == "---") {
					/*
					---
					Комплект Демона
					Когти Демона; Легкая чешуйчатая броня Демона; Железные наплечники Демона; Хвост Демона; Боевые сапоги Демона; Огненный пояс Демона; Обруч ярости Демона; Аура повиновения Демона; Дыхание Демона;
					урон:191; защ:82; сила:9; защ:78; защ:63; инт:9; инт:30; урон:35; инт:49;
					Призрачный комплект Демона
					Призрачный щит Демона; Призрачные наручи Демона;
					сила:19; сила:3;
					*/
					sMessage = gameText.nextLine().trimmed();
					if (equipCount != 0 && sMessage != QString::fromUtf8("Кредиты:")) {
						// Проверяем комплекты
						while (true) {
							if (sMessage == "---")
								break;
							// Сначала предположительно имя комплекта
							sMessage = gameText.nextLine().trimmed();
							// Список вещей в комплекте
							QStringList equipNameList = sMessage.split(";", QString::SkipEmptyParts, Qt::CaseInsensitive);
							int cnt = equipNameList.size();
							if (cnt < 2) {
								gameText.prior();
								break;
							}
							sMessage = gameText.nextLine().trimmed();
							// Список бонусов
							QStringList equipBonusList = sMessage.split(";", QString::SkipEmptyParts);
							if (equipBonusList.size() != cnt) {
								gameText.prior();
								gameText.prior();
								break;
							}
							sMessage = gameText.nextLine().trimmed();
							// Просматриваем экипировку, прописываем бонусы
							for (int setIndex = 0; setIndex < cnt; setIndex++) {
								QString equipName = equipNameList[setIndex].trimmed();
								for (int equipIndex = 0; equipIndex < 11; equipIndex++) {
									if (equipList[equipIndex].name == equipName) {
										QStringList bonusSplit = equipBonusList[setIndex].split(":");
										if (bonusSplit.size() == 2) {
											QString sBonus = bonusSplit[0].trimmed();
											if (sBonus == QString::fromUtf8("урон")) {
												equipList[equipIndex].loss_set += bonusSplit[1].toInt();
											} else if (sBonus == QString::fromUtf8("защ")) {
												equipList[equipIndex].protect_set += bonusSplit[1].toInt();
											} else if (sBonus == QString::fromUtf8("сила")) {
												equipList[equipIndex].force_set += bonusSplit[1].toInt();
											} else if (sBonus == QString::fromUtf8("ловк")) {
												equipList[equipIndex].dext_set += bonusSplit[1].toInt();
											} else if (sBonus == QString::fromUtf8("инт")) {
												equipList[equipIndex].intell_set += bonusSplit[1].toInt();
											}
										}
										break;
									}
								}
							}
						}
					} else {
						gameText.prior();
					}
				}
				/*
				---
				Татуировки:
				 "Кусочек солнца" /спина/ |35%рег.|
				---
				3- посмотреть битву
				4- недвижимость
				0- в игру.
				3- история сделок
				*/
				Pers *pers = Pers::instance();
				// Если это наш персонаж, обновляем данные
				if (pers->name() == sPersName)
				{
					pers->setCitizenship(sCitizenship);
				}
				// Ищем сохраненную информацию об персе
				PersInfo* persInfoPtr = getPersInfo(sPersName);
				if (!persInfoPtr) {
					persInfoPtr = new PersInfo;
					if (persInfoPtr)
						persInfoList.push_back(persInfoPtr);
				}
				if (persInfoPtr) {
					persInfoPtr->setName(sPersName);
					persInfoPtr->setLevel(nLevel);
					persInfoPtr->setCitizenship(sCitizenship);
					persInfoPtr->setClan(sClan);
					persInfoPtr->setRating(nRating);
					persInfoPtr->setHealthCurr(nHealthCurr);
					persInfoPtr->setHealthMax(nHealthMax);
					persInfoPtr->setEnergyCurr(nEnergyCurr);
					persInfoPtr->setEnergyMax(nEnergyMax);
					persInfoPtr->setExperienceCurr(nExperienceCurr);
					if (fExperienceRemain)
						persInfoPtr->setExperienceRemain(nExperienceRemain);
					if (fPower) {
						persInfoPtr->setForce(nPower1, nPower2);
					}
					if (fDext) {
						persInfoPtr->setDext(nDext1, nDext2);
					}
					if (fIntell) {
						persInfoPtr->setIntell(nIntell1, nIntell2);
					}
					persInfoPtr->setEquipLoss(nLoss);
					persInfoPtr->setEquipProtect(nProtect);
					for (int idx = 0; idx < 11; idx++)
						persInfoPtr->setEquip(idx+1, &equipList[idx]);
				}
				// Дописываем заточку в текст
				if (!persInfoPtr->citizenship().isEmpty())
				{
					if (nLoss != -1)
					{
						int nEquipLoss = -1;
						persInfoPtr->getEquipLoss1(&nEquipLoss);
						float nSharpening = (float)nLoss / (float)nEquipLoss;
						QString shpText;
						if (nSharpening > 1.01f)
						{
							shpText = QString::fromUtf8(" [заточка %1%]").arg(floor((nSharpening - 1.0f + 0.01f) * 100));
						}
						else {
							shpText = QString::fromUtf8(" [без заточки]").arg(nSharpening);
						}
						gameText.replace(nLossLine, gameText.getLine(nLossLine) + shpText, false);
					}
					if (nProtect != -1)
					{
						int nEquipProtect = -1;
						persInfoPtr->getEquipProtect1(&nEquipProtect);
						float nSharpening = (float)nProtect / (float)nEquipProtect;
						QString shpText;
						if (nSharpening > 1.01f)
						{
							shpText = QString::fromUtf8(" [заточка %1%]").arg(floor((nSharpening - 1.0f + 0.01f) * 100));
						}
						else {
							shpText = QString::fromUtf8(" [без заточки]");
						}
						gameText.replace(nProtectLine, gameText.getLine(nProtectLine) + shpText, false);
					}
				}
				// Блокируем дальнейший анализ
				gameText.setEnd();
				nPersStatus = Pers::StatusPersInform;
			} else if (sMessage == QString::fromUtf8("Экипировка:")) {
				// Список вещей персонажа
				// Деньги
				sMessage = gameText.nextLine().trimmed();
				if (moneysCountExp.indexIn(sMessage, 0) != -1) {
					pers->setMoneys(moneysCountExp.cap(1).toInt());
				}
				pers->setThingsStart(true);
				bool fDressed = false;
				while (!gameText.isEnd()) {
					sMessage = gameText.nextLine().trimmed();
					if (sMessage == QString::fromUtf8("одеты:")) {
						fDressed = true;
					} else if (sMessage == QString::fromUtf8("не одеты:")) {
						fDressed = false;
					} else {
						Thing* fg = new Thing(sMessage);
						if (fg->isValid()) {
							fg->setDressed(fDressed);
							pers->setThingElement(THING_APPEND, fg);
						} else {
							delete fg;
						}
					}
				}
				pers->setThingsEnd();
				nPersStatus = Pers::StatusThingsList;
				int fltrNum = pers->thingsSpecFiltersList().indexByName("02");
				if (pers->thingsSpecFiltersList().isActive(fltrNum)) {
					gameText = GameText(QString::fromUtf8("Экипировка:"), false);
					gameText.append(QString::fromUtf8("Деньги: %1 дринк")
							.arg(numToStr(pers->moneysCount(), "'"))
							, false);
					int nDressed = 0;
					int iface = pers->getThingsInterface();
					pers->setThingsInterfaceFilter(iface, fltrNum, true);
					int i = 0;
					while (true) {
						const Thing *th = pers->getThingByRow(i, iface);
						if (!th)
							break;
						if (nDressed == 0 && th->isDressed()) {
							//if (coloring) {
								gameText.append(QString::fromUtf8("<strong>одеты:</strong>"), true);
							//} else {
							//	gameText.append(QString::fromUtf8("одеты:"), false);
							//}
							nDressed = 1;
						} else if (nDressed == 0 || (nDressed == 1 && !th->isDressed())) {
							//if (coloring) {
								gameText.append(QString::fromUtf8("<strong>не одеты:</strong>"), true);
							//} else {
							//	gameText.append(QString::fromUtf8("не одеты:"), false);
							//}
							nDressed = 2;
						}
						QString thingStr = th->toString(Thing::ShowAll);
						QString colorStr;
						//if (coloring) {
							QColor c = pers->getThingColorByRow(i, iface);
							if (c.isValid())
								colorStr = c.name();
						//}
						if (!colorStr.isEmpty()) {
							gameText.append(QString("<font color=\"%1\">%2</font>").arg(colorStr).arg(thingStr), true);
						} else {
							gameText.append(thingStr, false);
						}
						++i;
					}
					pers->removeThingsInterface(iface);
					gameText.append(QString::fromUtf8("N- выберете предмет. (отправьте номер вместо N)"), false);
					gameText.append(QString::fromUtf8("0- в игру"), false);
				}
			} else if (sMessage.startsWith(QString::fromUtf8("Выбрана вещь: "))) {
				// Выбрана вещь: кровавый кристалл (вещь)

				gameText.setEnd(); // Блокируем дальнейший анализ
				nPersStatus = Pers::StatusThingIsTaken;
			} else if (sMessage.startsWith(QString::fromUtf8("Дом, милый дом."))) {
				// "Телепортация" домой

				gameText.setEnd(); // Блокируем дальнейший анализ
				nPersStatus = Pers::StatusAtHome;
			} else if (fightShowReg.indexIn(sMessage, 0) != -1) {
				// Режим просмотра чужого боя в 05 3

				gameText.setEnd(); // Блокируем дальнейший анализ
				nPersStatus = Pers::StatusFightShow;
			} else if (otherPersPosReg1.indexIn(sMessage, 0) != -1) {
				// Режим просмотра положения игроков по 05
				QVector<GameMap::maps_other_pers> aOtherPers;
				while (!gameText.isEnd()) {
					sMessage = gameText.nextLine().trimmed();
					if (otherPersPosReg2.indexIn(sMessage, 0) != -1) {
						struct GameMap::maps_other_pers other_pers;
						other_pers.name = otherPersPosReg2.cap(1).trimmed();
						if (otherPersPosReg2.cap(7).isEmpty()) {
							other_pers.offset_x = 0;
							other_pers.offset_y = 0;
						} else {
							other_pers.offset_x = otherPersPosReg2.cap(8).toInt();
							other_pers.offset_y = otherPersPosReg2.cap(9).toInt();
						}
						aOtherPers.push_back(other_pers);
					}
				}
				GameMap::instance()->setOtherPersPos(&aOtherPers);
				nPersStatus = Pers::StatusOtherPersPos;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("куплено!")) {
				nPersStatus = Pers::StatusBuyOk;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (dealerBuyReg.indexIn(sMessage, 0) != -1) {
				nPersStatus = Pers::StatusDealerBuy;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Выбери что нужно продать:")) { // Первая строка при продаже торговцу
				nPersStatus = Pers::StatusDealerSale;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Вы во дворе.")) {
				nPersStatus = Pers::StatusYard;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Немного повозившись возле станка, Вы искусно наточили свое оружие.")) {
				nPersStatus = Pers::StatusMasterRoom2;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Немного повозившись возле станка, Вы искусно наточили свое оружие и отполировали всю броню.")) {
				nPersStatus = Pers::StatusMasterRoom3;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Ваше имя в кубке конторы убийц...")) {
				nPersStatus = Pers::StatusInKillersCup;
				if (Settings::instance()->getBoolSetting(Settings::SettingInKillersCupPopup)) {
					initPopup(QString::fromUtf8("Ваше имя в кубке конторы убийц!"), 60);
				}
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (killerAttackReg.indexIn(sMessage, 0) != -1) {
				if (Settings::instance()->getBoolSetting(Settings::SettingKillerAttackPopup)) {
					initPopup(QString::fromUtf8("На вас совершено нападение! Убийца: ") + killerAttackReg.cap(1), 10);
				}
				nPersStatus = Pers::StatusKillerAttack;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Меню справки:")) {
				nPersStatus = Pers::StatusHelpMenu;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Сильнейшие персонажи.") || ratingInfoReg.indexIn(sMessage, 0) != -1) {
				nPersStatus = (sMessage == QString::fromUtf8("Сильнейшие персонажи.")) ? Pers::StatusTopList : Pers::StatusRatingInfo;
				//if (coloring) {
					gameText.replace("<big><strong><em>" + Qt::escape(gameText.currentLine()) + "</em></strong></big>", true);
					gameText.next();
					bool fOk = false;
					while (!gameText.isEnd()) {
						QString str1 = gameText.currentLine();
						if (!fOk && persInListOfTheBestReg.indexIn(str1, 0) != -1) {
							gameText.replace("<strong><font color=\"green\">" + Qt::escape(str1) + "</font></strong>", true);
							fOk = true;
						} else {
							if (str1.endsWith(QString::fromUtf8("сейчас в игре"))) {
								gameText.replace("<font color=\"green\">" + Qt::escape(str1) + "</font>", true);
							}
						}
						gameText.next();
					}
				//}
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Статистика")) {
				nPersStatus = Pers::StatusServerStatistic1;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Статистика по странам.")) {
				/*
				Статистика по странам.
				Страна Закарта 127 граждан.
				Восточная страна 100 граждан.
				Вольный город 84 граждан.
				Поле предков запад- город "Вольный"
				Поле предков восток- город "Вольный"
				Холм героев запад- страна Закарта
				Холм героев восток- страна Закарта
				Призрачный фонтан- город "Вольный"
				0- в игру
				*/
				nPersStatus = Pers::StatusServerStatistic2;
				Statistic *stat = Statistic::instance();
				while (!gameText.isEnd())
				{
					QString s1 = gameText.currentLine();
					QString par = s1.section('-', 0, 0).trimmed();

					if (par == QString::fromUtf8("Поле предков запад"))
						stat->setValue(Statistic::StatProtectAura1, s1.section('-', 1).trimmed());
					else if (par == QString::fromUtf8("Поле предков восток"))
						stat->setValue(Statistic::StatProtectAura2, s1.section('-', 1).trimmed());
					else if (par == QString::fromUtf8("Холм героев запад"))
						stat->setValue(Statistic::StatDamageAura1, s1.section('-', 1).trimmed());
					else if (par == QString::fromUtf8("Холм героев восток"))
						stat->setValue(Statistic::StatDamageAura2, s1.section('-', 1).trimmed());
					else if (par == QString::fromUtf8("Призрачный фонтан"))
						stat->setValue(Statistic::StatRegenAura1, s1.section('-', 1).trimmed());

					gameText.next();
				}
			} else if (sMessage == QString::fromUtf8("Ваша недвижимость:")) {
				nPersStatus = Pers::StatusRealEstate;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Выберите полку:")) {
				nPersStatus = Pers::StatusWarehouse;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Тролль:")) {
				nPersStatus = Pers::StatusTroll;
			} else if (warehouseShelfReg.indexIn(sMessage, 0) != -1) {
				nPersStatus = Pers::StatusWarehouseShelf;
				gameText.setEnd(); // Блокируем дальнейший анализ
			} else if (sMessage.startsWith(QString::fromUtf8("здесь идут сражения"))) {
				nPersStatus = Pers::StatusFightMultiSelect;
			} else if (sMessage.startsWith(QString::fromUtf8("новый бой"))) {
				nPersStatus = Pers::StatusFightOpenBegin;
				fNewFight = true;
			} else if (sMessage.startsWith(QString::fromUtf8("Команды:"))) {
				nPersStatus = Pers::StatusFightFinish;
			} else if (persStatus == Pers::StatusSecretBefore) {
				// Предыдущий статус  "перед тайником"
				if (sMessage.contains(QString::fromUtf8("тайник"), Qt::CaseInsensitive) || sMessage.startsWith(QString::fromUtf8("с разбитого ящика"), Qt::CaseInsensitive) || sMessage.contains(QString::fromUtf8("сокровищниц"), Qt::CaseInsensitive)) {
					nPersStatus = Pers::StatusSecretGet;
					if (fightDropMoneyReg2.indexIn(sMessage, 0) != -1) {
						fDropMoneys = true;
						nDropMoneys = nDropMoneys + fightDropMoneyReg2.cap(2).toInt();
					} else if (secretDropThingReg.indexIn(sMessage, 0) != -1) {
						++nThingsDropCount;
						sThingDropLast = secretDropThingReg.cap(2);
						if (Settings::instance()->getBoolSetting(Settings::SettingThingDropPopup)) {
							initPopup("+" + sThingDropLast, 3);
						}
					}
				}
			} else if (persStatus == Pers::StatusTakeBefore) {
				// Предыдущий статус  "перед отъемом"
				nPersStatus = Pers::StatusTake;
				if (fightDropMoneyReg2.indexIn(sMessage, 0) != -1) {
					fDropMoneys = true;
					nDropMoneys = nDropMoneys + fightDropMoneyReg2.cap(2).toInt();
				} else if (secretDropThingReg.indexIn(sMessage, 0) != -1) {
					++nThingsDropCount;
					sThingDropLast = secretDropThingReg.cap(2);
					if (Settings::instance()->getBoolSetting(Settings::SettingThingDropPopup)) {
						initPopup("+" + sThingDropLast, 3);
					}
				}
			} else if (sMessage.startsWith(QString::fromUtf8("Открытый бой"), Qt::CaseInsensitive)) {
				if (fightColoring)
					gameText.replace(QString::fromUtf8("<strong>Открытый бой</strong>") + sMessage.mid(12), true);
				nPersStatus = Pers::StatusFightOpenBegin;
				fight->start();
				fight->setMode(FIGHT_MODE_OPEN);
				gameText.next();
				parseFightGroups(gameText);
				nTimeout = fight->timeout();
				break;
			} else if (sMessage.startsWith(QString::fromUtf8("Закрытый бой"), Qt::CaseInsensitive)) {
				if (fightColoring)
					gameText.replace(QString::fromUtf8("<strong>Закрытый бой</strong>") + sMessage.mid(12), true);
				nPersStatus = Pers::StatusFightCloseBegin;
				fight->start();
				fight->setMode(FIGHT_MODE_CLOSE);
				gameText.next();
				parseFightGroups(gameText);
				nTimeout = fight->timeout();
				break;
			} else {
				// Строчка первая, но статус не определен
				searchHorseshoe(sMessage);
				// Статус не определяется на первой строке много где,
				// но в данном случае нас интересует резутьтат удара в бою
				if (fight->isActive() || persStatus == Pers::StatusFightMultiSelect || persStatus == Pers::StatusNotKnow) {
					// Сейчас статус не известен. На предыдущем ходе были в бою или не известно чего делали.
					if (parseFightStepResult(gameText)) { // Это был текст с результатами боя
						if (fight->allyCount() == 0) { // Мин. и макс. удары считаем только когда в один в бою
							int num1 = fight->minDamage();
							if (num1 != -1) {
								if (stat->isEmpty(Statistic::StatDamageMinFromPers) || num1 < stat->value(Statistic::StatDamageMinFromPers).toInt()) {
									// Записываем минимальный удар в статистику
									stat->setValue(Statistic::StatDamageMinFromPers, num1);
								}
							}
							num1 = fight->maxDamage();
							if (num1 > stat->value(Statistic::StatDamageMaxFromPers).toInt()) {
								// Записываем максимальный удар в статистику
								stat->setValue(Statistic::StatDamageMaxFromPers, num1);
							}
						}
						sMessage = gameText.currentLine().trimmed();
						if (sMessage.startsWith(QString::fromUtf8("Открытый бой"))) {
							if (fightColoring)
								gameText.replace(QString::fromUtf8("<strong>Открытый бой</strong>") + sMessage.mid(12), true);
							nPersStatus = Pers::StatusFightOpenBegin;
							if (!fight->isActive())
								fight->start();
							fight->setMode(FIGHT_MODE_OPEN);
							gameText.next();
							parseFightGroups(gameText);
							nTimeout = fight->timeout();
							break;
						} else if (sMessage.startsWith(QString::fromUtf8("Закрытый бой"))) {
							if (fightColoring)
								gameText.replace(QString::fromUtf8("<strong>Закрытый бой</strong>") + sMessage.mid(12), true);
							nPersStatus = Pers::StatusFightCloseBegin;
							if (!fight->isActive())
								fight->start();
							fight->setMode(FIGHT_MODE_CLOSE);
							gameText.next();
							parseFightGroups(gameText);
							nTimeout = fight->timeout();
							break;
						} else if (sMessage.startsWith(QString::fromUtf8("Бой завершен!"))) {
							// Бой завершен
							nPersStatus = Pers::StatusFightFinish;
							fight->finish();
							nTimeout = 0;
							gameText.next();
							if (!gameText.isEnd()) {
								sMessage = gameText.currentLine().trimmed();
								if (experienceDropReg.indexIn(sMessage, 0) != -1) {
									// Берем опыт, который дали в конце боя
									long long experienceDropCount = experienceDropReg.cap(1).toLongLong();
									stat->setValue(Statistic::StatExperienceDropCount,
										stat->value(Statistic::StatExperienceDropCount).toLongLong() + experienceDropCount);
								}
							}
							break; // Блокируем дальнейший анализ
						}
					}
				}
			}
		} else {
			// *** Строка уже НЕ ПЕРВАЯ в выборке и, скорее всего, статус персонажа уже известен ***
			// Тут анализируем информацию, которая находится заведомо не в первой строке ответа сервера
			// По хорошему эту логику нужно переносить в соответствующие блоки сразу после определения статуса
			if (nPersStatus == Pers::StatusStand || nPersStatus == Pers::StatusSecretBefore) {
				// Персонаж стоит
				if (commandStrReg.indexIn(sMessage, 0) != -1) {
					// Найдена строка команды для движения
					QString cmdStr = commandStrReg.cap(2).trimmed();
					// Т.к. команда "на север" не всегда "8" и т.д., то сначала анализируем строку
					if (cmdStr.contains(QString::fromUtf8("на юг"), Qt::CaseInsensitive)) {
						nCmdStatus |= 4;
					} else if (cmdStr.contains(QString::fromUtf8("на запад"), Qt::CaseInsensitive)) {
						nCmdStatus |= 16;
					} else if (cmdStr.contains(QString::fromUtf8("на восток"), Qt::CaseInsensitive)) {
						nCmdStatus |= 64;
					} else if (cmdStr.contains(QString::fromUtf8("на север"), Qt::CaseInsensitive)) {
						nCmdStatus |= 256;
					} else if (cmdStr.contains(QString::fromUtf8("найти"), Qt::CaseInsensitive) || cmdStr.contains(QString::fromUtf8("искать"), Qt::CaseInsensitive) || cmdStr.contains(QString::fromUtf8("портал"), Qt::CaseInsensitive) || cmdStr.contains(QString::fromUtf8("встать в центр"), Qt::CaseInsensitive)) {
						nCmdStatus |= 1024; // Тут есть портал
					} else if (cmdStr.contains(QString::fromUtf8("обыскать тайник"), Qt::CaseInsensitive) || cmdStr.contains(QString::fromUtf8("разбить ящик"), Qt::CaseInsensitive) || cmdStr.contains(QString::fromUtf8("обыскать сокровищницу"), Qt::CaseInsensitive)) {
						nCmdStatus |= 2048; // Тут есть тайник
					} else {
						// Анализируем цифру команды
						int cmdNum = commandStrReg.cap(1).toInt();
						if (cmdNum == 2) {
							nCmdStatus |= 8;
							if (cmdStr.contains(QString::fromUtf8("я не готов"), Qt::CaseInsensitive)) {
								nCmdStatus |= 2;
							}
						} else if (cmdNum == 4) {
							nCmdStatus |= 32;
						} else if (cmdNum == 6) {
							nCmdStatus |= 128;
						} else if (cmdNum == 8) {
							nCmdStatus |= 512;
						} else {
							// Есть нестандартная команда
							if (cmdNum == 1 && (cmdStr.contains(QString::fromUtf8("выйти"), Qt::CaseInsensitive) || cmdStr.contains(QString::fromUtf8("уйти"), Qt::CaseInsensitive) || cmdStr.contains(QString::fromUtf8("назад"), Qt::CaseInsensitive))) {
								nCmdStatus |= 2;
							}
							nCmdStatus |= 1; // Есть посторонние команды
						}
					}
				}
			} else if (nPersStatus == Pers::StatusSecretGet || nPersStatus == Pers::StatusTake || nPersStatus == Pers::StatusNotKnow) {
				long long expDropCount = 0;
				if (experienceDropReg.indexIn(sMessage, 0) != -1) {
					// Берем опыт, который дали в тайнике
					expDropCount = experienceDropReg.cap(1).toLongLong();
				} else if (experienceDropReg2.indexIn(sMessage, 0) != -1) {
					// Берем опыт, который дали в тайнике
					expDropCount = experienceDropReg2.cap(1).toLongLong();
				}
				if (expDropCount > 0) {
					stat->setValue(Statistic::StatExperienceDropCount,
						stat->value(Statistic::StatExperienceDropCount).toLongLong() + expDropCount);
				}
			} else if (nPersStatus == Pers::StatusTroll) {
				if (sMessage.contains(QString::fromUtf8("вот вам осколок"), Qt::CaseInsensitive)) {
					++nThingsDropCount;
					sThingDropLast = QString::fromUtf8("осколок камня судьбы");
					if (Settings::instance()->getBoolSetting(Settings::SettingThingDropPopup)) {
						initPopup("+" + sThingDropLast, 3);
					}
				}
			} else if (nPersStatus != Pers::StatusFightFinish) {
				if (experienceDropReg.indexIn(sMessage, 0) != -1) {
					// Найден опыт, но не в бою (У Элементаля например)
					long long expDropCount = experienceDropReg.cap(1).toLongLong();
					stat->setValue(Statistic::StatExperienceDropCount,
						stat->value(Statistic::StatExperienceDropCount).toLongLong() + expDropCount);
				}
			}
			searchHorseshoe(sMessage);
		}
		if (nPersStatus == Pers::StatusNotKnow) {
			// Любая строка, статус еще не определен
			if (sMessage.startsWith(QString::fromUtf8("Открытый бой"), Qt::CaseInsensitive)) {
				if (fightColoring)
					gameText.replace(QString::fromUtf8("<strong>Открытый бой</strong>") + sMessage.mid(12), true);
				nPersStatus = Pers::StatusFightOpenBegin;
				fight->start();
				fight->setMode(FIGHT_MODE_OPEN);
			} else if (sMessage.startsWith(QString::fromUtf8("Закрытый бой"), Qt::CaseInsensitive)) {
				if (fightColoring)
					gameText.replace(QString::fromUtf8("<strong>Закрытый бой</strong>") + sMessage.mid(12), true);
				nPersStatus = Pers::StatusFightCloseBegin;
				fight->start();
				fight->setMode(FIGHT_MODE_CLOSE);
			}
		} else if (nPersStatus == Pers::StatusStand) {
			// Статус персонажа определен как стоит
			if (secretBeforeReg.indexIn(sMessage, 0) != -1  || secretBeforeReg2.indexIn(sMessage, 0) != -1) {
				// Перед тайником
				nPersStatus = Pers::StatusSecretBefore;
			} else if (takeBeforeReg.indexIn(sMessage, 0) != -1) {
				// перед грабежом побежденного
				nPersStatus = Pers::StatusTakeBefore;
			}
		}
		gameText.next();
	}
	// Парсинг закончен, реагируем на результаты
	if (nPersStatus == Pers::StatusFightOpenBegin || nPersStatus == Pers::StatusFightCloseBegin) {
		if (!fight->isActive()) {
			fight->start();
		}
		if (fight->isActive()) {
			int fStep = fight->getStep();
			if (fStep == 1) {
				QStringList enemies = fight->mobEnemiesList();
				bool specReset = false;
				bool specNotMark = false;
				Settings *settings = Settings::instance();
				foreach (Settings::SpecificEnemy se, settings->getSpecificEnemies()) {
					if (enemies.indexOf(se.name) != -1) {
						if (!specNotMark && se.mapNotMark)
							specNotMark = true;
						if (!specReset && se.resetQueue)
							specReset = true;
						if (specNotMark && specReset)
							break;
					}
				}
				if (!specNotMark) {
					int enCnt = fight->gameMobEnemyCount();
					if (enCnt > 0) {
						GameMap *maps = GameMap::instance();
						maps->setMapElementEnemies(persPos, enCnt, enCnt);
						maps->setMapElementEnemiesList(persPos, fight->mobEnemiesList());
					}
				}
				Sender *sd = Sender::instance();
				if (specReset) {
					const int q_len = sd->getGameQueueLength();
					if (q_len > 0) {
						sd->resetGameQueue();
						if (settings->getBoolSetting(Settings::SettingResetQueuePopup)) {
							initPopup(QString::fromUtf8("Очередь сброшена"), 30);
						}
						GameText text(QString::fromUtf8("### Очередь сброшена. Сброшено команд: %1 ###").arg(q_len), false);
						setGameText(text, 3);
						setConsoleText(text, 3, false);
					}
				}
			}
			else if (fStep > 1) { // Должен быть не первый шаг, т.к. есть игровая бага из за которой не видны ауры на первом ударе
				// Анализ наличия характерных аур, для отображения в статистике
				Statistic *stat = Statistic::instance();
				Pers *pers = Pers::instance();

				int oldProtectValue = 0;
				if (stat->value(Statistic::StatProtectAura1).toString() == pers->citizenship())
					++oldProtectValue;
				if (stat->value(Statistic::StatProtectAura2).toString() == pers->citizenship())
					++oldProtectValue;
				int oldDamageValue = 0;
				if (stat->value(Statistic::StatDamageAura1).toString() == pers->citizenship())
					++oldDamageValue;
				if (stat->value(Statistic::StatDamageAura2).toString() == pers->citizenship())
					++oldDamageValue;
				if (fight->allyAurasCount() == 0)
				{
					if (oldProtectValue > 0)
					{
						stat->setValue(Statistic::StatProtectAura1, QVariant());
						stat->setValue(Statistic::StatProtectAura2, QVariant());
					}
					if (oldDamageValue > 0)
					{
						stat->setValue(Statistic::StatDamageAura1, QVariant());
						stat->setValue(Statistic::StatDamageAura2, QVariant());
					}
				}
				else {
					QString val = fight->getAllyAuraValue(QString::fromUtf8("дух предков"));
					if (val.isEmpty())
					{
						if (oldProtectValue != 0)
						{
							stat->setValue(Statistic::StatProtectAura1, QVariant());
							stat->setValue(Statistic::StatProtectAura2, QVariant());
						}
					}
					else if (val == "20%" && fight->gameHumanAllyCount() == 0)
					{
						if (oldProtectValue != 1)
						{
							stat->setValue(Statistic::StatProtectAura1, pers->citizenship());
							stat->setValue(Statistic::StatProtectAura2, QVariant());
						}
					}
					else if (val == "40%" && fight->gameHumanAllyCount() == 0)
					{
						if (oldProtectValue != 2)
						{
							stat->setValue(Statistic::StatProtectAura1, pers->citizenship());
							stat->setValue(Statistic::StatProtectAura2, pers->citizenship());
						}
					}
					val = fight->getAllyAuraValue(QString::fromUtf8("кровавый призрак"));
					if (val.isEmpty())
					{
						if (oldDamageValue != 0)
						{
							stat->setValue(Statistic::StatDamageAura1, QVariant());
							stat->setValue(Statistic::StatDamageAura2, QVariant());
						}
					}
					else if (val == "20%" && fight->gameHumanAllyCount() == 0)
					{
						if (oldDamageValue != 1)
						{
							stat->setValue(Statistic::StatDamageAura1, pers->citizenship());
							stat->setValue(Statistic::StatDamageAura2, QVariant());
						}
					} else if (val == "40%" && fight->gameHumanAllyCount() == 0)
					{
						if (oldDamageValue != 2)
						{
							stat->setValue(Statistic::StatDamageAura1, pers->citizenship());
							stat->setValue(Statistic::StatDamageAura2, pers->citizenship());
						}
					}
				}
			}
		}
	} else if (nPersStatus != Pers::StatusFightMultiSelect) {
		if (fight->isActive()) {
			fight->finish();
		}
	}
	// Начало установки параметров персонажа
	pers->beginSetPersParams();
	// Отправляем здоровье
	if (fHealth) {
		pers->setPersParams(Pers::ParamHealthMax, TYPE_INTEGER_FULL, nHealthF);
		pers->setPersParams(Pers::ParamHealthCurr, TYPE_INTEGER_FULL, nHealthC);
	}
	// Отправляем энергию
	if (fPower) {
		pers->setPersParams(Pers::ParamEnergyMax, TYPE_INTEGER_FULL, nPowerF);
		pers->setPersParams(Pers::ParamEnergyCurr, TYPE_INTEGER_FULL, nPowerC);
	}
	// Отправляем уровень персонажа
	if (fLevel) {
		pers->setPersParams(Pers::ParamPersLevel, TYPE_INTEGER_FULL, nLevel);
	}
	// Отправляем опыт
	if (fExperience) {
		pers->setPersParams(Pers::ParamExperienceMax, TYPE_LONGLONG_FULL, nExperienceFull);
		pers->setPersParams(Pers::ParamExperienceCurr, TYPE_LONGLONG_FULL, nExperience);
	}
	// Сохраняем и отправляем текущий статус персонажа
	if (persStatus != nPersStatus) {
		if (nPersStatus == Pers::StatusFightFinish) {
			stat->setValue(Statistic::StatFightsCount, stat->value(Statistic::StatFightsCount).toInt() + 1);
		}
		persStatus = nPersStatus;
		pers->setPersParams(Pers::ParamPersStatus, TYPE_INTEGER_FULL, nPersStatus);
	}
	// Конец установки параметров персонажа
	pers->endSetPersParams();
	// Отправляем имя персонажа
	if (fName && (sName != Pers::instance()->name())) {
		pers->setName(sName);
		persParamChanged();
		// Обновляем регулярки
		updateRegExpForPersName();
	}
	// Отправляем упавшие деньги
	if (fDropMoneys) {
		stat->setValue(Statistic::StatDropMoneys, stat->value(Statistic::StatDropMoneys).toInt() + nDropMoneys);
	}
	// Отправляем количество найденных вещей
	if (nThingsDropCount > 0) {
		stat->setValue(Statistic::StatThingsDropCount, stat->value(Statistic::StatThingsDropCount).toInt() + nThingsDropCount);
		stat->setValue(Statistic::StatThingDropLast, sThingDropLast);
	}
	// Отправляем значение таймаута
	if (nTimeout >= 0) {
		valueChanged(VALUE_TIMEOUT, TYPE_INTEGER_FULL, nTimeout);
	}
	//if (myMessage) {
		// Выводим сообщение игры на экран
		setGameText(gameText, 2);
	//}
	if (nCmdStatus > 3  && persPos.isValid()) { // Т.е есть что то кроме нестандартных и посторонних команд и координаты
		GameMap::instance()->setMapElementPaths(persPos, nCmdStatus);
		if ((nCmdStatus & 1024) != 0) {
			GameMap::instance()->setMapElementType(persPos, MapScene::MapElementFeature(MapScene::LocationPortal));
		} else if ((nCmdStatus & 2048) != 0) {
			GameMap::instance()->setMapElementType(persPos, MapScene::MapElementFeature(MapScene::LocationSecret));
		}
	}
	if (fNewFight) {
		// Если выбран новый бой, посылаем "0" чтобы показать список мобов
		Sender::instance()->sendSystemString("0"); // Продолжение игры
	} else if (nPersStatus == Pers::StatusFightMultiSelect) {
		int mode = Settings::instance()->getIntSetting(Settings::SettingFightSelectAction);
		if (mode == 1 || mode == 2) { // Всегда новый бой
			Sender *sender = Sender::instance();
			if (mode == 2 || sender->getGameQueueLength() > 0) {
				sender->sendSystemString("1"); // 1 - новый бой
			}
		}
	} else if ((nPersStatus == Pers::StatusNotKnow && Settings::instance()->getBoolSetting(Settings::SettingResetQueueForUnknowStatus))
		|| nPersStatus == Pers::StatusKillerAttack) {
		// Если очередь не пуста, сбрасываем очередь
		Sender *sender = Sender::instance();
		int q_len = sender->getGameQueueLength();
		if (q_len > 0) {
			sender->resetGameQueue();
			if (Settings::instance()->getBoolSetting(Settings::SettingResetQueuePopup)) {
				initPopup(QString::fromUtf8("Очередь сброшена"), 30);
			}
			GameText text(QString::fromUtf8("### Очередь сброшена. Сброшено команд: %1 ###").arg(q_len), false);
			setGameText(text, 3);
			setConsoleText(text, 3, false);
		}
	}
	Sender::instance()->setGameTextFilter(myMessage);
}

void PluginCore::searchHorseshoe(const QString &sMessage)
{
	if (sMessage.endsWith('U')) {
		initPopup(QString::fromUtf8("Обнаружена подкова!"), 20);
		QVector<GameMap::maps_other_pers> aHorseshoe;
		GameMap::maps_other_pers mop;
		mop.offset_x = 0;
		mop.offset_y = 0;
		mop.name = QString::fromUtf8("*подкова*");
		aHorseshoe.append(mop);
		GameMap::instance()->setOtherPersPos(&aHorseshoe);
	}
}

void PluginCore::processError(int errorNum)
{
	/**
	* Обрабатываются ошибочные ситуации
	**/
	QString errStr;
	if (errorNum == ERROR_INCORRECT_ACCOUNT) {
		errStr = QString::fromUtf8("### Не верный игровой аккаунт или аккаунт не активный ###");
	} else if (errorNum == ERROR_NO_MORE_MIRRORS) {
		errStr = QString::fromUtf8("### Нет доступных зеркал игры ###");
	} else if (errorNum == ERROR_SERVER_TIMEOUT) {
		errStr = QString::fromUtf8("### Нет ответа от сервера ###");
	} else if (errorNum == ERROR_QUICK_COMMAND) {
		errStr = QString::fromUtf8("### Слишком часто отправляются команды ###");
	} else {
		errStr = QString::fromUtf8("#### Неизвестная ошибка ####");
	}
	GameText gameText(errStr, false);
	setGameText(gameText, 3);
	setConsoleText(gameText, 3, false);
}

/**
 * Обрабатывает событие начала боя
 */
void PluginCore::fightStarted(int mode)
{
	if (mode == FIGHT_MODE_OPEN) {
		if (Settings::instance()->getIntSetting(Settings::SettingFightAutoClose) == 1) { // Автозакрытие боя если один против мобов
			if (fight->isPersInFight()) { // Наш персонаж в бою?
				if (fight->gameMobEnemyCount() > 0 && fight->gameHumanEnemyCount() == 0) { // В противниках только мобы
					if (fight->gameMobAllyCount() == 0 && fight->gameHumanAllyCount() == 0) { // В нашей команде никого нет
						Sender *sender = Sender::instance();
						sender->sendSystemString("*"); // Команда закрытия боя
						sender->sendSystemString("0"); // Продолжение игры
					}
				}
			}
		}
	}
}

void PluginCore::valueChanged(int valueId, int valueType, int value)
{
	foreach (SofMainWindow *wnd, mainWindowList)
	{
		emit wnd->valueChanged(valueId, valueType, value);
	}
}

void PluginCore::setGameText(const GameText &gameText, int type)
{
	foreach (SofMainWindow *wnd, mainWindowList)
	{
		wnd->setGameText(gameText.toHtml(), type);
	}
}

void PluginCore::setConsoleText(const GameText &gameText, int type, bool switch_)
{
	foreach (SofMainWindow *wnd, mainWindowList)
	{
		wnd->setConsoleText(gameText.toHtml(), type, switch_);
	}
}

bool PluginCore::sendCommandToCore(qint32 commandId)
{
	if (commandId == COMMAND_SAVE_SETTINGS) {
		// Немедленно сохраняет настройки плагина
		return Settings::instance()->save();
	} else if (commandId == COMMAND_CLOSE_WINDOW) {
		// Закрывается окно плагина
		//messageFiltered = false;
	}
	return false;
}

bool PluginCore::savePersStatus()
{
	// ***** Сохраняет основные параметры статуса персонажа в XML файле *****
	if (accJid.isEmpty()) {
		return false;
	}
	// Читаем старые настройки
	QDomDocument xmlDoc;
	QDomElement eRoot;
	QDomNode eOldAccount;
	bool bReplace = false;
	if (loadPluginXml(&xmlDoc, "sofgame_status.xml")) {
		// Ищем наш account элемент
		eRoot = xmlDoc.documentElement();
		if (eRoot.tagName() == "status") {
			eOldAccount = eRoot.firstChild();
			while (!eOldAccount.isNull()) {
				if (eOldAccount.toElement().tagName() == "account" && eOldAccount.toElement().attribute("jid") == accJid) {
					bReplace = true;
					break;
				}
				eOldAccount = eOldAccount.nextSibling();
			}
		}
	}
	// Создаем элемент нашего аккаунта
	QDomElement eNewAccount = xmlDoc.createElement("account");
	eNewAccount.setAttribute("jid", accJid);
	QDomElement domElement;
	Pers *pers = Pers::instance();
	Settings *settings = Settings::instance();
	if (settings->getIntSetting(Settings::SettingPersSaveMode) != 0) {
		if (settings->getBoolSetting(Settings::SettingSavePersParams)) {
			QDomElement eMain = xmlDoc.createElement("main");
			eNewAccount.appendChild(eMain);
			if (!pers->name().isEmpty()) {
				domElement = xmlDoc.createElement("pers-name");
				domElement.appendChild(xmlDoc.createTextNode(pers->name()));
				eMain.appendChild(domElement);
			}
			if (!pers->citizenship().isEmpty()) {
				domElement = xmlDoc.createElement("pers-citizenship");
				domElement.appendChild(xmlDoc.createTextNode(pers->citizenship()));
				eMain.appendChild(domElement);
			}
			int num1;
			if (pers->getIntParamValue(Pers::ParamPersLevel, &num1)) {
				domElement = xmlDoc.createElement("pers-level");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
				eMain.appendChild(domElement);
			}
			long long experience;
			if (pers->getLongParamValue(Pers::ParamExperienceCurr, &experience)) {
				domElement = xmlDoc.createElement("pers-experience-curr");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(experience)));
				eMain.appendChild(domElement);
			}
			if (pers->getLongParamValue(Pers::ParamExperienceMax, &experience)) {
				domElement = xmlDoc.createElement("pers-experience-max");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(experience)));
				eMain.appendChild(domElement);
			}
			if (pers->getIntParamValue(Pers::ParamHealthCurr, &num1)) {
				domElement = xmlDoc.createElement("pers-health-curr");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
				eMain.appendChild(domElement);
			}
			if (pers->getIntParamValue(Pers::ParamHealthMax, &num1)) {
				domElement = xmlDoc.createElement("pers-health-max");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
				eMain.appendChild(domElement);
			}
			if (pers->getIntParamValue(Pers::ParamEnergyCurr, &num1)) {
				domElement = xmlDoc.createElement("pers-energy-curr");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
				eMain.appendChild(domElement);
			}
			if (pers->getIntParamValue(Pers::ParamEnergyMax, &num1)) {
				domElement = xmlDoc.createElement("pers-energy-max");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
				eMain.appendChild(domElement);
			}
		}
		if (settings->getBoolSetting(Settings::SettingSaveStatistic)) {
			QDomElement eStat = xmlDoc.createElement("statistic");
			eNewAccount.appendChild(eStat);
			Statistic *stat = Statistic::instance();
			if (!stat->isEmpty(Statistic::StatLastGameJid)) {
				domElement = xmlDoc.createElement("last-game-jid");
				domElement.appendChild(xmlDoc.createTextNode(stat->value(Statistic::StatLastGameJid).toString()));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatLastChatJid)) {
				domElement = xmlDoc.createElement("last-chat-jid");
				domElement.appendChild(xmlDoc.createTextNode(stat->value(Statistic::StatLastChatJid).toString()));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatMessagesCount)) {
				domElement = xmlDoc.createElement("in-messages-count");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(stat->value(Statistic::StatMessagesCount).toInt())));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatFightsCount)) {
				domElement = xmlDoc.createElement("fights-count");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(stat->value(Statistic::StatFightsCount).toInt())));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatDamageMaxFromPers)) {
				domElement = xmlDoc.createElement("pers-fight-damage-max");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(stat->value(Statistic::StatDamageMaxFromPers).toInt())));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatDamageMinFromPers)) {
				domElement = xmlDoc.createElement("pers-fight-damage-min");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(stat->value(Statistic::StatDamageMinFromPers).toInt())));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatDropMoneys)) {
				domElement = xmlDoc.createElement("moneys-drop-count");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(stat->value(Statistic::StatDropMoneys).toInt())));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatThingsDropCount)) {
				domElement = xmlDoc.createElement("fings-drop-count");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(stat->value(Statistic::StatThingsDropCount).toInt())));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatThingDropLast)) {
				domElement = xmlDoc.createElement("fing-drop-last");
				domElement.appendChild(xmlDoc.createTextNode(stat->value(Statistic::StatThingDropLast).toString()));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatExperienceDropCount)) {
				domElement = xmlDoc.createElement("experience-drop-count");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(stat->value(Statistic::StatExperienceDropCount).toLongLong())));
				eStat.appendChild(domElement);
			}
			if (!stat->isEmpty(Statistic::StatKilledEnemies)) {
				domElement = xmlDoc.createElement("killed-enemies");
				domElement.appendChild(xmlDoc.createTextNode(QString::number(stat->value(Statistic::StatKilledEnemies).toInt())));
				eStat.appendChild(domElement);
			}
		}
		if (settings->getBoolSetting(Settings::SettingSaveBackpack)) {
			// Сохраняем данные о вещах
			QDomElement eBackpack = xmlDoc.createElement("backpack");
			pers->backpackToXml(eBackpack);
			eNewAccount.appendChild(eBackpack);
		}
	}
	if (!bReplace) {
		// Если настройки не прочитались или они некорректные, то создаем новый xml документ
		if (eRoot.isNull()) {
			eRoot = xmlDoc.createElement("status");
			eRoot.setAttribute("version", "0.1");
			xmlDoc.appendChild(eRoot);
		}
		eRoot.appendChild(eNewAccount);
	} else {
		// Заменяем старые настройки новыми
		eRoot.replaceChild(eNewAccount, eOldAccount);
	}
	// Сохраняем документ в файл
	if (savePluginXml(&xmlDoc, "sofgame_status.xml")) {
		persStatusChangedFlag = false;
		persBackpackChangedFlag = false;
		statisticChangedFlag = false;
		return true;
	}
	return false;
}

bool PluginCore::loadPersStatus()
{
	// Сброс статуса персонажа
	QVariant lastGameJid;
	QVariant lastChatJid;
	persStatus = Pers::StatusNotKnow;
	QVariant msgCount;
	QVariant moneysDropCount;
	QVariant fightsCount;
	QVariant fightDamageMin;
	QVariant fightDamageMax;
	QVariant thingsDropCount;
	QVariant thingDropLast;
	QVariant expDropCount;
	QVariant killedEnemies;
	// Загрузка статуса
	if (accJid.isEmpty()) {
		return false;
	}
	QDomDocument xmlDoc;
	if (!loadPluginXml(&xmlDoc, "sofgame_status.xml")) {
		return false;
	}
	QDomElement eRoot = xmlDoc.documentElement();
	if (eRoot.tagName() != "status") {
		return false;
	}
	QDomNode childStatusNode = eRoot.firstChild();
	QDomNode childMainNode;
	QDomNode childStatNode;
	QString str1, sTagName;

	Settings *settings = Settings::instance();
	Pers *pers = Pers::instance();
	pers->beginSetPersParams();
	while (!childStatusNode.isNull()) {
		if (childStatusNode.toElement().tagName() == "account" && childStatusNode.toElement().attribute("jid") == accJid) {
			QDomNode childAccountNode = childStatusNode.firstChild();
			while (!childAccountNode.isNull()) {
				if (childAccountNode.toElement().tagName() == "main") {
					if (settings->getIntSetting(Settings::SettingPersSaveMode) != 0 && settings->getBoolSetting(Settings::SettingSavePersParams)) {
						childMainNode = childAccountNode.firstChild();
						while (!childMainNode.isNull()) {
							if (childMainNode.toElement().tagName() == "pers-name") {
								pers->setName(getTextFromNode(&childMainNode));
							} else if (childMainNode.toElement().tagName() == "pers-citizenship") {
								pers->setCitizenship(getTextFromNode(&childMainNode));
							} else if (childMainNode.toElement().tagName() == "pers-level") {
								pers->setPersParams(Pers::ParamPersLevel, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							} else if (childMainNode.toElement().tagName() == "pers-experience-curr") {
								pers->setPersParams(Pers::ParamExperienceCurr, TYPE_LONGLONG_FULL, getTextFromNode(&childMainNode).toLongLong());
							} else if (childMainNode.toElement().tagName() == "pers-experience-max") {
								pers->setPersParams(Pers::ParamExperienceMax, TYPE_LONGLONG_FULL, getTextFromNode(&childMainNode).toLongLong());
							} else if (childMainNode.toElement().tagName() == "pers-health-curr") {
								pers->setPersParams(Pers::ParamHealthCurr, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							} else if (childMainNode.toElement().tagName() == "pers-health-max") {
								pers->setPersParams(Pers::ParamHealthMax, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							} else if (childMainNode.toElement().tagName() == "pers-energy-curr") {
								pers->setPersParams(Pers::ParamEnergyCurr, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							} else if (childMainNode.toElement().tagName() == "pers-energy-max") {
								pers->setPersParams(Pers::ParamEnergyMax, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							}
							childMainNode = childMainNode.nextSibling();
						}
					}
				} else if (childAccountNode.toElement().tagName() == "statistic") {
					if (settings->getIntSetting(Settings::SettingPersSaveMode) != 0 && settings->getBoolSetting(Settings::SettingSaveStatistic)) {
						childStatNode = childAccountNode.firstChild();
						while (!childStatNode.isNull()) {
							sTagName = childStatNode.toElement().tagName();
							if (sTagName == "last-game-jid") {
								lastGameJid = getTextFromNode(&childStatNode);
							} else if (sTagName == "last-chat-jid") {
								lastChatJid = getTextFromNode(&childStatNode);
							} else if (sTagName == "in-messages-count") {
								msgCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "fights-count") {
								fightsCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "pers-fight-damage-max") {
								fightDamageMax = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "pers-fight-damage-min") {
								fightDamageMin = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "moneys-drop-count") {
								moneysDropCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "fings-drop-count") {
								thingsDropCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "fing-drop-last") {
								thingDropLast = getTextFromNode(&childStatNode);
							} else if (sTagName == "experience-drop-count") {
								expDropCount = getTextFromNode(&childStatNode).toLongLong();
							} else if (sTagName == "killed-enemies") {
								killedEnemies = getTextFromNode(&childStatNode).toInt();
							}
							childStatNode = childStatNode.nextSibling();
						}
					}
				} else if (childAccountNode.toElement().tagName() == "backpack") {
					if (settings->getIntSetting(Settings::SettingPersSaveMode) != 0 && settings->getBoolSetting(Settings::SettingSaveBackpack)) {
						// Обрабатываем данные о вещах
						QDomElement el = childAccountNode.toElement();
						pers->loadThingsFromDomElement(el);
					}
				}
				childAccountNode = childAccountNode.nextSibling();
			}
		}
		childStatusNode = childStatusNode.nextSibling();
	}
	pers->endSetPersParams();
	Statistic *stat = Statistic::instance();
	stat->setValue(Statistic::StatLastGameJid, lastGameJid);
	stat->setValue(Statistic::StatLastChatJid, lastChatJid);
	stat->setValue(Statistic::StatMessagesCount, msgCount);
	stat->setValue(Statistic::StatDamageMaxFromPers, fightDamageMax);
	stat->setValue(Statistic::StatDamageMinFromPers, fightDamageMin);
	stat->setValue(Statistic::StatFightsCount, fightsCount);
	stat->setValue(Statistic::StatDropMoneys, moneysDropCount);
	stat->setValue(Statistic::StatThingsDropCount, thingsDropCount);
	stat->setValue(Statistic::StatThingDropLast, thingDropLast);
	stat->setValue(Statistic::StatExperienceDropCount, expDropCount);
	stat->setValue(Statistic::StatKilledEnemies, killedEnemies);
	return true;
}

bool PluginCore::sendString(const QString &str)
{
	// Проверяем на наличие встроенных команд
	QString str1 = str.trimmed();
	if (!str1.startsWith("/")) {
		// Проверяем наличие алиаса
		QString new_cmd = Aliases::instance()->command(str1);
		if (!new_cmd.isEmpty()) {
			str1 = new_cmd;
		}
		if (!str1.startsWith("/")) {
			// Отсылаем строку окну плагина
			setGameText(GameText(str1, false), 1);
			// Отсылаем строку серверу
			Sender::instance()->sendString(str1);
			return true;
		}
	}
	if (str1.startsWith("/help")) {
		setConsoleText(GameText(str1, false), 1, true);
		GameText text;
		text.append(QString::fromUtf8("<big><strong><em>--= help =--</em></strong></big>"), true);
		text.append(QString::fromUtf8("<strong>/-</strong> — Сброс очереди команд"), true);
		text.append(QString::fromUtf8("<strong>/1...</strong> — Позволяет отдавать односимвольные числовые команды в игру без дублирования их клавишей &lt;Enter&gt;."), true);
		text.append(QString::fromUtf8("<strong>&nbsp;&nbsp;/1+</strong> — Включение режима автоввода."), true);
		text.append(QString::fromUtf8("<strong>&nbsp;&nbsp;/1-</strong> — Отключение режима автоввода."), true);
		text.append(QString::fromUtf8("<strong>&nbsp;&nbsp;/1</strong> — Состояние режима (вкл или выкл)."), true);
		text.append(QString::fromUtf8("<strong>&nbsp;&nbsp;/1-&lt;числовая_команда&gt;</strong> — Отправка длинной команды без отключения режима. Пример: <strong>/1-02</strong> — Отправка 02 игре не отключая режимом автоввода."), true);
		text.append(QString::fromUtf8("<strong>/aliases</strong> — управление алиасами команд"), true);
		text.append(QString::fromUtf8("<strong>/clear</strong> — очистка и сброс различных данных"), true);
		text.append(QString::fromUtf8("<strong>/help</strong> — этот текст помощи"), true);
		text.append(QString::fromUtf8("<strong>/maps</strong> — информация и управление картами"), true);
		text.append(QString::fromUtf8("<strong>/pers info</strong> — информация о текущем персонаже"), true);
		text.append(QString::fromUtf8("<strong>/pers info2</strong> — детальная информация о текущем персонаже"), true);
		text.append(QString::fromUtf8("<strong>/pers info &lt;nick&gt;</strong> — информация о персонаже с ником nick"), true);
		text.append(QString::fromUtf8("<strong>/pers info2 &lt;nick&gt;</strong> — детальная информация о персонаже с ником nick"), true);
		text.append(QString::fromUtf8("<strong>/pers list</strong> — список персонажей, для которых доступна информация"), true);
		text.append(QString::fromUtf8("<strong>/send_delta</strong> — показать значение паузы между отправками команд игровому серверу"), true);
		text.append(QString::fromUtf8("<strong>/send_delta=n</strong> — установить значение паузы между отправками команд игровому серверу в миллисекундах"), true);
		text.append(QString::fromUtf8("<strong>/server_timeout</strong> — показать время ожидания ответа сервера"), true);
		text.append(QString::fromUtf8("<strong>/server_timeout=n</strong> — установить время ожидания сервера в n сек."), true);
		text.append(QString::fromUtf8("<strong>/settings</strong> — Управление настройками"), true);
		text.append(QString::fromUtf8("<strong>/stat</strong> — полная статистика"), true);
		text.append(QString::fromUtf8("<strong>/stat 1</strong> — общая статистика"), true);
		text.append(QString::fromUtf8("<strong>/stat 2</strong> — статистика боев"), true);
		text.append(QString::fromUtf8("<strong>/stat 9</strong> — статистика зеркал"), true);
		text.append(QString::fromUtf8("<strong>/things</strong> — вещи, фильтры вещей, цены"), true);
		text.append(QString::fromUtf8("<strong>/ver</strong> — версия плагина"), true);
		setConsoleText(text, 3, true);
	} else if (str1 == "/-") {
		setConsoleText(GameText(str1, false), 1, false);
		Sender *sender = Sender::instance();
		int q_len = sender->getGameQueueLength();
		if (q_len > 0) {
			sender->resetGameQueue();
			str1 = QString::fromUtf8("### Очередь сброшена. Количество: %1 ###").arg(q_len);
		} else {
			str1 = QString::fromUtf8("### Очередь пуста. ###");
		}
		GameText text(str1, false);
		setGameText(text, 3);
		setConsoleText(text, 3, false);
	} else if (str1 == "/1+") {
		foreach (SofMainWindow *wnd, mainWindowList)
		{
			wnd->setAutoEnterMode(true);
		}
	} else if (str1.startsWith("/1-")) {
		if (str1.length() == 3) {
			foreach (SofMainWindow *wnd, mainWindowList)
			{
				wnd->setAutoEnterMode(false);
			}
		} else {
			sendString(str1.mid(3));
		}
	} else if (str1 == "/1") {
		setConsoleText(GameText(str1, false), 1, false);
		SofMainWindow *wnd = NULL;
		if (!mainWindowList.isEmpty())
			wnd = mainWindowList.first();
		GameText text;
		text.append(QString("Auto enter mode is <strong>%1</strong>").arg((wnd != NULL && wnd->getAutoEnterMode()) ? "ON" : "OFF"), true);
		setGameText(text, 3);
		setConsoleText(text, 3, false);
	} else if (str1 == "/stat" || str1.startsWith("/stat ")) {
		setConsoleText(GameText(str1, false), 1, true);
		getStatistics(str1);
	} else if (str1 == "/send_delta") {
		setConsoleText(GameText(str1, false), 1, true);
		GameText text("send_delta = <strong>" + QString::number(Sender::instance()->getSendDelta()) + " msec.</strong>", true);
		setConsoleText(text, 3, true);
	} else if (str1.startsWith("/send_delta=")) {
		setConsoleText(GameText(str1, false), 1, true);
		if (str1.length() >= 13) {
			bool res;
			int nDelta = str1.mid(12).toInt(&res);
			str1 = "send_delta not set";
			if (res) {
				Sender *sender = Sender::instance();
				if (sender->setSendDelta(nDelta)) {
					str1 = "send_delta set is <strong>" + QString::number(sender->getSendDelta()) + " msec.</strong>";
				}
			}
			setConsoleText(GameText(str1, true), 3, true);
		}
	} else if (str1 == "/server_timeout") {
		setConsoleText(GameText(str1, false), 1, true);
		GameText text("server_timeout = <strong>" + QString::number(Sender::instance()->getServerTimeoutDuration()) + " sec.</strong>", true);
		setConsoleText(text, 3, true);
	} else if (str1.startsWith("/server_timeout=")) {
		setConsoleText(GameText(str1,false), 1, true);
		if (str1.length() >= 17) {
			bool res;
			int nTimeout = str1.mid(16).toInt(&res);
			str1 = "server timeout not set";
			if (res) {
				Sender *sender = Sender::instance();
				if (sender->setServerTimeoutDuration(nTimeout)) {
					str1 = "server_timeout set is <strong>" + QString::number(sender->getServerTimeoutDuration()) + " sec.</strong>";
					Settings::instance()->setIntSetting(Settings::SettingServerTimeout, nTimeout);
				}
			}
			setConsoleText(GameText(str1, true), 3, true);
		}
	} else if (str1.startsWith("/maps")) {
		setConsoleText(GameText(str1, false), 1, true);
		QStringList mapsCmd = splitCommandString(str1);
		mapsCommands(mapsCmd);
	} else if (str1.startsWith("/pers")) {
		setConsoleText(GameText(str1, false), 1, true);
		QStringList persCmd = str1.split(" ");
		persCommands(persCmd);
	} else if (str1.startsWith("/clear")) {
		setConsoleText(GameText(str1, false), 1, false);
		QStringList clearCmd = str1.split(" ");
		clearCommands(clearCmd);
	} else if (str1.startsWith("/things")) {
		setConsoleText(GameText(str1, false), 1, true);
		QStringList thingsCmd = str1.split(" ");
		thingsCommands(thingsCmd);
	} else if (str1.startsWith("/aliases")) {
		setConsoleText(GameText(str1, false), 1, true);
		QStringList aliasesCmd = splitCommandString(str1);
		aliasesCommands(aliasesCmd);
	} else if (str1.startsWith("/settings")) {
		setConsoleText(GameText(str1, false), 1, true);
		QStringList settingsCmd = str1.split(" ");
		settingsCommands(settingsCmd);
	} else if (str1 == "/ver") {
		setConsoleText(GameText(str1, false), 1, true);
		setConsoleText(GameText(cVer, false), 3, true);
	} else {
		setConsoleText(GameText(str1, false), 1, true);
		GameText text(QString::fromUtf8("Неизвестная команда. Наберите <strong>/help</strong> для получения помощи"), true);
		setConsoleText(text, 3, true);
	}
	return true;
}

void PluginCore::getStatistics(const QString &commandPtr)
{
	int mode = 0;
	QStringList args = commandPtr.split(" ");
	if (args.size() == 2) {
		if (args[1] == "1") {
			mode = 1;
		} else if (args[1] == "2") {
			mode = 2;
		} else if (args[1] == "9") {
			mode = 9;
		}
	}
	GameText text;
	text.append(QString::fromUtf8("<big><strong><em>--= Статистика =--</em></strong></big>"), true);
	Statistic *stat = Statistic::instance();
	if (mode == 0 || mode == 1) {
		text.append(QString::fromUtf8("<strong><em>--Общая статистика--</em></strong>"), true);
		text.append(QString::fromUtf8("Jid игры: <em>%1</em>")
			    .arg(stat->toString(Statistic::StatLastGameJid)), true);
		text.append(QString::fromUtf8("Jid чата: <em>%1</em>")
			.arg(stat->toString(Statistic::StatLastChatJid)), true);
		text.append(QString::fromUtf8("Всего сообщений: <em>%1</em>")
			.arg(stat->toString(Statistic::StatMessagesCount)), true);
	}
	if (mode == 0 || mode == 2) {
		text.append(QString::fromUtf8("<strong><em>--Статистика боев--</em></strong>"), true);
		text.append(QString::fromUtf8("Всего боев: <em>%1</em>")
			.arg(stat->toString(Statistic::StatFightsCount)), true);
		text.append(QString::fromUtf8("Лучший удар: <em>%1</em>")
			.arg(stat->toString(Statistic::StatDamageMaxFromPers)), true);
		text.append(QString::fromUtf8("Худший удар: <em>%1</em>")
			.arg(stat->toString(Statistic::StatDamageMinFromPers)), true);
		text.append(QString::fromUtf8("Денег собрано: <em>%1</em>")
			.arg(stat->toString(Statistic::StatDropMoneys)), true);
		text.append(QString::fromUtf8("Вещей собрано: <em>%1</em>")
			.arg(stat->toString(Statistic::StatThingsDropCount)), true);
		text.append(QString::fromUtf8("Последняя вещь: <em>%1</em>")
			.arg(stat->toString(Statistic::StatThingDropLast)), true);
		text.append(QString::fromUtf8("Полученный опыт: <em>%1</em>")
			.arg(stat->toString(Statistic::StatExperienceDropCount)), true);
		text.append(QString::fromUtf8("Повержено врагов: <em>%1</em>")
			.arg(stat->toString(Statistic::StatKilledEnemies)), true);
	}
	if (mode == 0 || mode == 9) {
		text.append(QString::fromUtf8("<strong><em>--Статистика зеркал игры--</em></strong>"), true);
		Sender *sender = Sender::instance();
		QStringList game_jids = sender->gameJidList();
		text.append(QString::fromUtf8("Всего зеркал: <em>%1</em>").arg(QString::number(game_jids.size())), true);
		int jid_index = 0;
		while (!game_jids.isEmpty()) {
			QString jid = game_jids.takeFirst();
			const struct Sender::jid_status* jstat = sender->getGameJidInfo(jid_index);
			if (jstat != 0) {
				text.append(QString::fromUtf8("-Зеркало: <em>%1</em>").arg(jid), true);
				QString str1;
				int status = jstat->status;
				if (status == 0) {
					str1 = QString::fromUtf8("отключено");
				} else {
					str1 = QString::fromUtf8("доступно");
				}
				text.append(QString::fromUtf8("&nbsp;&nbsp;статус: <em>%1</em>").arg(str1), true);
				if (jstat->last_status.isValid()) {
					str1 = jstat->last_status.toString("dd.MM.yyyy hh:mm:ss.zzz");
				} else {
					str1 = NA_TEXT;
				}
				text.append(QString::fromUtf8("&nbsp;&nbsp;последняя смена статуса: <em>%1</em>").arg(str1), true);

				if (status != 0) {
					if (jstat->last_send.isValid()) {
						str1 = jstat->last_send.toString("dd.MM.yyyy hh:mm:ss.zzz");
					} else {
						str1 = NA_TEXT;
					}
					text.append(QString::fromUtf8("&nbsp;&nbsp;последняя отправка: <em>%1</em>").arg(str1), true);
					if (jstat->last_send_ping.isValid()) {
						str1 = jstat->last_send_ping.toString("dd.MM.yyyy hh:mm:ss.zzz");
					} else {
						str1 = NA_TEXT;
					}
					text.append(QString::fromUtf8("&nbsp;&nbsp;последний пинг: <em>%1</em>").arg(str1), true);
					text.append(QString::fromUtf8("&nbsp;&nbsp;количество пакетов: <em>%1</em>").arg(QString::number(jstat->probe_count)), true);
					double inSec = jstat->resp_average / 1000.0f;
					text.append(QString::fromUtf8("&nbsp;&nbsp;средний отклик: <em>%1 сек.</em>").arg(QString::number(inSec)), true);
				}
			}
			jid_index++;
		}
	}
	setConsoleText(text, 3, true);
}

void PluginCore::mapsCommands(const QStringList &args)
{
	/**
	* Отображение данных по картам
	**/
	if (args.at(0) != "/maps")
		return;
	int cntArgs = args.size() - 1;
	GameText text;
	text.append(QString::fromUtf8("<big><strong><em>--= Карты =--</em></strong></big>"), true);
	if (cntArgs == 0) {
		text.append(QString::fromUtf8("<strong>/maps clear &lt;index&gt;</strong> — Очистка всего содержимого карты с индексом &lt;index&gt;"), true);
		text.append(QString::fromUtf8("<strong>/maps export &lt;index&gt; &lt;exp_file&gt; [xml|png]</strong> — Экспорт карты с индексом &lt;index&gt; в файл с именем &lt;exp_file&gt; и указанным типом xml или png (указывать не обязательно). Тип может быть определен по расширению файла."), true);
		text.append(QString::fromUtf8("<strong>/maps import &lt;imp_file&gt;</strong> — Импорт карт из файла"), true);
		text.append(QString::fromUtf8("<strong>/maps info</strong> — основная информация о картах"), true);
		text.append(QString::fromUtf8("<strong>/maps list</strong> — список всех карт"), true);
		text.append(QString::fromUtf8("<strong>/maps merge &lt;index1&gt; &lt;index2&gt;</strong> — Объединение карт"), true);
		text.append(QString::fromUtf8("<strong>/maps remove &lt;index&gt;</strong> — Удаление карты"), true);
		text.append(QString::fromUtf8("<strong>/maps rename &lt;index&gt; &lt;new_name&gt;</strong> — Переименование карты"), true);
		text.append(QString::fromUtf8("<strong>/maps switch &lt;index&gt;</strong> — переключение на карту с указанным индексом"), true);
		text.append(QString::fromUtf8("<strong>/maps unload &lt;index&gt;</strong> — выгрузка карты из памяти без сохранения изменений"), true);
		setConsoleText(text, 3, true);
	} else if (args.at(1) == "clear") {
		if (cntArgs == 2) {
			bool fOk;
			int map1 = args.at(2).toInt(&fOk);
			if (fOk) {
				if (GameMap::instance()->clearMap(map1)) {
					text.append(QString::fromUtf8("Карта успешно очищена"), false);
				} else {
					text.append(QString::fromUtf8("Ошибка: нет такой карты"), false);
				}
			} else {
				text.append(QString::fromUtf8("Ошибка: некорректный индекс карты"), false);
			}
		} else {
			text.append(QString::fromUtf8("Ошибка: неверное количество аргументов"), false);
		}
		setConsoleText(text, 3, true);
		return;
	} else if (args.at(1) == "rename") {
		if (cntArgs >= 3) {
			QStringList aName = args;
			aName.removeAt(0);
			aName.removeAt(0);
			bool fOk = false;
			int index = aName.takeFirst().toInt(&fOk);
			if (fOk) {
				QString sName = aName.join(" ").trimmed();
				if (!sName.isEmpty()) {
					int nRes = GameMap::instance()->renameMap(index, sName);
					switch (nRes) {
						case 0:
							text.append(QString::fromUtf8("Карта переименована"), false);
							break;
						case 1:
							text.append(QString::fromUtf8("Ошибка: нет такой карты"), false);
							break;
						case 2:
							text.append(QString::fromUtf8("Ошибка: карта с таким именем уже существует"), false);
							break;
						default:
							text.append(QString::fromUtf8("Ошибка"), false);
							break;
					}
				} else {
					text.append(QString::fromUtf8("Ошибка: необходимо указать новое имя карты"), false);
				}
			} else {
				text.append(QString::fromUtf8("Ошибка: необходимо указать индекс карты"), false);
			}
		} else {
			text.append(QString::fromUtf8("Ошибка: неверное количество аргументов"), false);
		}
		setConsoleText(text, 3, true);
		return;
	} else if (args.at(1) == "export") {
		if (cntArgs == 3 || cntArgs == 4) {
			QStringList maps = args.at(2).split(",");
			int type = 1; // XML по умолчанию
			if (cntArgs == 4) {
				// Явно указан тип
				if (args.at(4).toLower() == "png") {
					// Тип PNG
					type = 2;
				}
			} else {
				// Попробуем определить тип автоматически по расширению
				if (args.at(3).toLower().endsWith(".png")) {
					type = 2;
				}
			}
			int nRes = GameMap::instance()->exportMaps(maps, type, args.at(3));
			switch (nRes) {
				case 0:
					text.append(QString::fromUtf8("Экспорт успешно завершен"), false);
					break;
				case 1:
					text.append(QString::fromUtf8("Ошибка: неверные аргументы"), false);
					break;
				case 2:
					text.append(QString::fromUtf8("Ошибка: нет карт для экспорта"), false);
					break;
				case 3:
					text.append(QString::fromUtf8("Ошибка: неудачная запись в файл"), false);
					break;
				case 4:
					text.append(QString::fromUtf8("Может быть выгружена только одна карта"), false);
					break;
				default:
					text.append(QString::fromUtf8("Ошибка: ошибка экспорта"), false);
			}
		} else {
			text.append(QString::fromUtf8("Ошибка: неверное количество аргументов"), false);
		}
		setConsoleText(text, 3, true);
		return;
	} else if (args.at(1) == "import") {
		if (cntArgs == 2) {
			int nRes = GameMap::instance()->importMaps(args.at(2));
			switch (nRes) {
				case 0:
					text.append(QString::fromUtf8("Импорт успешно завершен"), false);
					break;
				case 1:
					text.append(QString::fromUtf8("Ошибка: ошибка чтения XML файла карт"), false);
					break;
				case 2:
					text.append(QString::fromUtf8("Ошибка: некорректрый формат карт или не совпадает версия"), false);
					break;
				default:
					text.append(QString::fromUtf8("Ошибка: ошибка импорта"), false);
			}
		} else {
			text.append(QString::fromUtf8("Ошибка: неверное количество аргументов"), false);
		}
		setConsoleText(text, 3, true);
		return;
	} else if (args.at(1) == "merge") {
		if (cntArgs == 3) {
			bool fOk;
			int map1 = args.at(2).toInt(&fOk);
			if (fOk) {
				int map2 = args.at(3).toInt(&fOk);
				if (fOk) {
					if (GameMap::instance()->mergeMaps(map1, map2)) {
						text.append(QString::fromUtf8("Объединение успешно завершено"), false);
					} else {
						text.append(QString::fromUtf8("Ошибка: ошибка объединения карт"), false);
					}
				} else {
					text.append(QString::fromUtf8("Ошибка: некорректный индекс второй карты"), false);
				}
			} else {
				text.append(QString::fromUtf8("Ошибка: некорректный индекс первой карты"), false);
			}
		} else {
			text.append(QString::fromUtf8("Ошибка: неверное количество аргументов"), false);
		}
		setConsoleText(text, 3, true);
		return;
	} else if (args.at(1) == "remove") {
		if (cntArgs == 2) {
			bool fOk;
			int map1 = args.at(2).toInt(&fOk);
			if (fOk) {
				if (GameMap::instance()->removeMap(map1)) {
					text.append(QString::fromUtf8("Карта успешно удалена"), false);
				} else {
					text.append(QString::fromUtf8("Ошибка: нет такой карты"), false);
				}
			} else {
				text.append(QString::fromUtf8("Ошибка: некорректный индекс карты"), false);
			}
		} else {
			text.append(QString::fromUtf8("Ошибка: неверное количество аргументов"), false);
		}
		setConsoleText(text, 3, true);
		return;
	} else if (cntArgs == 1) {
		if (args.at(1) == "info") {
			struct GameMap::maps_info mapsInf;
			GameMap::instance()->mapsInfo(&mapsInf);
			text.append(QString::fromUtf8("Всего найдено карт: <em>%1</em>").arg(mapsInf.maps_count), true);
			text.append(QString::fromUtf8("Всего загружено карт: <em>%1</em>").arg(mapsInf.maps_loaded), true);
			QString str1;
			int curMap = mapsInf.curr_map_index;
			if (curMap != -1) {
				str1 = QString::number(curMap) + " - " + mapsInf.curr_map_name;
			} else {
				str1 = NA_TEXT;
			}
			text.append(QString::fromUtf8("Текущая карта: <em>%1</em>").arg(str1), true);
			setConsoleText(text, 3, true);
		} else if (args.at(1) == "list") {
			GameMap::maps_info mapsInf;
			QVector<GameMap::maps_list2> mapsLst;
			GameMap *maps = GameMap::instance();
			maps->mapsInfo(&mapsInf);
			int currMap = mapsInf.curr_map_index;
			GameMap::instance()->getMapsList(&mapsLst);
			foreach (const GameMap::maps_list2 &ml, mapsLst)
			{
				const int idx = ml.index;
				QString str1 = Qt::escape(QString("%1 - %2%3%4")
					.arg(idx)
					.arg((idx == currMap) ? "*" : QString())
					.arg(ml.name)
					.arg((ml.loaded) ? QString::fromUtf8(" [загружена]") : QString()));
				if (ml.loaded)
				{
					str1 = "<font color=\"green\">" + str1 + "</font>";
				}
				if (idx == currMap)
				{
					str1 = "<strong>" + str1 + "</strong>";
				}
				text.append(str1, true);
			}
			text.append(QString::fromUtf8("Всего карт: ") + QString::number(mapsLst.size()), false);
			setConsoleText(text, 3, true);
		}
	} else if (cntArgs == 2) {
		if (args.at(1) == "switch") {
			bool fOk = false;
			int mapIndex = args.at(2).toInt(&fOk);
			if (fOk) {
				if (!GameMap::instance()->switchMap(mapIndex)) {
					fOk = false;
				}
			}
			if (fOk) {
				text.append(QString::fromUtf8("Переключено"), false);
			} else {
				text.append(QString::fromUtf8("Ошибка: карта не найдена"), false);
			}
			setConsoleText(text, 3, true);
			return;
		} else if (args.at(1) == "unload") {
			bool fOk = false;
			int mapIndex = args.at(2).toInt(&fOk);
			if (fOk) {
				if (!GameMap::instance()->unloadMap(mapIndex)) {
					fOk = false;
				}
			}
			if (fOk) {
				text.append(QString::fromUtf8("Выгружено"), false);
			} else {
				text.append(QString::fromUtf8("Ошибка: карта не найдена"), false);
			}
			setConsoleText(text, 3, true);
			return;
		}
	} else {
		text.append(QString::fromUtf8("Ошибка: неизвестный параметр"), false);
		setConsoleText(text, 3, true);
	}
}

PersInfo* PluginCore::getPersInfo(const QString &pers_name)
{
	/**
	* Возвращает указатель на объект PersInfo
	**/
	int persCnt = persInfoList.size();
	QString pers_name_l = pers_name.toLower();
	for (int persIndex = 0; persIndex < persCnt; persIndex++) {
		if (persInfoList[persIndex]) {
			if (persInfoList[persIndex]->getName().toLower() == pers_name_l) {
				return persInfoList[persIndex];
			}
		}
	}
	return 0;
}

void PluginCore::persCommands(const QStringList &args)
{
	/**
	* Отображение данных о персонаже
	**/
	if (args.at(0) != "/pers")
		return;
	int cntArgs = args.size() - 1;
	QString str1 = QString::fromUtf8("----=== Персонаж ===----");
	if (cntArgs == 0) {
		str1.append(QString::fromUtf8("\n/pers list — список имеющихся данных о персонажах"));
		str1.append(QString::fromUtf8("\n/pers info — краткая информация о собственном персонаже\n/pers info2 - подробная информация о собственном персонаже"));
		str1.append(QString::fromUtf8("\n/pers info <name> — краткая информация о персонаже <name>\n/pers info2 <name> - подробная информация о персонаже <name>"));
	} else if (cntArgs >= 1 && (args.at(1) == "info" || args.at(1) == "info2")) {
		int inf_ver = 1;
		if (args.at(1) == "info2") {
			inf_ver = 2;
		}
		str1.append(QString::fromUtf8("\n---- Общая информация ----"));
		if (cntArgs <= 2) {
			QString sPersName;
			if (cntArgs == 1) {
				// Наш персонаж
				sPersName = Pers::instance()->name();
			} else {
				// Персонаж указан в аргументе
				sPersName = args.at(2);
			}
			PersInfo* persInfo = getPersInfo(sPersName.toLower());
			if (persInfo) {
				str1.append(QString::fromUtf8("\nИмя персонажа: "));
				str1.append(persInfo->getName());
				str1.append(QString::fromUtf8("\nУровень: "));
				int num1;
				int force = 0;
				int dext = 0;
				int intell = 0;
				if (persInfo->getLevel(&num1)) {
					str1.append(QString::number(num1));
				} else {
					str1.append(NA_TEXT);
				}
				str1.append(QString::fromUtf8(", Здоровье: "));
				if (persInfo->getHealthMax(&num1)) {
					str1.append(QString::number(num1));
				} else {
					str1.append(NA_TEXT);
				}
				str1.append(QString::fromUtf8(", Энергия: "));
				if (persInfo->getEnergyMax(&num1)) {
					str1.append(QString::number(num1));
				} else {
					str1.append(NA_TEXT);
				}
				qint64 num3;
				str1.append(QString::fromUtf8(", Опыт: "));
				if (persInfo->getExperienceCurr(&num3)) {
					str1.append(numToStr(num3, "'"));
				} else {
					str1.append(NA_TEXT);
				}
				QString str2;
				str1.append(QString::fromUtf8("\nГражданство: "));
				if (!persInfo->citizenship().isEmpty())
					str1.append(persInfo->citizenship());
				else
					str1.append(QString::fromUtf8("нет"));
				str1.append(QString::fromUtf8("\nКлан: "));
				if (persInfo->getClan(&str2)) {
					str1.append(str2);
				} else {
					str1.append(NA_TEXT);
				}
				str1.append(QString::fromUtf8("\nРейтинг: "));
				if (persInfo->getRating(&num1)) {
					str1.append(QString::number(num1));
				} else {
					str1.append(NA_TEXT);
				}
				int num2;
				str1.append(QString::fromUtf8("\nСила: "));
				if (persInfo->getForce(&num1, &num2)) {
					str1.append(QString::number(num1) + "[" + QString::number(num2) + "]");
					force = num1;
				} else {
					str1.append(NA_TEXT);
				}
				str1.append(QString::fromUtf8("\nЛовкость: "));
				if (persInfo->getDext(&num1, &num2)) {
					str1.append(QString::number(num1) + "[" + QString::number(num2) + "]");
					dext = num1;
				} else {
					str1.append(NA_TEXT);
				}
				str1.append(QString::fromUtf8("\nИнтеллект: "));
				if (persInfo->getIntell(&num1, &num2)) {
					str1.append(QString::number(num1) + "[" + QString::number(num2) + "]");
					intell = num1;
				} else {
					str1.append(NA_TEXT);
				}
				str1.append(QString::fromUtf8("\nСуммарный урон экипировки: "));
				if (persInfo->getEquipLoss1(&num1)) {
					str1.append(numToStr(num1, "'"));
					if (persInfo->getEquipLoss2(&num2)) {
						if (num1 != num2) {
							str1.append(" / " + numToStr(num2, "'"));
						}
					} else {
						str1.append(" / ?");
					}
				} else {
					str1.append(NA_TEXT);
				}
				str1.append(QString::fromUtf8("\nСуммарная защита экипировки: "));
				if (persInfo->getEquipProtect1(&num1)) {
					str1.append(numToStr(num1, "'"));
					if (persInfo->getEquipProtect2(&num2)) {
						if (num1 != num2) {
							str1.append(" / " + numToStr(num2, "'"));
						}
					} else {
						str1.append(" / ?");
					}
				} else {
					str1.append(NA_TEXT);
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\n--- Экипировка ---"));
				int equipCount = 0;
				int namedCount = 0;
				int namedLevelAll = 0;
				int namedEffectCount = 0;
				float namedEffectAll = 0.0f;
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nОружие: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_WEAPON)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_WEAPON);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_WEAPON) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_WEAPON);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nЩит: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_SHIELD)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_SHIELD);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_SHIELD) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_SHIELD);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nГолова: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_HEAD)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_HEAD);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_HEAD) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_HEAD);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nШея: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_NECK)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_NECK);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_NECK) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_NECK);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nПлечи: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_SHOULDERS)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_SHOULDERS);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_SHOULDERS) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_SHOULDERS);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nРука 1: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_HAND1)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_HAND1);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_HAND1) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_HAND1);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nРука 2: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_HAND2)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_HAND2);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_HAND2) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_HAND2);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nКорпус: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_BODY)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_BODY);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_BODY) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_BODY);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nПояс: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_STRAP)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_STRAP);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_STRAP) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_STRAP);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nНоги: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_FEET)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_FEET);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_FEET) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_FEET);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				if (inf_ver == 2) str1.append(QString::fromUtf8("\nОбувь: "));
				if (persInfo->isEquipElement(PERS_INFO_EQUIP_SHOES)) {
					float namedEffect = persInfo->calculateEquipEfficiency(PERS_INFO_EQUIP_SHOES);
					if (inf_ver == 2) str1.append(persInfo->getEquipString(PERS_INFO_EQUIP_SHOES) + QString::fromUtf8(". Эфф: %1").arg(floor(namedEffect + 0.51f)));
					equipCount++;
					int level = persInfo->isEquipNamed(PERS_INFO_EQUIP_SHOES);
					if (level > 0) {
						namedCount++;
						namedLevelAll += level;
					}
					if (level >= 7) {
						namedEffectCount++;
						namedEffectAll += namedEffect;
					}
				} else {
					if (inf_ver == 2) str1.append(QString::fromUtf8("нет"));
				}
				str1.append(QString::fromUtf8("\nВещей одето: ") + QString::number(equipCount));
				if (equipCount > 0) {
					str1.append(QString::fromUtf8(", из них именных: ") + QString::number(namedCount));
				}
				str1.append(QString::fromUtf8("\nОбщий уровень именных: %1").arg(namedLevelAll));
				str1.append(QString::fromUtf8("\nОбщая эффективность именных: %1 / %2").arg(numToStr(floor(namedEffectAll + 0.51f), "'")).arg(namedEffectCount));
				if (inf_ver == 2) {
					str1.append(QString::fromUtf8("\n--- Долевой вклад экипировки ---"));
					// Считаем
					int force_sum = force;
					int dext_sum = dext;
					int intell_sum = intell;
					int equip_loss_sum = 0;
					int equip_protect_sum = 0;
					// Оружие
					struct params_info weaponParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_WEAPON, &weaponParams);
					force_sum += weaponParams.force;
					dext_sum += weaponParams.dext;
					intell_sum += weaponParams.intell;
					equip_loss_sum += weaponParams.equip_loss;
					equip_protect_sum += weaponParams.equip_protect;
					// Щит
					struct params_info shieldParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_SHIELD, &shieldParams);
					force_sum += shieldParams.force;
					dext_sum += shieldParams.dext;
					intell_sum += shieldParams.intell;
					equip_loss_sum += shieldParams.equip_loss;
					equip_protect_sum += shieldParams.equip_protect;
					// Голова
					struct params_info headParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_HEAD, &headParams);
					force_sum += headParams.force;
					dext_sum += headParams.dext;
					intell_sum += headParams.intell;
					equip_loss_sum += headParams.equip_loss;
					equip_protect_sum += headParams.equip_protect;
					// Шея
					struct params_info neckParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_NECK, &neckParams);
					force_sum += neckParams.force;
					dext_sum += neckParams.dext;
					intell_sum += neckParams.intell;
					equip_loss_sum += neckParams.equip_loss;
					equip_protect_sum += neckParams.equip_protect;
					// Плечи
					struct params_info shouldersParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_SHOULDERS, &shouldersParams);
					force_sum += shouldersParams.force;
					dext_sum += shouldersParams.dext;
					intell_sum += shouldersParams.intell;
					equip_loss_sum += shouldersParams.equip_loss;
					equip_protect_sum += shouldersParams.equip_protect;
					// Рука 1
					struct params_info hand1Params;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_HAND1, &hand1Params);
					force_sum += hand1Params.force;
					dext_sum += hand1Params.dext;
					intell_sum += hand1Params.intell;
					equip_loss_sum += hand1Params.equip_loss;
					equip_protect_sum += hand1Params.equip_protect;
					// Рука 2
					struct params_info hand2Params;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_HAND2, &hand2Params);
					force_sum += hand2Params.force;
					dext_sum += hand2Params.dext;
					intell_sum += hand2Params.intell;
					equip_loss_sum += hand2Params.equip_loss;
					equip_protect_sum += hand2Params.equip_protect;
					// Корпус
					struct params_info bodyParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_BODY, &bodyParams);
					force_sum += bodyParams.force;
					dext_sum += bodyParams.dext;
					intell_sum += bodyParams.intell;
					equip_loss_sum += bodyParams.equip_loss;
					equip_protect_sum += bodyParams.equip_protect;
					// Пояс
					struct params_info strapParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_STRAP, &strapParams);
					force_sum += strapParams.force;
					dext_sum += strapParams.dext;
					intell_sum += strapParams.intell;
					equip_loss_sum += strapParams.equip_loss;
					equip_protect_sum += strapParams.equip_protect;
					// Ноги
					struct params_info feetParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_FEET, &feetParams);
					force_sum += feetParams.force;
					dext_sum += feetParams.dext;
					intell_sum += feetParams.intell;
					equip_loss_sum += feetParams.equip_loss;
					equip_protect_sum += feetParams.equip_protect;
					// Обувь
					struct params_info shoesParams;
					persInfo->calculateEquipParams(PERS_INFO_EQUIP_SHOES, &shoesParams);
					force_sum += shoesParams.force;
					dext_sum += shoesParams.dext;
					intell_sum += shoesParams.intell;
					equip_loss_sum += shoesParams.equip_loss;
					equip_protect_sum += shoesParams.equip_protect;
					// Теперь считаем доли
					// Урон экипировки
					if (equip_loss_sum > 0) {
						str1.append(QString::fromUtf8("\n- Урон экипировки -"));
						int equip_loss = weaponParams.equip_loss;
						str1.append(QString::fromUtf8("\nОружие: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = shieldParams.equip_loss;
						str1.append(QString::fromUtf8("\nЩит: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = headParams.equip_loss;
						str1.append(QString::fromUtf8("\nГолова: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = neckParams.equip_loss;
						str1.append(QString::fromUtf8("\nШея: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = shouldersParams.equip_loss;
						str1.append(QString::fromUtf8("\nПлечи: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = hand1Params.equip_loss;
						str1.append(QString::fromUtf8("\nРука 1: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = hand2Params.equip_loss;
						str1.append(QString::fromUtf8("\nРука 2: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = bodyParams.equip_loss;
						str1.append(QString::fromUtf8("\nКорпус: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = strapParams.equip_loss;
						str1.append(QString::fromUtf8("\nПояс: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = feetParams.equip_loss;
						str1.append(QString::fromUtf8("\nНоги: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_loss = shoesParams.equip_loss;
						str1.append(QString::fromUtf8("\nОбувь: ") + QString::number(equip_loss) + " | " +QString::number(floor((float)equip_loss / (float)equip_loss_sum * 1000.0f + 0.5f) / 10.0f) + "%");
					}
					// Защита экипировки
					if (equip_protect_sum > 0) {
						str1.append(QString::fromUtf8("\n- Защита экипировки -"));
						int equip_protect = weaponParams.equip_protect;
						str1.append(QString::fromUtf8("\nОружие: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = shieldParams.equip_protect;
						str1.append(QString::fromUtf8("\nЩит: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = headParams.equip_protect;
						str1.append(QString::fromUtf8("\nГолова: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = neckParams.equip_protect;
						str1.append(QString::fromUtf8("\nШея: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = shouldersParams.equip_protect;
						str1.append(QString::fromUtf8("\nПлечи: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = hand1Params.equip_protect;
						str1.append(QString::fromUtf8("\nРука 1: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = hand2Params.equip_protect;
						str1.append(QString::fromUtf8("\nРука 2: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = bodyParams.equip_protect;
						str1.append(QString::fromUtf8("\nКорпус: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = strapParams.equip_protect;
						str1.append(QString::fromUtf8("\nПояс: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = feetParams.equip_protect;
						str1.append(QString::fromUtf8("\nНоги: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						equip_protect = shoesParams.equip_protect;
						str1.append(QString::fromUtf8("\nОбувь: ") + QString::number(equip_protect) + " | " +QString::number(floor((float)equip_protect / (float)equip_protect_sum * 1000.0f + 0.5f) / 10.0f) + "%");
					}
					// Сила
					if (force_sum > 0) {
						str1.append(QString::fromUtf8("\n- Сила -"));
						str1.append(QString::fromUtf8("\nРаспределение: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = weaponParams.force;
						str1.append(QString::fromUtf8("\nОружие: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = shieldParams.force;
						str1.append(QString::fromUtf8("\nЩит: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = headParams.force;
						str1.append(QString::fromUtf8("\nГолова: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = neckParams.force;
						str1.append(QString::fromUtf8("\nШея: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = shouldersParams.force;
						str1.append(QString::fromUtf8("\nПлечи: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = hand1Params.force;
						str1.append(QString::fromUtf8("\nРука 1: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = hand2Params.force;
						str1.append(QString::fromUtf8("\nРука 2: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = bodyParams.force;
						str1.append(QString::fromUtf8("\nКорпус: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = strapParams.force;
						str1.append(QString::fromUtf8("\nПояс: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = feetParams.force;
						str1.append(QString::fromUtf8("\nНоги: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						force = shoesParams.force;
						str1.append(QString::fromUtf8("\nОбувь: ") + QString::number(force) + " | " +QString::number(floor((float)force / (float)force_sum * 1000.0f + 0.5f) / 10.0f) + "%");
					}
					// Ловкость
					if (dext_sum > 0) {
						str1.append(QString::fromUtf8("\n- Ловкость -"));
						str1.append(QString::fromUtf8("\nРаспределение: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = weaponParams.dext;
						str1.append(QString::fromUtf8("\nОружие: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = shieldParams.dext;
						str1.append(QString::fromUtf8("\nЩит: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = headParams.dext;
						str1.append(QString::fromUtf8("\nГолова: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = neckParams.dext;
						str1.append(QString::fromUtf8("\nШея: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = shouldersParams.dext;
						str1.append(QString::fromUtf8("\nПлечи: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = hand1Params.dext;
						str1.append(QString::fromUtf8("\nРука 1: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = hand2Params.dext;
						str1.append(QString::fromUtf8("\nРука 2: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = bodyParams.dext;
						str1.append(QString::fromUtf8("\nКорпус: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = strapParams.dext;
						str1.append(QString::fromUtf8("\nПояс: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = feetParams.dext;
						str1.append(QString::fromUtf8("\nНоги: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						dext = shoesParams.dext;
						str1.append(QString::fromUtf8("\nОбувь: ") + QString::number(dext) + " | " +QString::number(floor((float)dext / (float)dext_sum * 1000.0f + 0.5f) / 10.0f) + "%");
					}
					// Интеллект
					if (intell_sum > 0) {
						str1.append(QString::fromUtf8("\n- Интеллект -"));
						str1.append(QString::fromUtf8("\nРаспределение: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = weaponParams.intell;
						str1.append(QString::fromUtf8("\nОружие: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = shieldParams.intell;
						str1.append(QString::fromUtf8("\nЩит: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = headParams.intell;
						str1.append(QString::fromUtf8("\nГолова: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = neckParams.intell;
						str1.append(QString::fromUtf8("\nШея: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = shouldersParams.intell;
						str1.append(QString::fromUtf8("\nПлечи: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = hand1Params.intell;
						str1.append(QString::fromUtf8("\nРука 1: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = hand2Params.intell;
						str1.append(QString::fromUtf8("\nРука 2: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = bodyParams.intell;
						str1.append(QString::fromUtf8("\nКорпус: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = strapParams.intell;
						str1.append(QString::fromUtf8("\nПояс: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = feetParams.intell;
						str1.append(QString::fromUtf8("\nНоги: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
						intell = shoesParams.intell;
						str1.append(QString::fromUtf8("\nОбувь: ") + QString::number(intell) + " | " +QString::number(floor((float)intell / (float)intell_sum * 1000.0f + 0.5f) / 10.0f) + "%");
					}
				}
				//--
				str1.append("\n");
			} else {
				str1.append(QString::fromUtf8("\nНет информации.\n"));
			}
		} else {
			str1.append(QString::fromUtf8("\nНеверное количество аргументов.\n"));
		}
	} else if (cntArgs == 1 && args.at(1) == "list") {
		str1.append(QString::fromUtf8("\n--- Данные о персонажах ---"));
		int persCnt = persInfoList.size();
		if (persCnt > 0) {
			for (int persIndex = 0; persIndex < persCnt; persIndex++) {
				str1.append("\n" + QString::number(persIndex + 1) + " - " + persInfoList[persIndex]->getName());
			}
			str1.append(QString::fromUtf8("\nВсего записей: ") + QString::number(persCnt));
		} else {
			str1.append(QString::fromUtf8("\nнет данных."));
		}
		str1.append("\n");
	}
	setConsoleText(GameText(str1, false), 3, true);
}

void PluginCore::clearCommands(const QStringList &args)
{
	/**
	* Обработка команд уровня clear
	**/
	if (args.at(0) != "/clear")
		return;
	int argsCount = args.size() - 1;
	GameText text;
	if (argsCount > 0) {
		if (args.at(1) == "text") {
			if (argsCount == 2) {
				if (args.at(2) == "all") {
					setGameText(text, 3);
					setConsoleText(GameText(QString(), false), 3, false);
					return;
				}
				if (args.at(2) == "game") {
					setGameText(text, 3);
					setConsoleText(GameText(QString::fromUtf8("Выполнено"), false), 3, false);
					return;
				}
				if (args.at(2) == "console") {
					setConsoleText(text, 3, false);
					return;
				}
			}
			text.append(QString::fromUtf8("<strong>/clear text all</strong> — Очистка окон вывода плагина и консоли"), true);
			text.append(QString::fromUtf8("<strong>/clear text console</strong> — Очистка окна вывода плагина"), true);
			text.append(QString::fromUtf8("<strong>/clear text game</strong> — Очистка окна вывода игровых данных"), true);
		} else if (args.at(1) == "stat") {
			int level = 0;
			if (argsCount >= 2) {
				level = args.at(2).toInt();
			}
			Statistic *stat = Statistic::instance();
			if (level == 0 || level == 1) {
				stat->setValue(Statistic::StatLastGameJid, QVariant());
				stat->setValue(Statistic::StatLastChatJid, QVariant());
				stat->setValue(Statistic::StatMessagesCount, QVariant());
				text.append(QString::fromUtf8("Общая статистика сброшена"), false);
			}
			if (level == 0 || level == 2) {
				stat->setValue(Statistic::StatFightsCount, QVariant());
				stat->setValue(Statistic::StatDamageMaxFromPers, QVariant());
				stat->setValue(Statistic::StatDamageMinFromPers, QVariant());
				stat->setValue(Statistic::StatDropMoneys, QVariant());
				stat->setValue(Statistic::StatThingsDropCount, QVariant());
				stat->setValue(Statistic::StatThingDropLast, QVariant());
				stat->setValue(Statistic::StatExperienceDropCount, QVariant());
				stat->setValue(Statistic::StatKilledEnemies, QVariant());
				text.append(QString::fromUtf8("Статистика боев сброшена"), false);
			}
		}
	}
	if (text.isEmpty()) {
		text.append(QString::fromUtf8("<strong>/clear text</strong> — Очистка окна вывода игровых данных"), true);
		text.append(QString::fromUtf8("<strong>/clear stat</strong> — Сброс всей статистики"), true);
		text.append(QString::fromUtf8("<strong>/clear stat n</strong> — Сброс статистики группы n"), true);
	}
	setConsoleText(text, 3, true);
}

/**
 * Обработка команд уровня things
 */
void PluginCore::thingsCommands(const QStringList &args)
{
	if (args.at(0) != "/things")
		return;
	int argsCount = args.size() - 1;
	GameText text;
	text.append(QString::fromUtf8("<big><strong><em>----=== Вещи ===----</em></strong></big>"), true);
	if (argsCount == 0) {
		text.append(QString::fromUtf8("<strong>/things list</strong> или <strong>/things list 0</strong> — отображение всех вещей"), true);
		text.append(QString::fromUtf8("<strong>/things list n</strong> — отображение вещей фильтра n"), true);
		text.append(QString::fromUtf8("<strong>/things filters list</strong> — отображение фильтров"), true);
		text.append(QString::fromUtf8("<strong>/things price list</strong> — отображение цен на вещи"), true);
	} else if (args.at(1) == "list") {
		int filterNum = 0;
		bool fOk = true;
		if (argsCount == 2) {
			filterNum = args.at(2).toInt(&fOk);
		}
		if (fOk && filterNum >= 0) {
			const ThingFiltersList filtersList = Pers::instance()->thingsFiltersList();
			if (filterNum <= filtersList.size()) {
				int iface = Pers::instance()->getThingsInterface();
				QString sFltrName;
				if (filterNum == 0) {
					sFltrName = QString::fromUtf8("Все вещи");
				} else {
					ThingFilter const *thf = filtersList.at(filterNum);
					if (thf) {
						sFltrName = QString::fromUtf8("Вещи из фильтра \"%1\"").arg(thf->name().trimmed());
					} else {
						sFltrName = QString::fromUtf8("Все вещи");
						filterNum = -1;
					}
					Pers::instance()->setThingsInterfaceFilter(iface, filterNum);
				}
				text.append(QString::fromUtf8("<strong><em>--- %1 ---</em></strong>").arg(sFltrName), true);
				int thingsCnt = 0;
				while (true) {
					const Thing* thing = Pers::instance()->getThingByRow(thingsCnt, iface);
					if (!thing)
						break;
					text.append(thing->toString(Thing::ShowAll), false);
					thingsCnt++;
				}
				if (thingsCnt > 0) {
					int nCountAll = Pers::instance()->getThingsCount(iface);
					int nPriceAll = Pers::instance()->getPriceAll(iface);
					text.append(QString::fromUtf8("Всего предметов: %1, на сумму: %2 дринк.").arg(numToStr(nCountAll, "'")).arg(numToStr(nPriceAll, "'")), false);
					int noPrice = Pers::instance()->getNoPriceCount(iface);
					if (noPrice > 0) {
						text.append(QString::fromUtf8("Предметов без цены: %1.").arg(numToStr(noPrice, "'")), true);
					}
				} else {
					text.append(QString::fromUtf8("список вещей пуст"), false);
				}
				Pers::instance()->removeThingsInterface(iface);
			} else {
				text.append(QString::fromUtf8("нет такого фильтра. См. <strong>/things filters list</strong>"), true);
			}
		} else {
			text.append(QString::fromUtf8("необходимо указать номер фильтра. См. <strong>/things filters list</strong>"), true);
		}
	} else if (args.at(1) == "filters") {
		text.append(QString::fromUtf8("<strong><em>--- Фильтры вещей ---</em></strong>"), true);
		if (argsCount == 2 && args.at(2) == "list") {
			ThingFiltersList filtersList = Pers::instance()->thingsFiltersList();
			int cntFilters = filtersList.size();
			if (cntFilters > 0) {
				for (int i = 0, cnt = filtersList.size(); i < cnt; ++i) {
					ThingFilter const *thf = filtersList.at(i);
					if (thf) {
						QString str1 = QString("%1 - [%2] %3")
							.arg(i+1)
							.arg(thf->isActive() ? "+" : "-")
							.arg(thf->name());
						text.append(str1, false);
					}
				}
				text.append(QString::fromUtf8("Всего фильтров: %1").arg(cntFilters), true);
			} else {
				text.append(QString::fromUtf8("нет фильтров"), false);
			}
		} else {
			text.append(QString::fromUtf8("неверный аргумент"), false);
		}
	} else if (args.at(1) == "price") {
		text.append(QString::fromUtf8("<strong><em>--- Цены вещей ---</em></strong>"), true);
		if (argsCount == 2 && args.at(2) == "list") {
			const QVector<Pers::price_item>* priceListPrt = Pers::instance()->getThingsPrice();
			int sizePrice = priceListPrt->size();
			if (sizePrice > 0) {
				for (int i = 0; i < sizePrice; i++) {
					QString str1 = priceListPrt->at(i).name;
					QString sType = thingTypeToString(priceListPrt->at(i).type);
					if (!sType.isEmpty()) {
						str1.append("(" + sType + ")");
					}
					int nPrice = priceListPrt->at(i).price;
					str1.append(": \t");
					if (nPrice != -1) {
						str1.append(QString::number(nPrice));
					} else {
						str1.append("нет цены");
					}
					text.append(str1, false);
				}
				text.append(QString::fromUtf8("Всего позиций: %1").arg(numToStr(sizePrice, "'")), true);
			} else {
				text.append(QString::fromUtf8("нет цен"), false);
			}
		} else {
			text.append(QString::fromUtf8("неверный аргумент"), false);
		}
	} else {
		text.append(QString::fromUtf8("неверный аргумент"), false);
	}
	setConsoleText(text, 3, true);
}

/**
 * Обработка команд уровня aliases
 */
void PluginCore::aliasesCommands(const QStringList &args)
{
	if (args.at(0) != "/aliases")
		return;
	int argsCount = args.size() - 1;
	GameText text;
	text.append(QString::fromUtf8("<big><strong><em>----=== Алиасы ===----</em></strong></big>"), true);
	if (argsCount == 0) {
		text.append(QString::fromUtf8("<strong>/aliases list</strong> — список всех алиасов"), true);
		text.append(QString::fromUtf8("<strong>/aliases append &lt;имя&gt; &lt;префикс? yes|no&gt; &lt;command&gt;</strong> — Добавление нового алиаса"), true);
		text.append(QString::fromUtf8("<strong>/aliases remove &lt;alias_index&gt;</strong> — Удаление алиаса по номеру"), true);
	} else if (args.at(1) == "list") {
		text.append(QString::fromUtf8("<strong><em>--- Список алиасов ---</em></strong>"), true);
		Aliases *pAliases = Aliases::instance();
		int cnt = pAliases->count();
		if (cnt != 0) {
			text.append(QString::fromUtf8("&nbsp;&nbsp;Имя&nbsp;&nbsp;Префикс&nbsp;&nbsp;Команда"), true);
			for (int i = 0; i < cnt; i++) {
				text.append(QString::fromUtf8("&nbsp;&nbsp;%1 - %2&nbsp;&nbsp;%3&nbsp;&nbsp;%4")
					.arg(i+1)
					.arg(Qt::escape(pAliases->aliasName(i)))
					.arg(pAliases->aliasPrefix(i) ? QString::fromUtf8("да") : QString::fromUtf8("нет"))
					.arg(Qt::escape(pAliases->aliasCommand(i))), true);
			}
			text.append(QString::fromUtf8("Всего записей: %1").arg(cnt), false);
		} else {
			text.append(QString::fromUtf8("список пуст"), false);
		}
	} else if (args.at(1) == "append") {
		text.append(QString::fromUtf8("<strong><em>--- Добавление алиаса ---</em></strong>"), true);
		bool res = false;
		if (argsCount >= 4) {
			if (args.at(3) == "yes" || args.at(3) == "no") {
				QStringList aliasData = args.mid(4);
				if (Aliases::instance()->appendAlias(args.at(2), args.at(3) == "yes", aliasData.join(" "))) {
					text.append(QString::fromUtf8("Добавлено без сохранения"), false);
				} else {
					text.append(QString::fromUtf8("Ошибка!"), false);
				}
				res = true;
			}
		}
		if (!res) {
			text.append(QString::fromUtf8("<strong>/aliases append &lt;text&gt; &lt;yes|no&gt; &lt;text&gt;</strong>"), true);
		}
	} else if (args.at(1) == "remove") {
		text.append(QString::fromUtf8("<strong><em>--- Удаление алиаса ---</em></strong>"), true);
		bool res = false;
		if (argsCount == 2) {
			bool fOk = false;
			const int idx = args.at(2).toInt(&fOk);
			if (fOk) {
				if (Aliases::instance()->removeAlias(idx - 1)) {
					text.append(QString::fromUtf8("Удалено без сохранения"), false);
				} else {
					text.append(QString::fromUtf8("Ошибка!"), false);
				}
				res = true;
			}
		}
		if (!res) {
			text.append(QString::fromUtf8("<strong>/aliases remove &lt;alias_index&gt;</strong>"), true);
		}
	} else {
		text.append(QString::fromUtf8("неверный аргумент"), false);
	}
	setConsoleText(text, 3, true);
}

/**
 * Обработка команд уровня settings
 */
void PluginCore::settingsCommands(const QStringList &args)
{
	if (args.at(0) != "/settings")
		return;
	int argsCount = args.size() - 1;
	GameText text;
	text.append(QString::fromUtf8("<big><strong><em>----=== Настройки ===----</em></strong></big>"), true);
	if (argsCount == 0) {
		text.append(QString::fromUtf8("<strong>/settings save</strong> — Сохранение настроек плагина"), true);
	} else if (args.at(1) == "save") {
		text.append(QString::fromUtf8("<strong><em>--- Сохранение настроек ---</em></strong>"), true);
		Settings::instance()->save();
		text.append(QString::fromUtf8("Сохранено"), false);
	} else {
		text.append(QString::fromUtf8("неверный аргумент"), false);
	}
	setConsoleText(text, 3, true);
}

/**
 * Инициализация всплывающего окна
 */
void PluginCore::initPopup(const QString &string, int secs)
{
	// Устанавливаем свои настройки
	int msecs = secs * 1000;
	int delay_ = PluginHosts::psiOptions->getGlobalOption("options.ui.notifications.passive-popups.delays.status").toInt();
	bool enbl_ = PluginHosts::psiOptions->getGlobalOption("options.ui.notifications.passive-popups.enabled").toBool();
	if (delay_ != msecs) {
		QVariant delay(msecs);
		PluginHosts::psiOptions->setGlobalOption("options.ui.notifications.passive-popups.delays.status", delay);
	}
	if (!enbl_) {
		QVariant enbl(true);
		PluginHosts::psiOptions->setGlobalOption("options.ui.notifications.passive-popups.enabled", enbl);
	}
	// Вызываем popup
	PluginHosts::myPopupHost->initPopup(string, QString::fromUtf8("Sof game"));
	// Восстанавливаем настройки
	if (delay_ != msecs) {
		QVariant delay(delay_);
		PluginHosts::psiOptions->setGlobalOption("options.ui.notifications.passive-popups.delays.status", delay);
	}
	if (!enbl_) {
		QVariant enbl(false);
		PluginHosts::psiOptions->setGlobalOption("options.ui.notifications.passive-popups.enabled", enbl);
	}
}

void PluginCore::persParamChanged()
{
	persStatusChangedFlag = true;
	if (Settings::instance()->getIntSetting(Settings::SettingPersSaveMode) == 2) { // Автосохранение через 5 минут
		if (!saveStatusTimer.isActive()) {
			saveStatusTimer.start(5000*60);
		}
	}
}

void PluginCore::persBackpackChanged()
{
	persBackpackChangedFlag = true;
	if (Settings::instance()->getIntSetting(Settings::SettingPersSaveMode) == 2) { // Автосохранение через 5 минут
		if (!saveStatusTimer.isActive()) {
			saveStatusTimer.start(5000*60);
		}
	}
}

void PluginCore::statisticsChanged()
{
	statisticChangedFlag = true;
	if (Settings::instance()->getIntSetting(Settings::SettingPersSaveMode) == 2) { // Автосохранение через 5 минут
		if (!saveStatusTimer.isActive()) {
			saveStatusTimer.start(5000*60);
		}
	}
}

/**
 * Обработка сигнала таймера автосохранения статуса игрока и статистики
 */
void PluginCore::saveStatusTimeout()
{
	if (persStatusChangedFlag || persBackpackChangedFlag || statisticChangedFlag) {
		savePersStatus();
	}
}

void PluginCore::updateSetting(Settings::SettingKey key)
{
	if (key == Settings::SettingGameTextColoring) {
		fightColoring = Settings::instance()->getBoolSetting(key);
	}
}
