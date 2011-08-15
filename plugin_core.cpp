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


// Глобальные переменные (знаю что не кошерно, зато работает быстро и... удобно)
PopupAccessingHost *myPopupHost;
OptionAccessingHost* psiOptions;

// Глобальные константы
QStringList valueXmlStrings = (QStringList() << "last-game-jid" << "last-chat-jid" << "messages-count" << "damage-max-from-pers" << "damage-min-from-pers" << "fights-count" << "drop-moneys" << "fings-drop-count" << "fing-drop-last" << "experience-drop-count" << "killed-enemies");
QStringList persSaveModeStrings = (QStringList() << "none" << "save-at-exit" << "each-5-min-if-change");
QStringList mirrorChangeModeStrings = (QStringList() << "first-accessible" << "auto-select-optimal");
QStringList fightTimerStrings = (QStringList() << "not-to-display" << "seconds-only" << "minutes-and-seconds");
QStringList fightAutoCloseStrings = (QStringList() << "not-close" << "if-one-opposite-mob");
QStringList watchRestHealthEnergyStrings = (QStringList() << "no-watch" << "intell-watch" << "each-10-secs" << "each-30-secs" << "each-60-secs");
QStringList fightSelectActionStrings = (QStringList() << "no-action" << "new-if-queue-not-empty" << "always-new");
//-----

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
	mainWindow = 0;
	fight = new Fight();
	// Сброс статусов изменений
	persStatusChangedFlag = false;
	persBackpackChangedFlag = false;
	statisticChangedFlag = false;
	// Настройка регулярок
	mapCoordinatesExp.setPattern(QString::fromUtf8("^(-?[0-9]+):(-?[0-9]+)$"));
	parPersRegExp.setPattern(QString::fromUtf8("^(\\w+)\\[у:([0-9]{1,2})\\|з:(-?[0-9]{1,6})/([0-9]{1,6})\\|э:(-?[0-9]{1,6})/([0-9]{1,6})\\|о:([0-9]{1,12})/([0-9]{1,12})\\]$"));
	fightDropMoneyReg2.setPattern(QString::fromUtf8("(нашли|выпало) ([0-9]+) дринк"));
	secretDropFingReg.setPattern(QString::fromUtf8("(забрали|нашли|выпала) \"(.+)\"")); // забрали "панцирь раритет" и ушли
	experienceDropReg.setCaseSensitivity(Qt::CaseInsensitive);
	experienceDropReg.setPattern(QString::fromUtf8("^Очки опыта:? \\+([0-9]+)$")); // очки опыта +1500 || Очки опыта: +50000
	experienceDropReg2.setPattern(QString::fromUtf8("^\\+([0-9]+) опыта\\.?")); // +4800 опыта за разбитый ящик.
	secretBeforeReg.setPattern(QString::fromUtf8("^[0-9]- обыскать( тайник| сокровищницу)?$"));  // 1- обыскать тайник
	secretBeforeReg2.setPattern(QString::fromUtf8("^[0-9]- разбить ящик$"));  // 1- разбить ящик
	takeBeforeReg.setPattern(QString::fromUtf8("^[0-9]- забрать$"));  // 1- забрать
	commandStrReg.setPattern(QString::fromUtf8("^([0-9])- (.+)$")); // Команды перемещения
	fingElementReg.setPattern(QString::fromUtf8("^([0-9]+)- (.+)\\((\\w+)\\)((.+)\\{(.+)\\})?(И:([0-9]+)ур\\.)?(- ([0-9]+)шт\\.)?$")); // Вещь
	persInfoReg.setPattern(QString::fromUtf8("^Характеристики (героя|героини): (\\w+)( \\{\\w+\\})?$"));
	persInfoMainReg.setPattern(QString::fromUtf8("^Уровень:([0-9]+), Здоровье:(-?[0-9]+)/([0-9]+), Энергия:(-?[0-9]+)/([0-9]+), Опыт:([0-9]+)(\\.|, Ост. до след уровня:([0-9]+))$")); // Уровень:25, Здоровье:6051/6051, Энергия:7956/7956, Опыт:186821489, Ост. до след уровня:133178511
	persInfoSitizenshipReg.setPattern(QString::fromUtf8("^Гражданство: (.+)$")); // Гражданство: город "Вольный"
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
	// --
	fightOneTimeoutReg.setPattern((QString::fromUtf8("^до завершения хода№([0-9]+): ([0-9]+)мин\\.([0-9]+)сек\\.$"))); // Номер хода и таймаут
	fightElement0Reg.setPattern(QString::fromUtf8("([^/]+)(/(.+)/)?\\[(у:([0-9]+)\\,з:)?([0-9]+)/([0-9]+)\\]")); // Боевые единицы (союзники)
	//demon/В/[у:27,з:9971/12464]
	fightElement1Reg.setPattern(QString::fromUtf8("([0-9]+)- ([^/]+)(/(.+)/)?\\[(у:([0-9]+)\\,з:)?([0-9]+)/([0-9]+)\\]")); // Боевые единицы (противники)
	fightElement2Reg.setPattern(QString::fromUtf8("(.+)\\[(.+):(-?[0-9]+)\\]")); // Ауры в бою
	fightElement0Reg.setMinimal(true);
	fightElement1Reg.setMinimal(true);
	fightElement2Reg.setMinimal(true);
	parPersPower1Reg.setPattern(QString::fromUtf8("^Энергия:(-?[0-9]{1,6})/([0-9]{1,6})$"));
	// Соединения
	if (mySender) {
		connect(mySender, SIGNAL(errorOccurred(int)), this, SLOT(processError(int)));
		connect(mySender, SIGNAL(gameTextReceived(QString*,QString*)), this, SLOT(textParsing(QString*, QString*)));
		connect(mySender, SIGNAL(accountChanged(QString)), this, SLOT(changeAccountJid(QString)));
	}
	connect(Pers::instance(), SIGNAL(persParamChanged(int, int, int)), this, SLOT(persParamChanged()));
	connect(Pers::instance(), SIGNAL(fingsChanged()), this, SLOT(persBackpackChanged()));
	connect(&saveStatusTimer, SIGNAL(timeout()), this, SLOT(saveStatusTimeout()));
}

PluginCore::~PluginCore()
{
	if (saveStatusTimer.isActive())
		saveStatusTimer.stop();
	disconnect(mySender, SIGNAL(errorOccurred(int)), this, SLOT(processError(int)));
	disconnect(mySender, SIGNAL(gameTextReceived(QString*,QString*)), this, SLOT(textParsing(QString*, QString*)));
	if (mainWindow) {
		delete mainWindow;
	}
	GameMap::reset();
	if (fight) {
		delete fight;
	}
	savePersStatus();
	Pers::reset();
	Aliases::reset();
	int cnt = persInfoList.size();
	for (int i = 0; i < cnt; i++) {
		if (persInfoList[i])
			delete persInfoList[i];
	}
	saveWindowSettings();
}

void PluginCore::updateRegExpForPersName()
{
	// Функция настраивает регулярные выражения, зависящие от имени персонажа
	QString sName = QRegExp::escape(Pers::instance()->name());
	fightDamageFromPersReg1.setPattern("^" + sName + QString::fromUtf8(" .+ Повреждения: ([0-9]+)$"));
	fightDamageFromPersReg2.setPattern("^" + sName + QString::fromUtf8("\\*([0-9]+) .+/повр:([0-9]+)$"));
	fightDamageFromPersReg3.setPattern("^" + sName + QString::fromUtf8(" .+ нет повреждений$"));
	fightDropMoneyReg1.setPattern("^" + sName + QString::fromUtf8(" ([-+][0-9]+) дринк$")); // xxxxx +5 дринк;
	fightDropThingReg1.setPattern("^" + sName + QString::fromUtf8(" \\+(.+)$")); // xxxxx +шкура мат. волка

}

void PluginCore::doShortCut()
{
	if (!mainWindow) {
		mainWindow = new SofMainWindow();
	}
	mainWindow->show();
}

void PluginCore::changeAccountJid(QString newJid)
{
	// *** В настройках плагина сменился игровой аккаунт ***
	if (!accJid.isEmpty()) {
		// Сохраняем статус текущего персонажа
		savePersStatus();
	}
	accJid = newJid;
	// Сбрасываем объект персонажа
	Pers::instance()->init();
	// Загрузить новые настройки
	loadPluginSettings();
	// Загрузить новые данные персонажа
	loadPersStatus();
	// Обновить регулярки зависящие он имени персонажа
	updateRegExpForPersName();
	// Перегрузить карты
	GameMap::instance()->init(accJid);
	GameMap::instance()->setMapsParams(GameMap::AutoSaveMode, settingMapsSaveMode);
	// Уведомить окно о переключении персонажа
	if (mainWindow)
		mainWindow->init();
	return;
}

void PluginCore::setAccountJidStatus(qint32 status)
{
	// *** Смена статуса игрового аккаунта *** //  И как это радостное событие отловить, а?
	// Если аккаунт отключен, то устанавливаем игровым jid-ам статус отключенных
	setConsoleText(QString::fromUtf8("### Аккаунт отключен ###"), false);
	mySender->setAccountStatus(status);
}

void PluginCore::setGameJidStatus(int jid_index, qint32 status)
{
	if (mySender->setGameJidStatus(jid_index, status)) {
		const Sender::jid_status* jstat = mySender->getGameJidInfo(jid_index);
		QString str1 = "### " + jstat->jid + " - ";
		if (status == 0) {
			str1.append(QString::fromUtf8("отключен"));
		} else {
			str1.append(QString::fromUtf8("подключен"));
		}
		str1.append(" ###");
		setConsoleText(str1, false);
	}
}

bool PluginCore::textParsing(QString* jid, QString* message)
{
	bool myMessage = false; // Пока считаем что сообщение послано не нами
//	if (mySender->doGameAsk(&jid, &message)) { // Сначала сообщение обрабатывает Sender
//		return true;
//	}
	// Проверяем открыто ли окно
	if (mainWindow && mainWindow->isVisible()) {
		myMessage = true;
	}
	//--
	if (lastGameJid != jid) {
		lastGameJid = *jid;
		valueChanged(VALUE_LAST_GAME_JID, TYPE_STRING, 0);
	}
	++statMessagesCount;
	valueChanged(VALUE_MESSAGES_COUNT, TYPE_INTEGER_FULL, statMessagesCount);
	// Распускаем полученное сообщение построчно
	QStringList aMessage = message->split("\n");
	// Просматриваем данные построчно
	PersStatus nPersStatus = NotKnow;
	//int nParamLine = -1;
	bool fName = false; QString sName = 0;
	bool fLevel = false; int nLevel = 0;
	bool fExperience = false; qint64 nExperience = 0; qint64 nExperienceFull = 0;
	bool fHealth = false; int nHealthC = 0; int nHealthF = 0;
	bool fPower = false; int nPowerC = 0; int nPowerF = 0;
	bool fDropMoneys = false; int nDropMoneys = 0;
	bool fStatFightDamageMin = false; bool fStatFightDamageMax = false;
	int nFingsDropCount = 0; QString sFingDropLast;
	bool fExperienceDrop = false;
	int nTimeout = -1;
	//bool fFight = false;
	int nCmdStatus = 0;
	int nCount = aMessage.count();
	QStringList oTempStrList;
	QString sMessage;
	int startStr = 0;
	// Проверяем наличие координат // 210:290
	int persX = QINT32_MIN;
	int persY = QINT32_MIN;
	bool fPersPosChanged = false;
	bool fNewFight = false;
	if (mapCoordinatesExp.indexIn(aMessage[0], 0) != -1) {
		// Найдены координаты местоположения на карте
		startStr = 1;
		persX = mapCoordinatesExp.cap(1).toInt();
		persY = mapCoordinatesExp.cap(2).toInt();
		if (persX != settingPersX || persY != settingPersY) {
			// Изменились координаты персонажа
			fPersPosChanged = true;
			settingPersX = persX;
			settingPersY = persY;
			// Добавляем элемент в карту
			GameMap::instance()->addMapElement(persX, persY);
			// Устанавливаем новую позицию персонажа
			GameMap::instance()->setPersPos(persX, persY);
		}
	}
	int i = 0;
	while (i < nCount) {
		sMessage = aMessage.at(i).trimmed(); // Убираем пробельные символы в начале и в конце строки
		if (i == startStr) { // Первая строка в выборке
			if (parPersRegExp.indexIn(sMessage, 0) != -1) { // Проверяем наличие строки с полными параметрами персонажа
				fName = true; sName = parPersRegExp.cap(1);
				fLevel = true; nLevel = parPersRegExp.cap(2).toInt();
				fExperience = true; nExperience = parPersRegExp.cap(7).toLongLong(); nExperienceFull = parPersRegExp.cap(8).toLongLong() + nExperience;
				fHealth = true; nHealthC = parPersRegExp.cap(3).toInt(); nHealthF = parPersRegExp.cap(4).toInt();
				fPower = true; nPowerC = parPersRegExp.cap(5).toInt(); nPowerF = parPersRegExp.cap(6).toInt();
				//nParamLine = i; // Запомнили строку с полными параметрами персонажа
				nPersStatus = Stand;
			} else if (sMessage.startsWith(QString::fromUtf8("Бой открыт"))) {
				nPersStatus = FightOpenBegin;
			} else if (sMessage.startsWith(QString::fromUtf8("Бой закрыт"))) {
				nPersStatus = FightCloseBegin;
			} else if (persInfoReg.indexIn(sMessage, 0) != -1) {
				// Информация о персонаже
				QString sPersName = persInfoReg.cap(2).trimmed();
				int nLevel = -1;
				int nHealthCurr = QINT32_MIN; int nHealthMax = QINT32_MIN;
				int nEnergyCurr = QINT32_MIN; int nEnergyMax = QINT32_MIN;
				qint64 nExperienceCurr = -1; qint64 nExperienceRemain = -1; bool fExperienceRemain = false;
				int nPower1 = QINT32_MIN; int nPower2 = QINT32_MIN; bool fPower = false;
				int nDext1 = QINT32_MIN; int nDext2 = QINT32_MIN; bool fDext = false;
				int nIntell1 = QINT32_MIN; int nIntell2 = QINT32_MIN; bool fIntell = false;
				int nLoss = -1; int nProtect = -1;
				QString sSitizenship;
				int nRating = QINT32_MIN;
				QString sClan;
				sMessage = aMessage[++i].trimmed();
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
					sMessage = aMessage[++i].trimmed();
				}
				if (persInfoSitizenshipReg.indexIn(sMessage, 0) != -1) {
					// Гражданство: город "Вольный"
					sSitizenship = persInfoSitizenshipReg.cap(1).trimmed();
					sMessage = aMessage[++i].trimmed();
				}
				if (persInfoClanReg.indexIn(sMessage, 0) != -1) {
					//  Клан: Лига Теней/ЛТ/
					sClan = persInfoClanReg.cap(1).trimmed();
					sMessage = aMessage[++i].trimmed();
				}
				if (persInfoRatingReg.indexIn(sMessage, 0) != -1) {
					// Рейтинг: 5 место.
					nRating = persInfoRatingReg.cap(1).toInt();
					sMessage = aMessage[++i].trimmed();
				}
				if (sMessage == QString::fromUtf8("Распределение:")) {
					sMessage = aMessage[++i].trimmed();
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
						sMessage = aMessage[++i].trimmed();
					}
					if (persInfoLossReg.indexIn(sMessage, 0) != -1) {
						// Суммарный урон экипировки: 3452
						nLoss = persInfoLossReg.cap(1).toInt();
						sMessage = aMessage[++i].trimmed();
					}
					if (persInfoProtectReg.indexIn(sMessage, 0) != -1) {
						// Суммарная защита экипировки: 3361
						nProtect = persInfoProtectReg.cap(1).toInt();
						sMessage = aMessage[++i].trimmed();
					}
				}
				if (sMessage == QString::fromUtf8("Умения:")) {
					sMessage = aMessage[++i].trimmed();
					// простой удар. ур.1; поражение:2; эн.4.
					// двойной удар. ур.:14;удар:35/55; защ.:16/18; подг.2хода; эн.36.
					// аура ярости. ур.:7;урон команды:+52;удар:8; защ.:8; подг.4хода; эн.109.
					// аура защитника. ур.:4;защита команды:+45;удар:7; защ.:7; подг.5хода; эн.104.
					while (persInfoAbility.indexIn(sMessage, 0) != -1) {

						sMessage = aMessage[++i].trimmed();
					}
					// Вещи:  (02- список вещей)
					if (sMessage.startsWith(QString::fromUtf8("Вещи:"))) {

						sMessage = aMessage[++i].trimmed();
					}
					// деньги: 10292833 дринк
					if (sMessage.startsWith(QString::fromUtf8("деньги:"))) {

						sMessage = aMessage[++i].trimmed();
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

					sMessage = aMessage[++i].trimmed();
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
					sMessage = aMessage[++i].trimmed();
					if (equipCount != 0 && sMessage != QString::fromUtf8("Кредиты:")) {
						// Проверяем комплекты
						while (true) {
							if (sMessage == "---")
								break;
							// Сначала предположительно имя комплекта
							sMessage = aMessage[++i].trimmed();
							// Список вещей в комплекте
							QStringList equipNameList = sMessage.split(";", QString::SkipEmptyParts, Qt::CaseInsensitive);
							int cnt = equipNameList.size();
							if (cnt < 2) {
								if (i > 1) i--;
								break;
							}
							sMessage = aMessage[++i].trimmed();
							// Список бонусов
							QStringList equipBonusList = sMessage.split(";", QString::SkipEmptyParts);
							if (equipBonusList.size() != cnt) {
								if (i > 2) i -= 2;
								break;
							}
							sMessage = aMessage[++i].trimmed();
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
						i--;
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
					persInfoPtr->setSitizenship(sSitizenship);
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
				i = nCount; // Блокируем дальнейший анализ
				nPersStatus = PersInform;
			} else if (sMessage == QString::fromUtf8("Экипировка:")) {
				// Список вещей персонажа
				Pers::instance()->setFingsStart(true);
				sMessage = aMessage[++i].trimmed();
				// Деньги

				bool fDressed = false;
				while (i < nCount) {
					sMessage = aMessage[i].trimmed();
					if (sMessage == QString::fromUtf8("одеты:")) {
						fDressed = true;
					} else if (sMessage == QString::fromUtf8("не одеты:")) {
						fDressed = false;
					} else {
						Thing* fg = new Thing(sMessage);
						if (fg->isValid()) {
							fg->setDressed(fDressed);
							Pers::instance()->setFingElement(FING_APPEND, fg);
						} else {
							delete fg;
						}
					}
					i++;
				}
				Pers::instance()->setFingsEnd();
				nPersStatus = ThingsList;
			} else if (sMessage.startsWith(QString::fromUtf8("Выбрана вещь: "))) {
				// Выбрана вещь: кровавый кристалл (вещь)

				i = nCount; // Блокируем дальнейший анализ
				nPersStatus = ThingIsTaken;
			} else if (fightShowReg.indexIn(sMessage, 0) != -1) {
				// Режим просмотра чужого боя в 05 3

				i = nCount; // Блокируем дальнейший анализ
				nPersStatus = FightShow;
			} else if (otherPersPosReg1.indexIn(sMessage, 0) != -1) {
				// Режим просмотра положения игроков по 05
				i++;
				QVector<GameMap::maps_other_pers> aOtherPers;
				while (i < nCount) {
					sMessage = aMessage[i].trimmed();
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
					i++;
				}
				GameMap::instance()->setOtherPersPos(&aOtherPers);
				nPersStatus = OtherPersPos;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("куплено!")) {
				nPersStatus = BuyOk;
			} else if (dealerBuyReg.indexIn(sMessage, 0) != -1) {
				nPersStatus = DealerBuy;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Выбери что нужно продать:")) { // Первая строка при продаже торговцу
				nPersStatus = DealerSale;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Вы во дворе.")) {
				nPersStatus = Yard;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Немного повозившись возле станка, Вы искусно наточили свое оружие.")) {
				nPersStatus = MasterRoom2;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Немного повозившись возле станка, Вы искусно наточили свое оружие и отполировали всю броню.")) {
				nPersStatus = MasterRoom3;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Ваше имя в кубке конторы убийц...")) {
				nPersStatus = InKillersCup;
				if (settingInKillersCupPopup) {
					initPopup(QString::fromUtf8("Sof game"), QString::fromUtf8("Ваше имя в кубке конторы убийц!"), 60);
				}
				i = nCount; // Блокируем дальнейший анализ
			} else if (killerAttackReg.indexIn(sMessage, 0) != -1) {
				if (settingKillerAttackPopup) {
					initPopup(QString::fromUtf8("Sof game"), QString::fromUtf8("На вас совершено нападение! Убийца: ") + killerAttackReg.cap(1), 10);
				}
				nPersStatus = KillerAttack;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Меню справки:")) {
				nPersStatus = HelpMenu;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Сильнейшие персонажи.")) {
				nPersStatus = TopList;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Статистика")) {
				nPersStatus = ServerStatistic1;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Статистика по странам.")) {
				nPersStatus = ServerStatistic2;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Ваша недвижимость:")) {
				nPersStatus = RealEstate;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage == QString::fromUtf8("Выберите полку:")) {
				nPersStatus = Warehouse;
				i = nCount; // Блокируем дальнейший анализ
			} else if (warehouseShelfReg.indexIn(sMessage, 0) != -1) {
				nPersStatus = WarehouseShelf;
				i = nCount; // Блокируем дальнейший анализ
			} else if (sMessage.startsWith(QString::fromUtf8("здесь идут сражения"))) {
				nPersStatus = FightMultiSelect;
			} else if (sMessage.startsWith(QString::fromUtf8("новый бой"))) {
				nPersStatus = FightOpenBegin;
				fNewFight = true;
			} else if (sMessage.startsWith(QString::fromUtf8("Команды:"))) {
				nPersStatus = FightFinish;
			} else if (persStatus == SecretBefore) {
				// Предыдущий статус  "перед тайником"
				if (sMessage.contains(QString::fromUtf8("тайник"), Qt::CaseInsensitive) || sMessage.startsWith(QString::fromUtf8("с разбитого ящика"), Qt::CaseInsensitive) || sMessage.contains(QString::fromUtf8("сокровищниц"), Qt::CaseInsensitive)) {
					nPersStatus = SecretGet;
					if (fightDropMoneyReg2.indexIn(sMessage, 0) != -1) {
						fDropMoneys = true;
						nDropMoneys = nDropMoneys + fightDropMoneyReg2.cap(2).toInt();
					} else if (secretDropFingReg.indexIn(sMessage, 0) != -1) {
						++nFingsDropCount;
						sFingDropLast = secretDropFingReg.cap(2);
						if (settingFingDropPopup) {
							initPopup(QString::fromUtf8("Sof game"), "+" + sFingDropLast, 3);
						}
					}
				}
			} else if (persStatus == TakeBefore) {
				// Предыдущий статус  "перед отъемом"
				nPersStatus = Take;
				if (fightDropMoneyReg2.indexIn(sMessage, 0) != -1) {
					fDropMoneys = true;
					nDropMoneys = nDropMoneys + fightDropMoneyReg2.cap(2).toInt();
				} else if (secretDropFingReg.indexIn(sMessage, 0) != -1) {
					++nFingsDropCount;
					sFingDropLast = secretDropFingReg.cap(2);
					if (settingFingDropPopup) {
						initPopup(QString::fromUtf8("Sof game"), "+" + sFingDropLast, 3);
					}
				}
			} else if (sMessage.startsWith(QString::fromUtf8("Открытый бой"), Qt::CaseInsensitive)) {
				nPersStatus = FightOpenBegin;
				fight->start();
				fight->setMode(FIGHT_MODE_OPEN);
				i++;
				i += parseFinghtGroups(aMessage, i);
				nTimeout = fight->timeout();
				break;
			} else if (sMessage.startsWith(QString::fromUtf8("Закрытый бой"), Qt::CaseInsensitive)) {
				nPersStatus = FightCloseBegin;
				fight->start();
				fight->setMode(FIGHT_MODE_CLOSE);
				i++;
				i += parseFinghtGroups(aMessage, i);
				nTimeout = fight->timeout();
				break;
			} else {
				// Строчка первая, но статус не определен
				// Статус не определяется на первой строке много где,
				// но в данном случае нас интересует резутьтат удара в бою
				if (fight->isActive() || persStatus == FightMultiSelect || persStatus == NotKnow) {
					// Сейчас статус не известен. На предыдущем ходе были в бою или не известно чего делали.
					int p_cnt = parseFinghtStepResult(aMessage, i);
					if (p_cnt != 0) { // Это был текст с результатами боя
						if (fight->allyCount() == 0) { // Мин. и макс. удары считаем только когда в один в бою
							int num1 = fight->minDamage();
							if (statFightDamageMin == -1 || (num1 != -1 && num1 < statFightDamageMin)) {
								fStatFightDamageMin = true;
								statFightDamageMin = num1;
							}
							num1 = fight->maxDamage();
							if (num1 > statFightDamageMax) {
								fStatFightDamageMax = true;
								statFightDamageMax = num1;
							}
						}
						i += p_cnt;
						sMessage = aMessage.at(i).trimmed();
						if (sMessage.startsWith(QString::fromUtf8("Открытый бой"))) {
							nPersStatus = FightOpenBegin;
							if (!fight->isActive())
								fight->start();
							fight->setMode(FIGHT_MODE_OPEN);
							i++;
							i += parseFinghtGroups(aMessage, i);
							nTimeout = fight->timeout();
							break;
						} else if (sMessage.startsWith(QString::fromUtf8("Закрытый бой"))) {
							nPersStatus = FightCloseBegin;
							if (!fight->isActive())
								fight->start();
							fight->setMode(FIGHT_MODE_CLOSE);
							i++;
							i += parseFinghtGroups(aMessage, i);
							nTimeout = fight->timeout();
							break;
						} else if (sMessage.startsWith(QString::fromUtf8("Бой завершен!"))) {
							// Бой завершен
							nPersStatus = FightFinish;
							fight->finish();
							nTimeout = 0;
							i++;
							if (i < nCount) {
								sMessage = aMessage.at(i).trimmed();
								if (experienceDropReg.indexIn(sMessage, 0) != -1) {
									// Берем опыт, который дали в конце боя
									statExperienceDropCount += experienceDropReg.cap(1).toInt();
									fExperienceDrop = true;
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
			if (nPersStatus == Stand || nPersStatus == SecretBefore) {
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
			} else if (nPersStatus == SecretGet || nPersStatus == Take) {
				if (experienceDropReg2.indexIn(sMessage, 0) != -1) {
					// Берем опыт, который дали в тайнике
					statExperienceDropCount += experienceDropReg2.cap(1).toInt();
					fExperienceDrop = true;
				}
			} else if (nPersStatus != FightFinish) {
				if (experienceDropReg.indexIn(sMessage, 0) != -1) {
					// Найден опыт, но не в бою (У Элементаля например)
					statExperienceDropCount += experienceDropReg.cap(1).toInt();
					fExperienceDrop = true;
				}
			}
		}
		if (nPersStatus == NotKnow) {
			// Любая строка, статус еще не определен
			// Бывает, попадает команда в строчку, потому "contains" а не "startsWith"
			if (sMessage.contains(QString::fromUtf8("Открытый бой"), Qt::CaseInsensitive)) {
				nPersStatus = FightOpenBegin;
				fight->start();
				fight->setMode(FIGHT_MODE_OPEN);
			} else if (sMessage.contains(QString::fromUtf8("Закрытый бой"), Qt::CaseInsensitive)) {
				nPersStatus = FightCloseBegin;
				fight->start();
				fight->setMode(FIGHT_MODE_CLOSE);
			}
		} else if (nPersStatus == Stand) {
			// Статус персонажа определен как стоит
			if (secretBeforeReg.indexIn(sMessage, 0) != -1  || secretBeforeReg2.indexIn(sMessage, 0) != -1) {
				// Перед тайником
				nPersStatus = SecretBefore;
			} else if (takeBeforeReg.indexIn(sMessage, 0) != -1) {
				// перед грабежом побежденного
				nPersStatus = TakeBefore;
			}
		}
		i++;
	}
	// Парсинг закончен, реагируем на результаты
	if (nPersStatus == FightOpenBegin || nPersStatus == FightCloseBegin) {
		if (!fight->isActive()) {
			fight->start();
		}
		if (fight->isActive()) {
			if (fight->getStep() == 1) {
				int enCnt = fight->gameMobEnemyCount();
				if (enCnt > 0) {
					if (fight->getStep() == 1) {
						GameMap::instance()->setMapElementEnemies(settingPersX, settingPersY, enCnt, enCnt);
						GameMap::instance()->setMapElementEnemiesList(settingPersX, settingPersY, fight->mobEnemiesList());
					}
				}
			}
		}
	} else if (nPersStatus != FightMultiSelect) {
		if (fight->isActive()) {
			fight->finish();
		}
	}
	// Начало установки параметров персонажа
	Pers::instance()->beginSetPersParams();
	// Отправляем здоровье
	if (fHealth) {
		Pers::instance()->setPersParams(VALUE_HEALTH_MAX, TYPE_INTEGER_FULL, nHealthF);
		Pers::instance()->setPersParams(VALUE_HEALTH_CURR, TYPE_INTEGER_FULL, nHealthC);
	}
	// Отправляем энергию
	if (fPower) {
		Pers::instance()->setPersParams(VALUE_ENERGY_MAX, TYPE_INTEGER_FULL, nPowerF);
		Pers::instance()->setPersParams(VALUE_ENERGY_CURR, TYPE_INTEGER_FULL, nPowerC);
	}
	// Отправляем уровень персонажа
	if (fLevel) {
		Pers::instance()->setPersParams(VALUE_PERS_LEVEL, TYPE_INTEGER_FULL, nLevel);
	}
	// Отправляем опыт
	if (fExperience) {
		if (statExperienceFull == -1 || statExperienceFull != nExperienceFull) {
			statExperienceFull = nExperienceFull;
			valueChanged(VALUE_EXPERIENCE_MAX, TYPE_INTEGER_FULL, nExperienceFull);
			persParamChanged();
		}
		if (statExperience == -1 || statExperience != nExperience) {
			statExperience = nExperience;
			valueChanged(VALUE_EXPERIENCE_CURR, TYPE_INTEGER_FULL, nExperience);
			persParamChanged();
		}
	}
	// Сохраняем и отправляем текущий статус персонажа
	if (persStatus != nPersStatus) {
		if (nPersStatus == FightFinish) {
			statFightsCount = statFightsCount + 1;
			valueChanged(VALUE_FIGHTS_COUNT, TYPE_INTEGER_FULL, statFightsCount);
			statisticsChanged();
		}
		persStatus = nPersStatus;
		//valueChanged(VALUE_PERS_STATUS, TYPE_INTEGER_FULL, nPersStatus);
		Pers::instance()->setPersParams(VALUE_PERS_STATUS, TYPE_INTEGER_FULL, nPersStatus);
	}
	// Конец установки параметров персонажа
	Pers::instance()->endSetPersParams();
	// Отправляем имя персонажа
	if (fName && (sName != Pers::instance()->name())) {
		Pers::instance()->setName(sName);
		valueChanged(VALUE_PERS_NAME, TYPE_STRING, 0);
		persParamChanged();
		// Обновляем регулярки
		updateRegExpForPersName();
	}
	// Отправляем упавшие деньги
	if (fDropMoneys) {
		statMoneysDropCount += nDropMoneys;
		valueChanged(VALUE_DROP_MONEYS, TYPE_INTEGER_FULL, statMoneysDropCount);
		statisticsChanged();
	}
	// Отправляем минимальный удар
	if (fStatFightDamageMin) {
		valueChanged(VALUE_DAMAGE_MIN_FROM_PERS, TYPE_INTEGER_FULL, statFightDamageMin);
		statisticsChanged();
	}
	// Отправляем максимальный удар
	if (fStatFightDamageMax) {
		valueChanged(VALUE_DAMAGE_MAX_FROM_PERS, TYPE_INTEGER_FULL, statFightDamageMax);
		statisticsChanged();
	}
	// Отправляем количество найденных вещей
	if (nFingsDropCount > 0) {
		statFingsDropCount += nFingsDropCount;
		statFingDropLast = sFingDropLast;
		valueChanged(VALUE_FINGS_DROP_COUNT, TYPE_INTEGER_FULL, statFingsDropCount);
		statisticsChanged();
	}
	// Отправляем упавший опыт
	if (fExperienceDrop) {
		valueChanged(VALUE_EXPERIENCE_DROP_COUNT, TYPE_INTEGER_FULL, statExperienceDropCount);
		statisticsChanged();
	}
	// Отправляем значение таймаута
	if (nTimeout >= 0) {
		valueChanged(VALUE_TIMEOUT, TYPE_INTEGER_FULL, nTimeout);
	}
	// Отсылаем новую позицию персонажа
	if (fPersPosChanged) {
		valueChanged(VALUE_CHANGE_PERS_POS, TYPE_INTEGER_FULL, persY * 100000 + persX);
	}
	//if (myMessage) {
		// Выводим сообщение игры на экран
		int startText = -1;
		if (persX != QINT32_MIN && persY != QINT32_MIN) { // Есть координаты от GPS
			startText = message->indexOf("\n");
		}
		if (startText == -1) {
			setGameText(*message);
		} else {
			setGameText(message->midRef(startText + 1).toString());
		}
	//}
	if (nCmdStatus > 3  && persX != QINT32_MIN && persY != QINT32_MIN) { // Т.е Есть что то кроме нестандартных и посторонних команд и координаты
		GameMap::instance()->setMapElementPaths(persX, persY, nCmdStatus);
		if ((nCmdStatus & 1024) != 0) {
			GameMap::instance()->setMapElementType(persX, persY, GameMap::TypePortal);
		} else if ((nCmdStatus & 2048) != 0) {
			GameMap::instance()->setMapElementType(persX, persY, GameMap::TypeSecret);
		}
	}
	if (fNewFight) {
		// Если выбран новый бой, посылаем "0" чтобы показать список мобов
		QString str1 = "0"; // Продолжение игры
		mySender->sendSystemString(&str1);
	} else if (nPersStatus == FightMultiSelect) {
		if (settingFightSelectAction == 1 || settingFightSelectAction == 2) { // Всегда новый бой
			if (settingFightSelectAction == 2 || mySender->getGameQueueLength() > 0) {
				QString str1 = "1"; // 1 - новый бой
				mySender->sendSystemString(&str1);
			}
		}
	} else if ((nPersStatus == NotKnow && settingResetQueueForUnknowStatus) || nPersStatus == KillerAttack) {
		// Если очередь не пуста, сбрасываем очередь
		int q_len = mySender->getGameQueueLength();
		if (q_len > 0) {
			mySender->resetGameQueue();
			if (settingResetQueuePopup) {
				initPopup(QString::fromUtf8("Sof game"), QString::fromUtf8("Очередь сброшена"), 60);
			}
			QString str1 = QString::fromUtf8("### Очередь сброшена. Сброшено команд: %1 ###").arg(q_len);
			setGameText(str1);
			setConsoleText(str1, false);
		}
	}
	return myMessage;
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
	setGameText(errStr);
	setConsoleText(errStr, false);
}

void PluginCore::fightStarted(int mode)
{
	/**
	* Обрабатывает событие начала боя
	**/
	if (mode == FIGHT_MODE_OPEN) {
		if (fight->isPersInFight()) { // Наш персонаж в бою?
			if (settingFightAutoClose == 1) { // Автозакрытие боя если один против мобов
				if (fight->gameMobEnemyCount() > 0 && fight->gameHumanEnemyCount() == 0) { // В противниках только мобы
					if (fight->gameMobAllyCount() == 0 && fight->gameHumanAllyCount() == 0) { // В нашей команде никого нет
						QString str1 = "*"; // Команда закрытия боя
						mySender->sendSystemString(&str1);
						str1 = "0"; // Продолжение игры
						mySender->sendSystemString(&str1);
					}
				}
			}
		}
	}
}

void PluginCore::valueChanged(int valueId, int valueType, int value)
{
	if (mainWindow) {
		emit mainWindow->valueChanged(valueId, valueType, value);
	}
}

void PluginCore::setGameText(QString message)
{
	if (mainWindow) {
		mainWindow->setGameText(message, 2);
	}
}

void PluginCore::setConsoleText(QString message, bool switch_)
{
	if (mainWindow) {
		mainWindow->setConsoleText(message, 2, switch_);
	}
}

bool PluginCore::getIntValue(int valueId, int* valuePtr)
{
	if (valueId == VALUE_PERS_STATUS) {
		*valuePtr = persStatus;
		return true;
	}
	if (valueId == VALUE_DROP_MONEYS) {
		*valuePtr = statMoneysDropCount;
		return true;
	}
	if (valueId == VALUE_MESSAGES_COUNT) {
		*valuePtr = statMessagesCount;
		return true;
	}
	if (valueId == VALUE_DAMAGE_MIN_FROM_PERS) {
		if (statFightDamageMin == -1) {
			return false;
		}
		*valuePtr = statFightDamageMin;
		return true;
	}
	if (valueId == VALUE_DAMAGE_MAX_FROM_PERS) {
		if (statFightDamageMax == -1) {
			return false;
		}
		*valuePtr = statFightDamageMax;
		return true;
	}
	if (valueId == VALUE_FINGS_DROP_COUNT) {
		*valuePtr = statFingsDropCount;
		return true;
	}
	if (valueId == VALUE_FIGHTS_COUNT) {
		*valuePtr = statFightsCount;
		return true;
	}
	if (valueId == VALUE_EXPERIENCE_CURR) {
		if (statExperience == -1) {
			return false;
		}
		*valuePtr = statExperience;
		return true;
	}
	if (valueId == VALUE_EXPERIENCE_MAX) {
		if (statExperienceFull == -1) {
			return false;
		}
		*valuePtr = statExperienceFull;
		return true;
	}
	if (valueId == VALUE_EXPERIENCE_DROP_COUNT) {
		*valuePtr = statExperienceDropCount;
		return true;
	}
	if (valueId == VALUE_KILLED_ENEMIES) {
		*valuePtr = statKilledEnemies;
		return true;
	}
	return false;
}

bool PluginCore::getTextValue(int valueId, QString* valuePtr)
{
	if (valueId == VALUE_LAST_GAME_JID) {
		if (lastGameJid.length() == 0) {
			return false;
		}
		*valuePtr = lastGameJid;
		return true;
	}
	if (valueId == VALUE_FING_DROP_LAST) {
		if (statFingDropLast.length() == 0) {
			return false;
		}
		*valuePtr = statFingDropLast;
		return true;
	}
	if (valueId == VALUE_LAST_CHAT_JID) {
		if (lastChatJid.length() == 0) {
			return false;
		}
		*valuePtr = lastChatJid;
		return true;
	}
	if (valueId == VALUE_PERS_NAME) {
		QString s_name = Pers::instance()->name();
		if (s_name.isEmpty()) {
			return false;
		}
		*valuePtr = s_name;
		return true;
	}
	return false;
}

void PluginCore::resetStatistic(int valueId)
{
	if (valueId == VALUE_DROP_MONEYS) {
		statMoneysDropCount = 0;
		valueChanged(VALUE_DROP_MONEYS, TYPE_INTEGER_FULL, statMoneysDropCount);
	} else if (valueId == VALUE_MESSAGES_COUNT) {
		statMessagesCount = 0;
		valueChanged(VALUE_MESSAGES_COUNT, TYPE_INTEGER_FULL, statMessagesCount);
	} else if (valueId == VALUE_DAMAGE_MIN_FROM_PERS) {
		statFightDamageMin = -1;
		valueChanged(VALUE_DAMAGE_MIN_FROM_PERS, TYPE_NA, 0);
	} else if (valueId == VALUE_DAMAGE_MAX_FROM_PERS) {
		statFightDamageMax = -1;
		valueChanged(VALUE_DAMAGE_MAX_FROM_PERS, TYPE_NA, 0);
	} else if (valueId == VALUE_FINGS_DROP_COUNT) {
		statFingsDropCount = 0;
		valueChanged(VALUE_FINGS_DROP_COUNT, TYPE_INTEGER_FULL, statFingsDropCount);
	} else if (valueId == VALUE_FIGHTS_COUNT) {
		statFightsCount = 0;
		valueChanged(VALUE_FIGHTS_COUNT, TYPE_INTEGER_FULL, statFightsCount);
	} else if (valueId == VALUE_LAST_GAME_JID) {
		lastGameJid = "";
		valueChanged(VALUE_LAST_GAME_JID, TYPE_NA, 0);
	} else if (valueId == VALUE_FING_DROP_LAST) {
		statFingDropLast = "";
		valueChanged(VALUE_FING_DROP_LAST, TYPE_NA, 0);
	} else if (valueId == VALUE_LAST_CHAT_JID) {
		lastChatJid = "";
		valueChanged(VALUE_LAST_CHAT_JID, TYPE_NA, 0);
	} else if (valueId == VALUE_EXPERIENCE_DROP_COUNT) {
		statExperienceDropCount = 0;
		valueChanged(VALUE_EXPERIENCE_DROP_COUNT, TYPE_INTEGER_FULL, statExperienceDropCount);
	} else if (valueId == VALUE_KILLED_ENEMIES) {
		statKilledEnemies = 0;
		valueChanged(VALUE_KILLED_ENEMIES, TYPE_INTEGER_FULL, statKilledEnemies);
	}
}

bool PluginCore::setIntSettingValue(qint32 settingId, int settingValue)
{
	// Если settingPtr == 0 то значение выставляется по-умолчанию
	//--
	if (settingId >= SETTING_SLOT1 && settingId <= SETTING_SLOT9) {
		int i = settingId - SETTING_SLOT1;
		settingSlots[i] = settingValue;
	} else if (settingId == SETTING_PERS_PARAM_SAVE_MODE) {
		settingPersSaveMode = 0;
		settingPersSaveMode = settingValue;
		if (settingPersSaveMode == 2) {
			if (!saveStatusTimer.isActive()) {
				saveStatusTimer.start(5000*60);
			}
		} else {
			if (saveStatusTimer.isActive()) {
				saveStatusTimer.stop();
			}
		}
	} else if (settingId == SETTING_SAVE_PERS_PARAM) {
		settingSavePersParam = false;
		if (settingValue == 1) {
			settingSavePersParam = true;
		}
	} else if (settingId == SETTING_SAVE_PERS_BACKPACK) {
		settingSaveBackpack = false;
		if (settingValue == 1) {
			settingSaveBackpack = true;
		}
	} else if (settingId == SETTING_SAVE_PERS_STAT) {
		settingSaveStat = false;
		if (settingValue == 1) {
			settingSaveStat = true;
		}
	} else if (settingId == SETTING_CHANGE_MIRROR_MODE) {
		settingChangeMirrorMode = 0;
		settingChangeMirrorMode = settingValue;
		return mySender->changeGameMirrorsMode(settingChangeMirrorMode);
	} else if (settingId == SETTING_WINDOW_SIZE_POS) {
		settingWindowSizePos = 0;
			settingWindowSizePos = settingValue;
	} else if (settingId == SETTING_WINDOW_POS_X) {
		settingWindowPosX = QINT32_MIN;
		settingWindowPosX = settingValue;
	} else if (settingId == SETTING_WINDOW_POS_Y) {
		settingWindowPosY = QINT32_MIN;
		settingWindowPosY = settingValue;
	} else if (settingId == SETTING_WINDOW_WIDTH) {
		settingWindowWidth = QINT32_MIN;
		settingWindowWidth = settingValue;
	} else if (settingId == SETTING_WINDOW_HEIGHT) {
		settingWindowHeight = QINT32_MIN;
		settingWindowHeight = settingValue;
	} else if (settingId == SETTING_FIGHT_TIMER) {
		settingFightTimer = 0;
		settingFightTimer = settingValue;
	} else if (settingId == SETTING_AUTOCLOSE_FIGHT) {
		if (settingValue == 0) {
			if (settingFightAutoClose != 0) {
				disconnect(fight, SIGNAL(fightStart(int)), this, SLOT(fightStarted(int)));
				settingFightAutoClose = 0;
			}
		} else {
			if (settingFightAutoClose != settingValue) {
				if (settingFightAutoClose == 0) {
					connect(fight, SIGNAL(fightStart(int)), this, SLOT(fightStarted(int)));
				}
				settingFightAutoClose = settingValue;
			}
		}
	} else if (settingId == SETTING_FING_DROP_POPUP) {
		settingFingDropPopup = false;
		if (settingValue == 1) {
			settingFingDropPopup = true;
		}
	} else if (settingId == SETTING_REST_HEALTH_ENERGY) {
		settingWatchRestHealthEnergy = 0;
		settingWatchRestHealthEnergy = settingValue;
		Pers::instance()->setSetting(SETTING_REST_HEALTH_ENERGY, settingValue);
	} else if (settingId == SETTING_FIGHT_SELECT_ACTION) {
		settingFightSelectAction = 0;
		settingFightSelectAction = settingValue;
	} else if (settingId == SETTING_IN_KILLERS_CUP_POPUP) {
		settingInKillersCupPopup = false;
		if (settingValue == 1) {
			settingInKillersCupPopup = true;
		}
	} else if (settingId == SETTING_KILLER_ATTACK_POPUP) {
		settingKillerAttackPopup = false;
		if (settingValue == 1) {
			settingKillerAttackPopup = true;
		}
	} else if (settingId == SETTING_SHOW_QUEUE_LENGTH) {
		settingShowQueryLength = false;
		if (settingValue == 1) {
			settingShowQueryLength = true;
		}
	} else if (settingId == SETTING_RESET_QUEUE_FOR_UNKNOW_STATUS) {
		settingResetQueueForUnknowStatus = false;
		if (settingValue != 0) {
			settingResetQueueForUnknowStatus = true;
		}
	} else if (settingId == SETTING_RESET_QUEUE_POPUP_SHOW) {
		settingResetQueuePopup = (settingValue != 0);
	} else if (settingId == SETTING_SERVER_TEXT_BLOCKS_COUNT) {
		settingServerTextBlocksCount = settingValue;
	} else if (settingId == SETTING_MAPS_PARAM_SAVE_MODE) {
		if (settingMapsSaveMode != settingValue) {
			settingMapsSaveMode = settingValue;
			GameMap::instance()->setMapsParams(GameMap::AutoSaveMode, settingValue);
		}
	} else {
		return false;
	}
	return true;
}

bool PluginCore::setStringSettingValue(qint32 settingId, QString* settingPtr)
{
	if (settingId == SETTING_PERS_NAME) {
		settingPersName = *settingPtr;
	} else if (settingId == SETTING_PERS_NAME_FONT) {
		settingPersNameFont = *settingPtr;
	} else if (settingId == SETTING_SERVER_TEXT_FONT) {
		settingServerTextFont = *settingPtr;
	} else {
		return false;
	}
	return true;
}

bool PluginCore::getIntSettingValue(qint32 settingId, qint32* settingPtr)
{
	int i;
	if (settingId >= SETTING_SLOT1 && settingId <= SETTING_SLOT9) {
		i = settingId - SETTING_SLOT1;
		*settingPtr = settingSlots[i];
	} else if (settingId == SETTING_PERS_PARAM_SAVE_MODE) {
		*settingPtr = settingPersSaveMode;
	} else if (settingId == SETTING_SAVE_PERS_PARAM) {
		*settingPtr = (settingSavePersParam) ? 1 : 0;
	} else if (settingId == SETTING_SAVE_PERS_BACKPACK) {
		*settingPtr = (settingSaveBackpack) ? 1 : 0;
	} else if (settingId == SETTING_SAVE_PERS_STAT) {
		*settingPtr = (settingSaveStat) ? 1 : 0;
	} else if (settingId == SETTING_CHANGE_MIRROR_MODE) {
		*settingPtr = settingChangeMirrorMode;
	} else if (settingId == SETTING_WINDOW_SIZE_POS) {
		*settingPtr = settingWindowSizePos;
	} else if (settingId == SETTING_WINDOW_POS_X) {
		if (settingWindowPosX == QINT32_MIN) {
			return false;
		}
		*settingPtr = settingWindowPosX;
	} else if (settingId == SETTING_WINDOW_POS_Y) {
		if (settingWindowPosY == QINT32_MIN) {
			return false;
		}
		*settingPtr = settingWindowPosY;
	} else if (settingId == SETTING_WINDOW_WIDTH) {
		if (settingWindowWidth == QINT32_MIN) {
			return false;
		}
		*settingPtr = settingWindowWidth;
	} else if (settingId == SETTING_WINDOW_HEIGHT) {
		if (settingWindowHeight == QINT32_MIN) {
			return false;
		}
		*settingPtr = settingWindowHeight;
	} else if (settingId == SETTING_FIGHT_TIMER) {
		if (settingFightTimer == -1) {
			return false;
		}
		*settingPtr = settingFightTimer;
	} else if (settingId == SETTING_AUTOCLOSE_FIGHT) {
		*settingPtr = settingFightAutoClose;
	} else if (settingId == SETTING_FING_DROP_POPUP) {
		if (settingFingDropPopup) {
			*settingPtr = 1;
		} else {
			*settingPtr = 0;
		}
	} else if (settingId == SETTING_REST_HEALTH_ENERGY) {
		*settingPtr = settingWatchRestHealthEnergy;
	} else if (settingId == SETTING_FIGHT_SELECT_ACTION) {
		*settingPtr = settingFightSelectAction;
	} else if (settingId == SETTING_IN_KILLERS_CUP_POPUP) {
		if (settingInKillersCupPopup) {
			*settingPtr = 1;
		} else {
			*settingPtr = 0;
		}
	} else if (settingId == SETTING_KILLER_ATTACK_POPUP) {
		if (settingKillerAttackPopup) {
			*settingPtr = 1;
		} else {
			*settingPtr = 0;
		}
	} else if (settingId == SETTING_SHOW_QUEUE_LENGTH) {
		if (settingShowQueryLength) {
			*settingPtr = 1;
		} else {
			*settingPtr = 0;
		}
	} else if (settingId == SETTING_RESET_QUEUE_FOR_UNKNOW_STATUS) {
		if (settingResetQueueForUnknowStatus) {
			*settingPtr = 1;
		} else {
			*settingPtr = 0;
		}
	} else if (settingId == SETTING_RESET_QUEUE_POPUP_SHOW) {
		*settingPtr = (settingResetQueuePopup) ? 1 : 0;
	} else if (settingId == SETTING_SERVER_TEXT_BLOCKS_COUNT) {
		*settingPtr = settingServerTextBlocksCount;
	} else if (settingId == SETTING_MAPS_PARAM_SAVE_MODE) {
		*settingPtr = settingMapsSaveMode;
	} else {
		return false;
	}
	return true;
}

bool PluginCore::getStringSettingValue(qint32 settingId, QString* settingPtr)
{
	if (settingId == SETTING_PERS_NAME) {
		*settingPtr = settingPersName;
	} else if (settingId == SETTING_PERS_NAME_FONT) {
		*settingPtr = settingPersNameFont;
	} else if (settingId == SETTING_SERVER_TEXT_FONT) {
		*settingPtr = settingServerTextFont;
	} else {
		return false;
	}
	return true;
}

bool PluginCore::sendCommandToCore(qint32 commandId)
{
	if (commandId == COMMAND_SAVE_SETTINGS) {
		// Немедленно сохраняет настройки плагина
		return savePluginSettings();
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
	if (settingSavePersParam) {
		QDomElement eMain = xmlDoc.createElement("main");
		eNewAccount.appendChild(eMain);
		if (!Pers::instance()->name().isEmpty()) {
			domElement = xmlDoc.createElement("pers-name");
			domElement.appendChild(xmlDoc.createTextNode(Pers::instance()->name()));
			eMain.appendChild(domElement);
		}
		int num1;
		if (Pers::instance()->getIntParamValue(VALUE_PERS_LEVEL, &num1)) {
			domElement = xmlDoc.createElement("pers-level");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
			eMain.appendChild(domElement);
		}
		if (statExperience != -1) {
			domElement = xmlDoc.createElement("pers-experience-curr");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statExperience)));
			eMain.appendChild(domElement);
		}
		if (statExperienceFull != -1) {
			domElement = xmlDoc.createElement("pers-experience-max");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statExperienceFull)));
			eMain.appendChild(domElement);
		}
		if (Pers::instance()->getIntParamValue(VALUE_HEALTH_CURR, &num1)) {
			domElement = xmlDoc.createElement("pers-health-curr");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
			eMain.appendChild(domElement);
		}
		if (Pers::instance()->getIntParamValue(VALUE_HEALTH_MAX, &num1)) {
			domElement = xmlDoc.createElement("pers-health-max");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
			eMain.appendChild(domElement);
		}
		if (Pers::instance()->getIntParamValue(VALUE_ENERGY_CURR, &num1)) {
			domElement = xmlDoc.createElement("pers-energy-curr");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
			eMain.appendChild(domElement);
		}
		if (Pers::instance()->getIntParamValue(VALUE_ENERGY_MAX, &num1)) {
			domElement = xmlDoc.createElement("pers-energy-max");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(num1)));
			eMain.appendChild(domElement);
		}
	}
	if (settingSaveStat != 0) {
		QDomElement eStat = xmlDoc.createElement("statistic");
		eNewAccount.appendChild(eStat);
		if (lastGameJid.length() != 0) {
			domElement = xmlDoc.createElement("last-game-jid");
			domElement.appendChild(xmlDoc.createTextNode(lastGameJid));
			eStat.appendChild(domElement);
		}
		if (lastChatJid.length() != 0) {
			domElement = xmlDoc.createElement("last-chat-jid");
			domElement.appendChild(xmlDoc.createTextNode(lastChatJid));
			eStat.appendChild(domElement);
		}
		if (statMessagesCount != 0) {
			domElement = xmlDoc.createElement("in-messages-count");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statMessagesCount)));
			eStat.appendChild(domElement);
		}
		if (statFightsCount != 0) {
			domElement = xmlDoc.createElement("fights-count");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statFightsCount)));
			eStat.appendChild(domElement);
		}
		if (statFightDamageMax != -1) {
			domElement = xmlDoc.createElement("pers-fight-damage-max");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statFightDamageMax)));
			eStat.appendChild(domElement);
		}
		if (statFightDamageMin != -1) {
			domElement = xmlDoc.createElement("pers-fight-damage-min");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statFightDamageMin)));
			eStat.appendChild(domElement);
		}
		if (statMoneysDropCount != 0) {
			domElement = xmlDoc.createElement("moneys-drop-count");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statMoneysDropCount)));
			eStat.appendChild(domElement);
		}
		if (statFingsDropCount != 0) {
			domElement = xmlDoc.createElement("fings-drop-count");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statFingsDropCount)));
			eStat.appendChild(domElement);
		}
		if (statFingDropLast.length() != 0) {
			domElement = xmlDoc.createElement("fing-drop-last");
			domElement.appendChild(xmlDoc.createTextNode(statFingDropLast));
			eStat.appendChild(domElement);
		}
		if (statExperienceDropCount != 0) {
			domElement = xmlDoc.createElement("experience-drop-count");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statExperienceDropCount)));
			eStat.appendChild(domElement);
		}
		if (statKilledEnemies != 0) {
			domElement = xmlDoc.createElement("killed-enemies");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(statKilledEnemies)));
			eStat.appendChild(domElement);
		}
	}
	if (Pers::instance() && settingSaveBackpack) {
		// Сохраняем данные о вещах
		QDomElement eFings = xmlDoc.createElement("backpack");
		Pers::instance()->exportFingsToDomElement(&xmlDoc, &eFings);
		eNewAccount.appendChild(eFings);
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

bool PluginCore::savePluginSettings()
{
	if (accJid.isEmpty()) {
		return false;
	}
	// Читаем старые настройки
	QDomDocument xmlDoc;
	QDomElement eRoot;
	QDomNode eOldAccount;
	bool bReplace = false;
	if (loadPluginXml(&xmlDoc, "sofgame_settings.xml")) {
		// Ищем наш account элемент
		eRoot = xmlDoc.documentElement();
		if (eRoot.tagName() == "settings") {
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
	QDomElement eMain = xmlDoc.createElement("main");
	eNewAccount.appendChild(eMain);
	if (settingPersName.isEmpty()) {
		settingPersName = Pers::instance()->name();
	}
	QDomElement ePersName = xmlDoc.createElement("pers-name");
	if (settingPersName.length() != 0) {
		ePersName.appendChild(xmlDoc.createTextNode(settingPersName));
	}
	eMain.appendChild(ePersName);
	if (settingPersSaveMode >= 0 && settingPersSaveMode < persSaveModeStrings.size()) {
		QDomElement eSaveParamMode = xmlDoc.createElement("pers-save-mode");
		eSaveParamMode.setAttribute("value", persSaveModeStrings.at(settingPersSaveMode));
		eMain.appendChild(eSaveParamMode);
	}
	QDomElement eSaveParam = xmlDoc.createElement("save-pers-params");
	eSaveParam.setAttribute("value", (settingSavePersParam) ? "true" : "false");
	eMain.appendChild(eSaveParam);
	QDomElement eSaveBackpack = xmlDoc.createElement("save-pers-backpack");
	eSaveBackpack.setAttribute("value", (settingSaveBackpack) ? "true" : "false");
	eMain.appendChild(eSaveBackpack);
	QDomElement eSaveStat = xmlDoc.createElement("save-statistic");
	eSaveStat.setAttribute("value", (settingSaveStat) ? "true" : "false");
	eMain.appendChild(eSaveStat);
	if (settingChangeMirrorMode >= 0 && settingChangeMirrorMode <= 1) {
		QDomElement eMirrorMode = xmlDoc.createElement("mirror-change-mode");
		eMirrorMode.setAttribute("value", mirrorChangeModeStrings.at(settingChangeMirrorMode));
		eMain.appendChild(eMirrorMode);
	}
	if (settingWindowSizePos == 1) {
		QDomElement eWindowParams = xmlDoc.createElement("window-save-params");
		eWindowParams.setAttribute("mode", "position-and-size");
		eMain.appendChild(eWindowParams);
	}
	if (settingWatchRestHealthEnergy >= 0 && settingWatchRestHealthEnergy <= 4) {
		QDomElement eWatchRest = xmlDoc.createElement("watch-rest-health-energy");
		eWatchRest.setAttribute("value", watchRestHealthEnergyStrings.at(settingWatchRestHealthEnergy));
		eMain.appendChild(eWatchRest);
	}
	QDomElement eInKillersCupPopup = xmlDoc.createElement("in-killers-cup-popup");
	if (settingInKillersCupPopup) {
		eInKillersCupPopup.setAttribute("value", "true");
	} else {
		eInKillersCupPopup.setAttribute("value", "false");
	}
	eMain.appendChild(eInKillersCupPopup);
	QDomElement eKillerAttackPopup = xmlDoc.createElement("killer-attack-popup");
	if (settingKillerAttackPopup) {
		eKillerAttackPopup.setAttribute("value", "true");
	} else {
		eKillerAttackPopup.setAttribute("value", "false");
	}
	eMain.appendChild(eKillerAttackPopup);
	QDomElement eShowQueueLength = xmlDoc.createElement("show-queue-length");
	if (settingShowQueryLength) {
		eShowQueueLength.setAttribute("value", "true");
	} else {
		eShowQueueLength.setAttribute("value", "false");
	}
	eMain.appendChild(eShowQueueLength);
	QDomElement eResetQueueForUnknow = xmlDoc.createElement("reset-queue-if-unknow-status");
	if (settingResetQueueForUnknowStatus) {
		eResetQueueForUnknow.setAttribute("value", "true");
	} else {
		eResetQueueForUnknow.setAttribute("value", "false");
	}
	eMain.appendChild(eResetQueueForUnknow);
	QDomElement eResetQueuePopup = xmlDoc.createElement("show-popup-if-reset-queue");
	eResetQueuePopup.setAttribute("value", (settingResetQueuePopup) ? "true" : "false");
	eMain.appendChild(eResetQueuePopup);
	if (settingServerTextBlocksCount > 0) {
		QDomElement eServerTextBlocksCount = xmlDoc.createElement("server-text-max-blocks-count");
		eServerTextBlocksCount.setAttribute("value", settingServerTextBlocksCount);
		eMain.appendChild(eServerTextBlocksCount);
	}
	QDomElement eFight = xmlDoc.createElement("fight");
	eNewAccount.appendChild(eFight);
	if (settingFightTimer >= 0 && settingFightTimer <= 2) {
		QDomElement eFightTimer = xmlDoc.createElement("fight-timer-show");
		eFightTimer.setAttribute("value", fightTimerStrings[settingFightTimer]);
		eFight.appendChild(eFightTimer);
	}
	if (settingFightSelectAction >= 0 && settingFightSelectAction <= 2) {
		QDomElement eFightSelectAction = xmlDoc.createElement("fight-select-action");
		eFightSelectAction.setAttribute("value", fightSelectActionStrings.at(settingFightSelectAction));
		eFight.appendChild(eFightSelectAction);
	}
	if (settingFightAutoClose >= 0 && settingFightAutoClose <= 1) {
		QDomElement eFightAutoClose = xmlDoc.createElement("fight-auto-close");
		eFightAutoClose.setAttribute("value", fightAutoCloseStrings.at(settingFightAutoClose));
		eFight.appendChild(eFightAutoClose);
	}
	QDomElement eFingDropPopup = xmlDoc.createElement("fing-drop-popup");
	if (settingFingDropPopup) {
		eFingDropPopup.setAttribute("value", "true");
	} else {
		eFingDropPopup.setAttribute("value", "false");
	}
	eFight.appendChild(eFingDropPopup);
	QDomElement eStat = xmlDoc.createElement("statistic");
	eNewAccount.appendChild(eStat);

	// Сохраняем настройки слотов
	QDomElement eSlots = xmlDoc.createElement("slots");
	eNewAccount.appendChild(eSlots);
	int j;
	QDomElement eSlot;
	for (int i = 0; i < SLOT_ITEMS_COUNT; i++) {
		j = settingSlots[i];
		if (j >= 0 && j < STAT_PARAMS_COUNT) {
			if (valueXmlStrings[j].length() != 0) {
				eSlot = xmlDoc.createElement("slot");
				eSlot.setAttribute("num", QString::number(i+1));
				eSlot.setAttribute("param", valueXmlStrings[j]);
				eSlots.appendChild(eSlot);
			}
		}
	}
	// Сохраняем настройки алиасов
	QDomElement eAliases = Aliases::instance()->saveToDomElement(xmlDoc);
	if (!eAliases.isNull()) {
		eNewAccount.appendChild(eAliases);
	}
	// Сохраняем настройки внешнего вида (цвета, шрифты)
	QDomElement eAppearance = xmlDoc.createElement("appearance");
	eNewAccount.appendChild(eAppearance);
	QDomElement ePersNameAppe = xmlDoc.createElement("pers-name");
	eAppearance.appendChild(ePersNameAppe);
	QDomElement ePersNameFontAppe = xmlDoc.createElement("font");
	ePersNameAppe.appendChild(ePersNameFontAppe);
	ePersNameFontAppe.setAttribute("value", settingPersNameFont);
	QDomElement eServerTextAppe = xmlDoc.createElement("server-text");
	eAppearance.appendChild(eServerTextAppe);
	QDomElement eServerTextFontAppe = xmlDoc.createElement("font");
	eServerTextAppe.appendChild(eServerTextFontAppe);
	eServerTextFontAppe.setAttribute("value", settingServerTextFont);
	// Сохраняем настройки рюкзака
	QDomElement eFings = xmlDoc.createElement("backpack");
	Pers::instance()->exportBackpackSettingsToDomElement(&xmlDoc, &eFings);
	eNewAccount.appendChild(eFings);
	// Сохраняем настройки карт
	QDomElement eMaps = xmlDoc.createElement("maps");
	eNewAccount.appendChild(eMaps);
	if (settingMapsSaveMode >= 0 && settingMapsSaveMode < persSaveModeStrings.size()) {
		QDomElement eSaveParamMode = xmlDoc.createElement("maps-save-mode");
		eSaveParamMode.setAttribute("value", persSaveModeStrings.at(settingMapsSaveMode));
		eMaps.appendChild(eSaveParamMode);
	}
	// Сохранение
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

bool PluginCore::saveWindowSettings()
{
	if (accJid.isEmpty()) {
		return false;
	}
	if (settingWindowSizePos == 0) {
		return false;
	}
	QDomDocument xmlDoc;
	if (!loadPluginXml(&xmlDoc, "sofgame_settings.xml")) {
		return false;
	}
	QDomElement eRoot = xmlDoc.documentElement();
	if (eRoot.tagName() != "settings") {
		return false;
	}
	QDomNode childSettingsNode = eRoot.firstChild();
	QDomNode childJidNode;
	QDomNode childMainNode;
//	qint32 i, j;
//	QString str1;
	while (!childSettingsNode.isNull()) {
		if (childSettingsNode.toElement().tagName() == "account") {
			if (childSettingsNode.toElement().attribute("jid") == accJid) {
				childJidNode = childSettingsNode.firstChild();
				while (!childJidNode.isNull()) {
					if (childJidNode.toElement().tagName() == "main") {
						childMainNode = childJidNode.firstChild();
						while (!childMainNode.isNull()) {
							QString sTagName = childMainNode.toElement().tagName();
							if (sTagName == "window-save-params") {
								QDomElement eWindowParams = childMainNode.toElement();
								if (settingWindowPosX != QINT32_MIN) {
									eWindowParams.setAttribute("pos-x", QString::number(settingWindowPosX));
								}
								if (settingWindowPosY != QINT32_MIN) {
									eWindowParams.setAttribute("pos-y", QString::number(settingWindowPosY));
								}
								if (settingWindowWidth != QINT32_MIN) {
									eWindowParams.setAttribute("width", QString::number(settingWindowWidth));
								}
								if (settingWindowHeight != QINT32_MIN) {
									eWindowParams.setAttribute("height", QString::number(settingWindowHeight));
								}
								break;
							}
							childMainNode = childMainNode.nextSibling();
						}
						break;
					}
					childJidNode = childJidNode.nextSibling();
				}
				break;
			}
		}
		childSettingsNode = childSettingsNode.nextSibling();
	}
	return savePluginXml(&xmlDoc, "sofgame_settings.xml");
}

bool PluginCore::loadPersStatus()
{
	// Сброс статуса персонажа
	lastGameJid = "";
	lastChatJid = "";
	persStatus = NotKnow;
	statExperience = -1; statExperienceFull = -1;
	statMessagesCount = 0;
	statMoneysDropCount = 0;
	statFightsCount = 0;
	statFightDamageMin = -1;
	statFightDamageMax = -1;
	statFingsDropCount = 0;
	statFingDropLast = "";
	statExperienceDropCount = 0;
	statKilledEnemies = 0;
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

	Pers::instance()->beginSetPersParams();
	while (!childStatusNode.isNull()) {
		if (childStatusNode.toElement().tagName() == "account" && childStatusNode.toElement().attribute("jid") == accJid) {
			QDomNode childAccountNode = childStatusNode.firstChild();
			while (!childAccountNode.isNull()) {
				if (settingSavePersParam && childAccountNode.toElement().tagName() == "main") {
					if (settingSavePersParam) {
						childMainNode = childAccountNode.firstChild();
						Pers::instance()->beginSetPersParams();
						while (!childMainNode.isNull()) {
							if (childMainNode.toElement().tagName() == "pers-name") {
								Pers::instance()->setName(getTextFromNode(&childMainNode));
							} else if (childMainNode.toElement().tagName() == "pers-level") {
								Pers::instance()->setPersParams(VALUE_PERS_LEVEL, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							} else if (childMainNode.toElement().tagName() == "pers-experience-curr") {
								statExperience = getTextFromNode(&childMainNode).toInt();
							} else if (childMainNode.toElement().tagName() == "pers-experience-max") {
								statExperienceFull = getTextFromNode(&childMainNode).toInt();
							} else if (childMainNode.toElement().tagName() == "pers-health-curr") {
								Pers::instance()->setPersParams(VALUE_HEALTH_CURR, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							} else if (childMainNode.toElement().tagName() == "pers-health-max") {
								Pers::instance()->setPersParams(VALUE_HEALTH_MAX, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							} else if (childMainNode.toElement().tagName() == "pers-energy-curr") {
								Pers::instance()->setPersParams(VALUE_ENERGY_CURR, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							} else if (childMainNode.toElement().tagName() == "pers-energy-max") {
								Pers::instance()->setPersParams(VALUE_ENERGY_MAX, TYPE_INTEGER_FULL, getTextFromNode(&childMainNode).toInt());
							}
							childMainNode = childMainNode.nextSibling();
						}
						Pers::instance()->endSetPersParams();
					}
				} else if (settingSaveStat && childAccountNode.toElement().tagName() == "statistic") {
					if (settingSaveStat) {
						childStatNode = childAccountNode.firstChild();
						while (!childStatNode.isNull()) {
							sTagName = childStatNode.toElement().tagName();
							if (sTagName == "last-game-jid") {
								lastGameJid = getTextFromNode(&childStatNode);
							} else if (sTagName == "last-chat-jid") {
								lastChatJid = getTextFromNode(&childStatNode);
							} else if (sTagName == "in-messages-count") {
								statMessagesCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "fights-count") {
								statFightsCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "pers-fight-damage-max") {
								statFightDamageMax = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "pers-fight-damage-min") {
								statFightDamageMin = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "moneys-drop-count") {
								statMoneysDropCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "fings-drop-count") {
								statFingsDropCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "fing-drop-last") {
								statFingDropLast = getTextFromNode(&childStatNode);
							} else if (sTagName == "experience-drop-count") {
								statExperienceDropCount = getTextFromNode(&childStatNode).toInt();
							} else if (sTagName == "killed-enemies") {
								statKilledEnemies = getTextFromNode(&childStatNode).toInt();
							}
							childStatNode = childStatNode.nextSibling();
						}
					}
				} else if (settingSaveBackpack && childAccountNode.toElement().tagName() == "backpack") {
					// Обрабатываем данные о вещах
					if (settingSaveBackpack) {
						Pers::instance()->loadFingsFromDomNode(&childAccountNode);
					}
				}
				childAccountNode = childAccountNode.nextSibling();
			}
		}
		childStatusNode = childStatusNode.nextSibling();
	}
	Pers::instance()->endSetPersParams();
	return true;
}

bool PluginCore::loadPluginSettings()
{
	// Начальная инициализация
	settingPersName = "";
	settingSlots.fill(-1, SLOT_ITEMS_COUNT);
	settingPersSaveMode = 1;
	settingMapsSaveMode = 1;
	settingSavePersParam = true;
	settingSaveStat = true;
	settingSaveBackpack = true;
	settingChangeMirrorMode = 0;
	settingPersX = QINT32_MIN;
	settingPersY = QINT32_MIN;
	settingWindowSizePos = 0;
	settingWindowPosX = QINT32_MIN;
	settingWindowPosY = QINT32_MIN;
	settingWindowWidth = QINT32_MIN;
	settingWindowHeight = QINT32_MIN;
	settingFightTimer = -1;
	settingFightAutoClose = 0;
	settingFingDropPopup = true;
	settingWatchRestHealthEnergy = 0;
	settingFightSelectAction = 0;
	settingInKillersCupPopup = false;
	settingKillerAttackPopup = false;
	settingShowQueryLength = false;
	settingPersNameFont = "";
	settingServerTextFont = "";
	settingResetQueueForUnknowStatus = true;
	settingServerTextBlocksCount = 0;
	settingResetQueuePopup = false;
	// Загрузка настроек
	if (accJid.isEmpty()) {
		return false;
	}
	QDomDocument xmlDoc;
	if (!loadPluginXml(&xmlDoc, "sofgame_settings.xml")) {
		return false;
	}
	QDomElement eRoot = xmlDoc.documentElement();
	if (eRoot.tagName() != "settings") {
		return false;
	}
	QDomNode childSettingsNode = eRoot.firstChild();
	QDomNode childJidNode;
	QDomNode childMainNode;
	QDomNode childStatNode;
	QDomNode childSlotsNode;
	qint32 i, j;
	QString str1;
	while (!childSettingsNode.isNull()) {
		if (childSettingsNode.toElement().tagName() == "account") {
			if (childSettingsNode.toElement().attribute("jid") == accJid) {
				childJidNode = childSettingsNode.firstChild();
				while (!childJidNode.isNull()) {
					if (childJidNode.toElement().tagName() == "main") {
						childMainNode = childJidNode.firstChild();
						while (!childMainNode.isNull()) {
							QString sTagName = childMainNode.toElement().tagName();
							if (sTagName == "pers-name") {
								settingPersName = getTextFromNode(&childMainNode);
							} else if (sTagName == "pers-save-mode") {
								str1 = childMainNode.toElement().attribute("value");
								i = persSaveModeStrings.indexOf(str1);
								if (i != -1) {
									settingPersSaveMode = i;
								}
							} else if (sTagName == "save-pers-params") {
								settingSavePersParam = true;
								if (childMainNode.toElement().attribute("value").toLower() == "false") {
									settingSavePersParam = false;
								}
							} else if (sTagName == "save-pers-backpack") {
								settingSaveBackpack = true;
								if (childMainNode.toElement().attribute("value").toLower() == "false") {
									settingSaveBackpack = false;
								}
							} else if (sTagName == "save-statistic") {
								settingSaveStat = true;
								if (childMainNode.toElement().attribute("value").toLower() == "false") {
									settingSaveStat = false;
								}
							} else if (sTagName == "mirror-change-mode") {
								str1 = childMainNode.toElement().attribute("value");
								i = mirrorChangeModeStrings.indexOf(str1);
								if (i != -1) {
									settingChangeMirrorMode = i;
									mySender->changeGameMirrorsMode(i);
								}
							} else if (sTagName == "window-save-params") {
								settingWindowSizePos = 1;
								str1 = childMainNode.toElement().attribute("pos-x");
								settingWindowPosX = str1.toInt();
								str1 = childMainNode.toElement().attribute("pos-y");
								settingWindowPosY = str1.toInt();
								str1 = childMainNode.toElement().attribute("width");
								settingWindowWidth = str1.toInt();
								str1 = childMainNode.toElement().attribute("height");
								settingWindowHeight = str1.toInt();
							} else if (sTagName == "watch-rest-health-energy") {
								str1 = childMainNode.toElement().attribute("value");
								i = watchRestHealthEnergyStrings.indexOf(str1);
								if (i != -1) {
									settingWatchRestHealthEnergy = i;
									Pers::instance()->setSetting(SETTING_REST_HEALTH_ENERGY, i);
								}
							} else if (sTagName == "in-killers-cup-popup") {
								str1 = childMainNode.toElement().attribute("value");
								if (str1 == "true") {
									settingInKillersCupPopup = true;
								}
							} else if (sTagName == "killer-attack-popup") {
								str1 = childMainNode.toElement().attribute("value");
								if (str1 == "true") {
									settingKillerAttackPopup = true;
								}
							} else if (sTagName == "show-queue-length") {
								str1 = childMainNode.toElement().attribute("value");
								if (str1 == "true") {
									settingShowQueryLength = true;
								}
							} else if (sTagName == "reset-queue-if-unknow-status") {
								str1 = childMainNode.toElement().attribute("value");
								settingResetQueueForUnknowStatus = (str1 == "true");
							} else if (sTagName == "show-popup-if-reset-queue") {
								str1 = childMainNode.toElement().attribute("value");
								settingResetQueuePopup = (str1 == "true");
							} else if (sTagName == "server-text-max-blocks-count") {
								str1 = childMainNode.toElement().attribute("value");
								settingServerTextBlocksCount = str1.toInt();
							}
							childMainNode = childMainNode.nextSibling();
						}
					} else if (childJidNode.toElement().tagName() == "fight") {
						QDomNode childNode = childJidNode.firstChild();
						while (!childNode.isNull()) {
							QString sTagName = childNode.toElement().tagName();
							if (sTagName == "fight-timer-show") {
								str1 = childNode.toElement().attribute("value");
								i = fightTimerStrings.indexOf(str1);
								if (i != -1) {
									settingFightTimer = i;
								}
							} else if (sTagName == "fight-select-action") {
								str1 = childNode.toElement().attribute("value");
								i = fightSelectActionStrings.indexOf(str1);
								if (i != -1) {
									settingFightSelectAction = i;
								}
							} else if (sTagName == "fight-auto-close") {
								str1 = childNode.toElement().attribute("value");
								i = fightAutoCloseStrings.indexOf(str1);
								if (i != -1) {
									if (settingFightAutoClose != i) {
										settingFightAutoClose = i;
										if (i != 0) {
											connect(fight, SIGNAL(fightStart(int)), this, SLOT(fightStarted(int)));
										} else {
											disconnect(fight, SIGNAL(fightStart(int)), this, SLOT(fightStarted(int)));
										}
									}
								}
							} else if (sTagName == "fing-drop-popup") {
								str1 = childNode.toElement().attribute("value");
								if (str1 == "true") {
									settingFingDropPopup = true;
								}
							}
							childNode = childNode.nextSibling();
						}
					} else if (childJidNode.toElement().tagName() == "statistic") {
						childStatNode = childJidNode.firstChild();
						while (!childStatNode.isNull()) {
							if (childStatNode.toElement().tagName() == "save-statistic") {
								str1 = childStatNode.toElement().attribute("value");
								settingSaveStat = true;
								if (str1.toLower() == "false")
									settingSaveStat = false;
							}
							childStatNode = childStatNode.nextSibling();
						}
					} else if (childJidNode.toElement().tagName() == "slots") {
						childSlotsNode = childJidNode.firstChild();
						while (!childSlotsNode.isNull()) {
							if (childSlotsNode.toElement().tagName() == "slot") {
								str1 = childSlotsNode.toElement().attribute("param");
								if (!str1.isEmpty()) {
									for (i = 0; i < STAT_PARAMS_COUNT; i++) {
										if (str1 == valueXmlStrings[i]) {
											j = 0;
											j = childSlotsNode.toElement().attribute("num").toInt();
											if (j >= 1 && j <= SLOT_ITEMS_COUNT) {
												settingSlots[j-1] = i;
												break;
											}
										}
									}
								}
							}
							childSlotsNode = childSlotsNode.nextSibling();
						}
					} else if (childJidNode.toElement().tagName() == "aliases") {
						Aliases::instance()->loadFromDomElement(childJidNode.toElement());
					} else if (childJidNode.toElement().tagName() == "backpack") {
						// Обрабатываем настройки рюкзака
						if (Pers::instance()) {
							Pers::instance()->loadBackpackSettingsFromDomNode(&childJidNode);
						}
					} else if (childJidNode.toElement().tagName() == "appearance") {
						QDomNode eAppeChildNode = childJidNode.firstChild();
						while (!eAppeChildNode.isNull()) {
							if (eAppeChildNode.toElement().tagName() == "pers-name") {
								QDomNode ePersNameChildNode = eAppeChildNode.firstChild();
								while (!ePersNameChildNode.isNull()) {
									if (ePersNameChildNode.toElement().tagName() == "font") {
										settingPersNameFont = ePersNameChildNode.toElement().attribute("value");
									}
									ePersNameChildNode = ePersNameChildNode.nextSibling();
								}
							} else if (eAppeChildNode.toElement().tagName() == "server-text") {
								QDomNode eServerTextChildNode = eAppeChildNode.firstChild();
								while (!eServerTextChildNode.isNull()) {
									if (eServerTextChildNode.toElement().tagName() == "font") {
										settingServerTextFont = eServerTextChildNode.toElement().attribute("value");
									}
									eServerTextChildNode = eServerTextChildNode.nextSibling();
								}
							}
							eAppeChildNode = eAppeChildNode.nextSibling();
						}
					} else if (childJidNode.toElement().tagName() == "maps") {
						QDomNode childMapsNode = childJidNode.firstChild();
						while (!childMapsNode.isNull()) {
							QString sTagName = childMapsNode.toElement().tagName();
							if (sTagName == "maps-save-mode") {
								str1 = childMapsNode.toElement().attribute("value");
								i = persSaveModeStrings.indexOf(str1);
								if (i != -1) {
									settingMapsSaveMode = i;
								}
							}
							childMapsNode = childMapsNode.nextSibling();
						}
					}
					childJidNode = childJidNode.nextSibling();
				}
				break;
			}
		}
		childSettingsNode = childSettingsNode.nextSibling();
	}
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
		if (new_cmd.isEmpty() || !str1.startsWith("/")) {
			// Отсылаем строку серверу
			mySender->sendString(str1);
			return true;
		}
	}
	if (str1.startsWith("/help") || str1.startsWith("help")) {
		str1 = "--= help =--\n";
		str1.append(QString::fromUtf8("/- — Сброс очереди команд\n"));
		str1.append(QString::fromUtf8("/1... — Позволяет отдавать односимвольные числовые команды в игру без дублирования их клавишей <Enter>.\n  /1+ — Включение режима автоввода.\n  /1- — Отключение режима автоввода.\n  /1 — Состояние режима (вкл или выкл).\n  /1-<числовая_команда> — Отправка длинной команды без отключения режима. Пример: /1-02 — Отправка 02 игре не отключая режимом автоввода.\n"));
		str1.append(QString::fromUtf8("/aliases — управление алиасами команд\n"));
		str1.append(QString::fromUtf8("/clear — очистка и сброс различных данных\n"));
		str1.append(QString::fromUtf8("/help — этот текст помощи\n"));
		str1.append(QString::fromUtf8("/maps — информация и управление картами\n"));
		str1.append("/pers info — information about current pers\n/pers info2 — detail information about current pers\n/pers info <name> — information about pers by name\n/pers info2 <name> — detail information about pers by name\n/pers list — list stored pers info\n");
		str1.append("/send_delta — get pause value for command send\n/send_delta=n — set pause value for command send in msec.\n");
		str1.append(QString::fromUtf8("/server_timeout — показать время ожидания ответа сервера\n/server_timeout=n — установить время ожидания сервера в n сек.\n"));
		str1.append(QString::fromUtf8("/settings — Управление настройками\n"));
		str1.append(QString::fromUtf8("/stat — полная статистика\n/stat 1 — общая статистика\n/stat 2 — статистика боев\n/stat 9 — статистика зеркал\n"));
		str1.append(QString::fromUtf8("/things — вещи, фильтры вещей, цены\n"));
		str1.append(QString::fromUtf8("/ver — версия плагина\n"));
		setConsoleText(str1, true);
	} else if (str1 == "/-") {
		int q_len = mySender->getGameQueueLength();
		if (q_len > 0) {
			mySender->resetGameQueue();
			str1 = QString::fromUtf8("### Очередь сброшена. Количество: %1 ###").arg(q_len);
		} else {
			str1 = QString::fromUtf8("### Очередь пуста. ###");
		}
		setGameText(str1);
		setConsoleText(str1, false);
	} else if (str1 == "/stat" || str1.startsWith("/stat ")) {
		getStatistics(&str1);
	} else if (str1 == "/send_delta") {
		str1 = "send_delta = " + QString::number(mySender->getSendDelta()) + " msec.";
		setConsoleText(str1, true);
	} else if (str1.startsWith("/send_delta=")) {
		if (str1.length() >= 13) {
			bool res;
			int nDelta = str1.mid(12).toInt(&res);
			str1 = "send_delta not set";
			if (res) {
				if (mySender->setSendDelta(nDelta)) {
					str1 = "send_delta set is " + QString::number(mySender->getSendDelta()) + " msec.";
				}
			}
			setConsoleText(str1, true);
		}
	} else if (str1 == "/server_timeout") {
		str1 = "server_timeout = " + QString::number(mySender->getServerTimeoutDuration()) + " sec.";
		setConsoleText(str1, true);
	} else if (str1.startsWith("/server_timeout=")) {
		if (str1.length() >= 17) {
			bool res;
			int nTimeout = str1.mid(16).toInt(&res);
			str1 = "server timeout not set";
			if (res) {
				if (mySender->setServerTimeoutDuration(nTimeout)) {
					str1 = "server_timeout set is " + QString::number(mySender->getServerTimeoutDuration()) + " sec.";
				}
			}
			setConsoleText(str1, true);
		}
	} else if (str1.startsWith("/maps")) {
		QStringList mapsCmd = splitCommandString(str1);
		mapsCommands(&mapsCmd);
	} else if (str1.startsWith("/pers")) {
		QStringList persCmd = str1.split(" ");
		persCommands(&persCmd);
	} else if (str1.startsWith("/clear")) {
		QStringList clearCmd = str1.split(" ");
		clearCommands(&clearCmd);
	} else if (str1.startsWith("/things")) {
		QStringList thingsCmd = str1.split(" ");
		fingsCommands(&thingsCmd);
	} else if (str1.startsWith("/aliases")) {
		QStringList aliasesCmd = splitCommandString(str1);
		aliasesCommands(aliasesCmd);
	} else if (str1.startsWith("/settings")) {
		QStringList settingsCmd = str1.split(" ");
		settingsCommands(settingsCmd);
	} else if (str1 == "/ver") {
		setConsoleText(cVer, true);
	} else {
		str1 = QString::fromUtf8("Неизвестная команда. Наберите /help для получения помощи\n");
		setConsoleText(str1, true);
	}
	return true;
}

void PluginCore::getStatistics(QString* commandPtr)
{
	int mode = 0;
	QStringList args = commandPtr->split(" ");
	if (args.size() == 2) {
		if (args[1] == "1") {
			mode = 1;
		} else if (args[1] == "2") {
			mode = 2;
		} else if (args[1] == "9") {
			mode = 9;
		}
	}
	QString str1;
	int i;
	QString stat_str = QString::fromUtf8("--= Статистика =--\n");
	if (mode == 0 || mode == 1) {
		stat_str.append(QString::fromUtf8("--Общая статистика--\n"));
		stat_str.append(QString::fromUtf8("Jid игры: "));
		if (getTextValue(VALUE_LAST_GAME_JID, &str1)) {
			stat_str.append(str1);
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nJid чата: "));
		if (getTextValue(VALUE_LAST_CHAT_JID, &str1)) {
			stat_str.append(str1);
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nВсего сообщений: "));
		if (getIntValue(VALUE_MESSAGES_COUNT, &i)) {
			stat_str.append(numToStr(i, "'"));
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\n"));
	}
	if (mode == 0 || mode == 2) {
		stat_str.append(QString::fromUtf8("--Статистика боев--\n"));
		stat_str.append(QString::fromUtf8("Всего боев: "));
		if (getIntValue(VALUE_FIGHTS_COUNT, &i)) {
			stat_str.append(numToStr(i, "'"));
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nЛучший удар: "));
		if (getIntValue(VALUE_DAMAGE_MAX_FROM_PERS, &i)) {
			stat_str.append(numToStr(i, "'"));
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nХудший удар: "));
		if (getIntValue(VALUE_DAMAGE_MIN_FROM_PERS, &i)) {
			stat_str.append(numToStr(i, "'"));
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nДенег собрано: "));
		if (getIntValue(VALUE_DROP_MONEYS, &i)) {
			stat_str.append(numToStr(i, "'"));
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nВещей собрано: "));
		if (getIntValue(VALUE_FINGS_DROP_COUNT, &i)) {
			stat_str.append(numToStr(i, "'"));
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nПоследняя вещь: "));
		if (getTextValue(VALUE_FING_DROP_LAST, &str1)) {
			stat_str.append(str1);
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nПолученный опыт: "));
		if (getIntValue(VALUE_EXPERIENCE_DROP_COUNT, &i)) {
			stat_str.append(numToStr(i, "'"));
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\nПовержено врагов: "));
		if (getIntValue(VALUE_KILLED_ENEMIES, &i)) {
			stat_str.append(numToStr(i, "'"));
		} else {
			stat_str.append(NA_TEXT);
		}
		stat_str.append(QString::fromUtf8("\n"));
	}
	if (mode == 0 || mode == 9) {
		stat_str.append(QString::fromUtf8("--Статистика зеркал игры--\n"));
		QStringList game_jids = mySender->getGameJids();
		stat_str.append(QString::fromUtf8("Всего зеркал: "));
		stat_str.append(QString::number(game_jids.size()));
		int jid_index = 0;
		while (!game_jids.isEmpty()) {
			QString jid = game_jids.takeFirst();
			const struct Sender::jid_status* jstat = mySender->getGameJidInfo(jid_index);
			if (jstat != 0) {
				stat_str.append(QString::fromUtf8("\n-Зеркало: "));
				stat_str.append(jid);
				stat_str.append(QString::fromUtf8("\nстатус: "));
				int status = jstat->status;
				if (status == 0) {
					stat_str.append(QString::fromUtf8("отключено"));
				} else {
					stat_str.append(QString::fromUtf8("доступно"));
				}
				stat_str.append(QString::fromUtf8("\nпоследняя смена статуса: "));
				if (jstat->last_status.isValid()) {
					stat_str.append(jstat->last_status.toString("dd.MM.yyyy hh:mm:ss.zzz"));
				} else {
					stat_str.append(NA_TEXT);
				}
				if (status != 0) {
					stat_str.append(QString::fromUtf8("\nпоследняя отправка: "));
					if (jstat->last_send.isValid()) {
						stat_str.append(jstat->last_send.toString("dd.MM.yyyy hh:mm:ss.zzz"));
					} else {
						stat_str.append(NA_TEXT);
					}
					stat_str.append(QString::fromUtf8("\nпоследний пинг: "));
					if (jstat->last_send_ping.isValid()) {
						stat_str.append(jstat->last_send_ping.toString("dd.MM.yyyy hh:mm:ss.zzz"));
					} else {
						stat_str.append(NA_TEXT);
					}
					stat_str.append(QString::fromUtf8("\nколичество пакетов: "));
					stat_str.append(QString::number(jstat->probe_count));
					stat_str.append(QString::fromUtf8("\nСредний отклик: "));
					double inSec = jstat->resp_average / 1000.0;
					stat_str.append(QString::number(inSec));
					stat_str.append(QString::fromUtf8(" сек."));
				}
			}
			jid_index++;
		}
		stat_str.append(QString::fromUtf8("\n"));
	}

	setConsoleText(stat_str, true);
}

void PluginCore::mapsCommands(QStringList* args)
{
	/**
	* Отображение данных по картам
	**/
	if ((*args)[0] != "/maps")
		return;
	int cntArgs = args->size() - 1;
	QString stat_str = QString::fromUtf8("--= Карты =--\n");
	if (cntArgs == 0) {
		stat_str.append(QString::fromUtf8("/maps clear <index> - Очистка всего содержимого карты с индексом <index>\n/maps export <index> <exp_file> - Экспорт карты с индексом <index> в файл с именем <exp_file>\n/maps import <imp_file> - Импорт карт из файла\n/maps info - основная информация о картах\n/maps list - список всех карт\n/maps merge <index1> <index2> - Объединение карт\n/maps remove <index> - Удаление карты\n/maps rename <index> <new_name> - Переименование карты\n/maps switch <index> - переключение на карту с указанным индексом\n/maps unload <index>- выгрузка карты из памяти без сохранения изменений\n"));
		setConsoleText(stat_str, true);
	} else if ((*args)[1] == "clear") {
		if (cntArgs == 2) {
			bool fOk;
			int map1 = (*args)[2].toInt(&fOk);
			if (fOk) {
				if (GameMap::instance()->clearMap(map1)) {
					stat_str.append(QString::fromUtf8("Карта успешно очищена\n"));
				} else {
					stat_str.append(QString::fromUtf8("Ошибка: нет такой карты\n"));
				}
			} else {
				stat_str.append(QString::fromUtf8("Ошибка: некорректный индекс карты\n"));
			}
		} else {
			stat_str.append(QString::fromUtf8("Ошибка: неверное количество аргументов\n"));
		}
		setConsoleText(stat_str, true);
		return;
	} else if ((*args)[1] == "rename") {
		if (cntArgs >= 3) {
			QStringList aName = *args;
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
							stat_str.append(QString::fromUtf8("Карта переименована\n"));
							break;
						case 1:
							stat_str.append(QString::fromUtf8("Ошибка: нет такой карты\n"));
							break;
						case 2:
							stat_str.append(QString::fromUtf8("Ошибка: карта с таким именем уже существует\n"));
							break;
						default:
							stat_str.append(QString::fromUtf8("Ошибка\n"));
							break;
					}
				} else {
					stat_str.append(QString::fromUtf8("Ошибка: необходимо указать новое имя карты\n"));
				}
			} else {
				stat_str.append(QString::fromUtf8("Ошибка: необходимо указать индекс карты\n"));
			}
		} else {
			stat_str.append(QString::fromUtf8("Ошибка: неверное количество аргументов\n"));
		}
		setConsoleText(stat_str, true);
		return;
	} else if ((*args)[1] == "export") {
		if (cntArgs == 3) {
			QStringList maps = (*args)[2].split(",");
			int type = 1; //0;
			//if ((*args)[4] == "xml") {
			//	type = 1;
			//}
			int nRes = GameMap::instance()->exportMaps(maps, type, (*args)[3]);
			switch (nRes) {
				case 0:
					stat_str.append(QString::fromUtf8("Экспорт успешно завершен\n"));
					break;
				case 1:
					stat_str.append(QString::fromUtf8("Ошибка: неверные аргументы\n"));
					break;
				case 2:
					stat_str.append(QString::fromUtf8("Ошибка: нет карт для экспорта\n"));
					break;
				case 3:
					stat_str.append(QString::fromUtf8("Ошибка: неудачная запись в файл\n"));
					break;
				default:
					stat_str.append(QString::fromUtf8("Ошибка: ошибка экспорта\n"));
			}
		} else {
			stat_str.append(QString::fromUtf8("Ошибка: неверное количество аргументов\n"));
		}
		setConsoleText(stat_str, true);
		return;
	} else if ((*args)[1] == "import") {
		if (cntArgs == 2) {
			int nRes = GameMap::instance()->importMaps((*args)[2]);
			switch (nRes) {
				case 0:
					stat_str.append(QString::fromUtf8("Импорт успешно завершен\n"));
					break;
				case 1:
					stat_str.append(QString::fromUtf8("Ошибка: ошибка чтения XML файла карт\n"));
					break;
				case 2:
					stat_str.append(QString::fromUtf8("Ошибка: некорректрый формат карт или не совпадает версия\n"));
					break;
				default:
					stat_str.append(QString::fromUtf8("Ошибка: ошибка импорта\n"));
			}
		} else {
			stat_str.append(QString::fromUtf8("Ошибка: неверное количество аргументов\n"));
		}
		setConsoleText(stat_str, true);
		return;
	} else if ((*args)[1] == "merge") {
		if (cntArgs == 3) {
			bool fOk;
			int map1 = (*args)[2].toInt(&fOk);
			if (fOk) {
				int map2 = (*args)[3].toInt(&fOk);
				if (fOk) {
					if (GameMap::instance()->mergeMaps(map1, map2)) {
						stat_str.append(QString::fromUtf8("Объединение успешно завершено\n"));
					} else {
						stat_str.append(QString::fromUtf8("Ошибка: ошибка объединения карт\n"));
					}
				} else {
					stat_str.append(QString::fromUtf8("Ошибка: некорректный индекс второй карты\n"));
				}
			} else {
				stat_str.append(QString::fromUtf8("Ошибка: некорректный индекс первой карты\n"));
			}
		} else {
			stat_str.append(QString::fromUtf8("Ошибка: неверное количество аргументов\n"));
		}
		setConsoleText(stat_str, true);
		return;
	} else if ((*args)[1] == "remove") {
		if (cntArgs == 2) {
			bool fOk;
			int map1 = (*args)[2].toInt(&fOk);
			if (fOk) {
				if (GameMap::instance()->removeMap(map1)) {
					stat_str.append(QString::fromUtf8("Карта успешно удалена\n"));
				} else {
					stat_str.append(QString::fromUtf8("Ошибка: нет такой карты\n"));
				}
			} else {
				stat_str.append(QString::fromUtf8("Ошибка: некорректный индекс карты\n"));
			}
		} else {
			stat_str.append(QString::fromUtf8("Ошибка: неверное количество аргументов\n"));
		}
		setConsoleText(stat_str, true);
		return;
	} else if (cntArgs == 1) {
		if ((*args)[1] == "info") {
			struct GameMap::maps_info mapsInf;
			GameMap::instance()->mapsInfo(&mapsInf);
			stat_str.append(QString::fromUtf8("Всего найдено карт: "));
			stat_str.append(QString::number(mapsInf.maps_count));
			stat_str.append(QString::fromUtf8("\nВсего загружено карт: "));
			stat_str.append(QString::number(mapsInf.maps_loaded));
			stat_str.append(QString::fromUtf8("\nТекущая карта: "));
			int curMap = mapsInf.curr_map_index;
			if (curMap != -1) {
				stat_str.append(QString::number(curMap));
				stat_str.append(" - " + mapsInf.curr_map_name);
			} else {
				stat_str.append(NA_TEXT);
			}
			stat_str.append("\n");
			setConsoleText(stat_str, true);
		} else if ((*args)[1] == "list") {
			QVector<GameMap::maps_list2> mapsLst;
			GameMap::instance()->getMapsList(&mapsLst);
			int cntMaps = mapsLst.size();
			for (int i = 0; i < cntMaps; i++) {
				stat_str.append(QString::number(mapsLst[i].index) + " - " + mapsLst[i].name);
				if (mapsLst[i].loaded) {
					stat_str.append(tr(" [loaded]"));
				}
				stat_str.append("\n");
			}
			stat_str.append(QString::fromUtf8("Всего карт: ") + QString::number(cntMaps) + "\n");
			setConsoleText(stat_str, true);
		}
	} else if (cntArgs == 2) {
		if ((*args)[1] == "switch") {
			bool fOk = false;
			int mapIndex = (*args)[2].toInt(&fOk);
			if (fOk) {
				if (!GameMap::instance()->switchMap(mapIndex)) {
					fOk = false;
				}
			}
			if (fOk) {
				stat_str.append("Switched\n");
			} else {
				stat_str.append("Error: map not found\n");
			}
			setConsoleText(stat_str, true);
			return;
		} else if ((*args)[1] == "unload") {
			bool fOk = false;
			int mapIndex = (*args)[2].toInt(&fOk);
			if (fOk) {
				if (!GameMap::instance()->unloadMap(mapIndex)) {
					fOk = false;
				}
			}
			if (fOk) {
				stat_str.append("Unloaded.\n");
				setConsoleText(stat_str, true);
			}
			return;
		}
	} else {
		stat_str.append(QString::fromUtf8("Ошибка: неизвестный параметр\n"));
		setConsoleText(stat_str, true);
	}
}

PersInfo* PluginCore::getPersInfo(QString pers_name)
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

void PluginCore::persCommands(QStringList* args)
{
	/**
	* Отображение данных о персонаже
	**/
	if ((*args)[0] != "/pers")
		return;
	int cntArgs = args->size() - 1;
	QString str1 = QString::fromUtf8("----=== Персонаж ===----");
	if (cntArgs == 0) {
		str1.append(QString::fromUtf8("\n/pers list - список имеющихся данных о персонажах"));
		str1.append(QString::fromUtf8("\n/pers info - краткая информация о собственном персонаже\n/pers info2 - подробная информация о собственном персонаже"));
		str1.append(QString::fromUtf8("\n/pers info <name> - краткая информация о персонаже <name>\n/pers info2 <name> - подробная информация о персонаже <name>"));
	} else if (cntArgs >= 1 && ((*args)[1] == "info" || (*args)[1] == "info2")) {
		int inf_ver = 1;
		if ((*args)[1] == "info2") {
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
				sPersName = (*args)[2];
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
				if (persInfo->getSitizenship(&str2)) {
					str1.append(str2);
				} else {
					str1.append(NA_TEXT);
				}
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
	} else if (cntArgs == 1 && (*args)[1] == "list") {
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
	setConsoleText(str1, true);
}

void PluginCore::clearCommands(QStringList* args)
{
	/**
	* Обработка команд уровня clear
	**/
	if ((*args)[0] != "/clear")
		return;
	int argsCount = args->size() - 1;
	QString str1 = "";
	if (argsCount > 0) {
		if ((*args)[1] == "text") {
			if (argsCount == 2) {
				if ((*args)[2] == "all") {
					setGameText("");
					setConsoleText("", false);
					return;
				}
				if ((*args)[2] == "game") {
					setGameText("");
					setConsoleText(QString::fromUtf8("Выполнено\n"), false);
					return;
				}
				if ((*args)[2] == "console") {
					setConsoleText("", false);
					return;
				}
			}
			str1.append(QString::fromUtf8("/clear text game - Очистка окна вывода игровых данных\n/clear text console - Очистка окна вывода плагина\n"));
		} else if ((*args)[1] == "stat") {
			int level = 0;
			if (argsCount >= 2) {
				level = (*args)[2].toInt();
			}
			if (level == 0 || level == 1) {
				resetStatistic(VALUE_LAST_GAME_JID);
				resetStatistic(VALUE_LAST_CHAT_JID);
				resetStatistic(VALUE_MESSAGES_COUNT);
				str1.append(QString::fromUtf8("Общая статистика сброшена\n"));
			}
			if (level == 0 || level == 2) {
				resetStatistic(VALUE_FIGHTS_COUNT);
				resetStatistic(VALUE_DAMAGE_MAX_FROM_PERS);
				resetStatistic(VALUE_DAMAGE_MIN_FROM_PERS);
				resetStatistic(VALUE_DROP_MONEYS);
				resetStatistic(VALUE_FINGS_DROP_COUNT);
				resetStatistic(VALUE_FING_DROP_LAST);
				resetStatistic(VALUE_EXPERIENCE_DROP_COUNT);
				resetStatistic(VALUE_KILLED_ENEMIES);
				str1.append(QString::fromUtf8("Статистика боев сброшена\n"));
			}
		}
	}
	if (str1.isEmpty()) {
		str1 = QString::fromUtf8("/clear text - Очистка окна вывода игровых данных\n/clear stat - Сброс всей статистики\n/clear stat n - Сброс статистики группы n\n");
	}
	setConsoleText(str1, true);
}

/**
 * Обработка команд уровня things
 */
void PluginCore::fingsCommands(QStringList* args)
{
	if ((*args)[0] != "/things")
		return;
	int argsCount = args->size() - 1;
	QString str1 = QString::fromUtf8("----=== Вещи ===----\n");
	if (argsCount == 0) {
		str1.append(QString::fromUtf8("/things list или /things list 0 - отображение всех вещей\n/things list n - отображение вещей фильтра n\n/things filters list - отображение фильтров\n/things price list - отображение цен на вещи\n"));
	} else if ((*args)[1] == "list") {
		int filterNum = 0;
		bool fOk = true;
		if (argsCount == 2) {
			filterNum = (*args)[2].toInt(&fOk);
		}
		if (fOk && filterNum >= 0) {
			QList<FingFilter*> filtersList;
			Pers::instance()->getFingsFiltersEx(&filtersList);
			if (filterNum <= filtersList.size()) {
				int iface = Pers::instance()->getThingsInterface();
				if (filterNum == 0) {
					str1.append(QString::fromUtf8("--- Все вещи ---\n"));
				} else {
					QString sFltrName = "";
					if (filterNum > 0 && filterNum <= filtersList.size()) {
						sFltrName = filtersList.at(filterNum-1)->name().trimmed();
					}
					if (sFltrName.isEmpty()) {
						sFltrName = NA_TEXT;
					}
					str1.append(QString::fromUtf8("--- Вещи из фильтра \"%1\" ---\n").arg(sFltrName));
					Pers::instance()->setThingsInterfaceFilter(iface, filterNum);
				}
				int fingsCnt = 0;
				while (true) {
					Thing* thing = Pers::instance()->getFingByRow(fingsCnt, iface);
					if (!thing) break;
					str1.append(thing->toString(Thing::ShowAll));
					str1.append("\n");
					fingsCnt++;
				}
				if (fingsCnt > 0) {
					int nCountAll = Pers::instance()->getFingsCount(iface);
					int nPriceAll = Pers::instance()->getPriceAll(iface);
					str1.append(QString::fromUtf8("Всего предметов: %1, на сумму: %2 дринк.\n").arg(numToStr(nCountAll, "'")).arg(numToStr(nPriceAll, "'")));
					int noPrice = Pers::instance()->getNoPriceCount(iface);
					if (noPrice > 0) {
						str1.append(QString::fromUtf8("Предметов без цены: %1.\n").arg(numToStr(noPrice, "'")));
					}
				} else {
					str1.append(QString::fromUtf8("список вещей пуст\n"));
				}
				Pers::instance()->removeThingsInterface(iface);
			} else {
				str1.append(QString::fromUtf8("нет такого фильтра. См. /things filters list\n"));
			}
		} else {
			str1.append(QString::fromUtf8("необходимо указать номер фильтра. См. /things filters list\n"));
		}
	} else if ((*args)[1] == "filters") {
		str1.append(QString::fromUtf8("--- Фильтры вещей ---\n"));
		if (argsCount == 2 && (*args)[2] == "list") {
			QList<FingFilter*> filtersList;
			Pers::instance()->getFingsFiltersEx(&filtersList);
			int cntFilters = filtersList.size();
			if (cntFilters > 0) {
				for (int i = 0; i < cntFilters; i++) {
					str1.append(QString::number(i+1) + " - ");
					str1.append(filtersList.at(i)->isActive() ? "[+] " : "[-] ");
					str1.append(filtersList.at(i)->name() + "\n");
				}
				str1.append(QString::fromUtf8("Всего фильтров: %1").arg(cntFilters));
			} else {
				str1.append(QString::fromUtf8("нет фильтров\n"));
			}
		} else {
			str1.append(QString::fromUtf8("неверный аргумент\n"));
		}
	} else if ((*args)[1] == "price") {
		str1.append(QString::fromUtf8("--- Цены вещей ---\n"));
		if (argsCount == 2 && (*args)[2] == "list") {
			QVector<Pers::price_item>* priceListPrt = Pers::instance()->getFingsPrice();
			int sizePrice = priceListPrt->size();
			if (sizePrice > 0) {
				for (int i = 0; i < sizePrice; i++) {
					str1.append(priceListPrt->at(i).name);
					int nType = priceListPrt->at(i).type;
					QString sType = thingTypeToString(nType);
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
					str1.append(";\n");
				}
				str1.append(QString::fromUtf8("Всего позиций: %1").arg(numToStr(sizePrice, "'")));
			} else {
				str1.append(QString::fromUtf8("нет цен\n"));
			}
		} else {
			str1.append(QString::fromUtf8("неверный аргумент\n"));
		}
	} else {
		str1.append(QString::fromUtf8("неверный аргумент\n"));
	}
	setConsoleText(str1, true);
}

/**
 * Обработка команд уровня aliases
 */
void PluginCore::aliasesCommands(const QStringList &args)
{
	if (args.at(0) != "/aliases")
		return;
	int argsCount = args.size() - 1;
	QString str1 = QString::fromUtf8("----=== Алиасы ===----\n");
	if (argsCount == 0) {
		str1.append(QString::fromUtf8("/aliases list — список всех алиасов\n/aliases append <имя> <префикс? yes|no> <command> — Добавление нового алиаса\n/aliases remove <alias_index> — Удаление алиаса по номеру\n"));
	} else if (args.at(1) == "list") {
		str1.append(QString::fromUtf8("--- Список алиасов ---\n"));
		Aliases *pAliases = Aliases::instance();
		int cnt = pAliases->count();
		if (cnt != 0) {
			str1.append(QString::fromUtf8("\n  Имя\tПрефикс\tКоманда\n"));
			for (int i = 0; i < cnt; i++) {
				str1.append(QString::fromUtf8("\n  %1 - %2\t%3\t%4")
					.arg(i+1)
					.arg(pAliases->aliasName(i))
					.arg(pAliases->aliasPrefix(i) ? QString::fromUtf8("да") : QString::fromUtf8("нет"))
					.arg(pAliases->aliasCommand(i)));
			}
			str1.append(QString::fromUtf8("\nВсего записей: %1").arg(cnt));
		} else {
			str1.append(QString::fromUtf8("список пуст\n"));
		}
	} else if (args.at(1) == "append") {
		str1.append(QString::fromUtf8("--- Добавление алиаса ---\n"));
		bool res = false;
		if (argsCount >= 4) {
			if (args.at(3) == "yes" || args.at(3) == "no") {
				if (Aliases::instance()->appendAlias(args.at(2), args.at(3) == "yes", args.at(4))) {
					str1.append(QString::fromUtf8("Добавлено без сохранения\n"));
				} else {
					str1.append(QString::fromUtf8("Ошибка!\n"));
				}
				res = true;
			}
		}
		if (!res) {
			str1.append("/aliases append <text> <yes|no> <text>\n");
		}
	} else if (args.at(1) == "remove") {
		str1.append(QString::fromUtf8("--- Удаление алиаса ---\n"));
		bool res = false;
		if (argsCount == 2) {
			bool fOk = false;
			const int idx = args.at(2).toInt(&fOk);
			if (fOk) {
				if (Aliases::instance()->removeAlias(idx - 1)) {
					str1.append(QString::fromUtf8("Удалено без сохранения\n"));
				} else {
					str1.append(QString::fromUtf8("Ошибка!\n"));
				}
				res = true;
			}
		}
		if (!res) {
			str1.append("/aliases remove <alias_index>\n");
		}
	} else {
		str1.append(QString::fromUtf8("неверный аргумент\n"));
	}
	setConsoleText(str1, true);
}

/**
 * Обработка команд уровня settings
 */
void PluginCore::settingsCommands(const QStringList &args)
{
	if (args.at(0) != "/settings")
		return;
	int argsCount = args.size() - 1;
	QString str1 = QString::fromUtf8("----=== Настройки ===----\n");
	if (argsCount == 0) {
		str1.append(QString::fromUtf8("/settings save — Сохранение настроек плагина\n"));
	} else if (args.at(1) == "save") {
		str1.append(QString::fromUtf8("--- Сохранение настроек ---\n"));
		savePluginSettings();
		str1.append(QString::fromUtf8("Сохранено\n"));
	} else {
		str1.append(QString::fromUtf8("неверный аргумент\n"));
	}
	setConsoleText(str1, true);
}

/**
 * Инициализация всплывающего окна
 */
void PluginCore::initPopup(QString title, QString string, int secs)
{
	// Устанавливаем свои настройки
	int msecs = secs * 1000;
	int delay_ = psiOptions->getGlobalOption("options.ui.notifications.passive-popups.delays.status").toInt();
	bool enbl_ = psiOptions->getGlobalOption("options.ui.notifications.passive-popups.enabled").toBool();
	if (delay_ != msecs) {
		QVariant delay(msecs);
		psiOptions->setGlobalOption("options.ui.notifications.passive-popups.delays.status", delay);
	}
	if (!enbl_) {
		QVariant enbl(true);
		psiOptions->setGlobalOption("options.ui.notifications.passive-popups.enabled", enbl);
	}
	// Вызываем popup
	myPopupHost->initPopup(string, title);
	// Восстанавливаем настройки
	if (delay_ != msecs) {
		QVariant delay(delay_);
		psiOptions->setGlobalOption("options.ui.notifications.passive-popups.delays.status", delay);
	}
	if (!enbl_) {
		QVariant enbl(false);
		psiOptions->setGlobalOption("options.ui.notifications.passive-popups.enabled", enbl);
	}
}

void PluginCore::persParamChanged()
{
	persStatusChangedFlag = true;
	if (settingPersSaveMode == 2) { // Автосохранение через 5 минут
		if (!saveStatusTimer.isActive()) {
			saveStatusTimer.start(5000*60);
		}
	}
}

void PluginCore::persBackpackChanged()
{
	persBackpackChangedFlag = true;
	if (settingPersSaveMode == 2) { // Автосохранение через 5 минут
		if (!saveStatusTimer.isActive()) {
			saveStatusTimer.start(5000*60);
		}
	}
}

void PluginCore::statisticsChanged()
{
	statisticChangedFlag = true;
	if (settingPersSaveMode == 2) { // Автосохранение через 5 минут
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
