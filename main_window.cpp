/*
 * main_window.cpp - Sof Game Psi plugin
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

#include <QtGui>
#include <QRectF>
#include <QGraphicsItem>

#include "main_window.h"
#include "common.h"
#include "plugin_core.h"
#include "maps/game_map.h"
#include "maps/mapmarkedit.h"
#include "utils.h"
#include "pers_info.h"
#include "sender.h"
#include "settings.h"
#include "pluginhosts.h"
#include "subclasses/textview.h"
#include "statistic/statistic.h"

#define SLOTS_COUNT 9

QList< QPair<int, QString> > SofMainWindow::statisticXmlStrings = QList< QPair<int, QString> >()
				<< QPair<int, QString>(Statistic::StatLastGameJid, "last-game-jid")
				<< QPair<int, QString>(Statistic::StatLastChatJid, "last-chat-jid")
				<< QPair<int, QString>(Statistic::StatMessagesCount, "messages-count")
				<< QPair<int, QString>(Statistic::StatDamageMaxFromPers, "damage-max-from-pers")
				<< QPair<int, QString>(Statistic::StatDamageMinFromPers, "damage-min-from-pers")
				<< QPair<int, QString>(Statistic::StatFightsCount, "fights-count")
				<< QPair<int, QString>(Statistic::StatDropMoneys, "drop-moneys")
				<< QPair<int, QString>(Statistic::StatThingsDropCount, "fings-drop-count")
				<< QPair<int, QString>(Statistic::StatThingDropLast, "fing-drop-last")
				<< QPair<int, QString>(Statistic::StatExperienceDropCount, "experience-drop-count")
				<< QPair<int, QString>(Statistic::StatKilledEnemies, "killed-enemies");


SofMainWindow::SofMainWindow() : QWidget(0)
{
	timeoutStamp = 0;
	timeoutTimer = 0;
	setupUi(this);
#ifdef Q_WS_WIN
	// В некоторых случаях фреймы с QFrame::StyledPanel в Windows не отображаются.
	// Похоже зависит от текущей темы. Ставим прямые углы для Windows
	header->setFrameShape(QFrame::Panel);
	mainFrame->setFrameShape(QFrame::Panel);
	mainInfo->setFrameShape(QFrame::Panel);
	footer->setFrameShape(QFrame::Panel);
#endif
	initStatisticData();
	// Верхние кнопочки
	connect(mainModeBtn, SIGNAL(released()), SLOT(activateMainPage()));
	connect(fightModeBtn, SIGNAL(released()), SLOT(activateFightPage()));
	connect(persInfoModeBtn, SIGNAL(released()), SLOT(activatePersInfoPage()));
	connect(thingsModeBtn, SIGNAL(released()), SLOT(activateThingsPage()));
	connect(statModeBtn, SIGNAL(released()), SLOT(activateStatPage()));
	connect(settingsModeBtn, SIGNAL(released()), SLOT(activateSettingsPage()));
	// Страницы плагина
	stackedWidget->setCurrentIndex(0);
	// Создаем действия для карты
	QAction* actionSaveMap = new QAction(gameMapView);
	//actionSaveMap->setIcon (QIcon(":/images/edit_copy.png"));
	actionSaveMap->setText(QString::fromUtf8("Сохранить"));
	//actionSaveMap->setShortcut(tr("Ctrl+C"));
	actionSaveMap->setStatusTip(QString::fromUtf8("Сохранить карты"));
	connect(actionSaveMap, SIGNAL(triggered()), this, SLOT(saveMap()));
	QAction* actionCreateMap = new QAction(gameMapView);
	actionCreateMap->setText(QString::fromUtf8("Создать карту"));
	actionCreateMap->setStatusTip(QString::fromUtf8("Создать новую карту"));
	connect(actionCreateMap, SIGNAL(triggered()), this, SLOT(createMap()));
	QAction* actionClearMap = new QAction(gameMapView);
	actionClearMap->setText(QString::fromUtf8("Очистить карту"));
	actionClearMap->setStatusTip(QString::fromUtf8("Очистить текущую карту"));
	connect(actionClearMap, SIGNAL(triggered()), this, SLOT(clearMap()));
	actionMarkMapElement = new QAction(gameMapView);
	actionMarkMapElement->setText(QString::fromUtf8("Отметка на карте"));
	actionMarkMapElement->setStatusTip(QString::fromUtf8("Поставить/убрать отметку на карте"));
	//actionMarkMapElement->setCheckable(true);
	connect(actionMarkMapElement, SIGNAL(triggered()), this, SLOT(markMapElement()));
	actionMoveMapElement = new QAction(gameMapView);
	actionMoveMapElement->setText(QString::fromUtf8("Перенести элемент"));
	actionMoveMapElement->setStatusTip(QString::fromUtf8("Перенести элемент на другую карту"));
	connect(actionMoveMapElement, SIGNAL(triggered()), this, SLOT(moveMapElement()));
	actionRemoveMapElement = new QAction(gameMapView);
	actionRemoveMapElement->setText(QString::fromUtf8("Удалить элемент"));
	actionRemoveMapElement->setStatusTip(QString::fromUtf8("Удалить элемент с карты"));
	connect(actionRemoveMapElement, SIGNAL(triggered()), this, SLOT(removeMapElement()));
	// Создаем контекстное меню
	mapMenu = new QMenu(this);
	mapMenu->addAction(actionSaveMap);
	mapMenu->addAction(actionCreateMap);
	mapMenu->addAction(actionClearMap);
	mapMenu->addAction(actionMarkMapElement);
	mapMenu->addAction(actionMoveMapElement);
	mapMenu->addAction(actionRemoveMapElement);
	mapMenu->insertSeparator(actionCreateMap);
	mapMenu->insertSeparator(actionMarkMapElement);
	// Привязываем сигнал
	connect(gameMapView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(mapShowContextMenu(const QPoint &)));
	gameMapView->setContextMenuPolicy(Qt::CustomContextMenu);
	// Строка команд
	connect(userCommandLine, SIGNAL(returnPressed()), SLOT(userCommandReturnPressed()));
	connect(userCommandLine, SIGNAL(textChanged()), SLOT(userCommandChanged()));
	// Кнопка ввода команд
	cmd_send->setIcon(PluginHosts::psiIcon->getIcon("psi/action_button_send"));
	connect(cmd_send, SIGNAL(clicked()), SLOT(userCommandReturnPressed()));
	// Таббар вещей (фильтры)
	thingsTabBar = new QTabBar(page_4);
	connect(thingsTabBar, SIGNAL(currentChanged(int)), this, SLOT(showThings(int)));
	// Таблица вещей
	QLayout* lt = page_4->layout();
	lt->removeWidget(thingsTable);
	lt->removeItem(thingsSummaryLayout);
	lt->removeItem(moneyCountLayout);
	lt->addWidget(thingsTabBar);
	lt->addWidget(thingsTable);
	lt->addItem(thingsSummaryLayout);
	lt->addItem(moneyCountLayout);
	connect(thingsTable, SIGNAL(changeSummary()), this, SLOT(showThingsSummary()));
	connect(thingsTable, SIGNAL(writeToConsole(QString,int,bool)), this, SLOT(setConsoleText(QString,int,bool)));
	// Табвиджет настроек
	settingTab->setCurrentIndex(0);
	// Кнопки шрифтов
	fontLabelGroup.push_back(persNameFont_label);
	fontLabelGroup.push_back(gameTextFont_label);
	fontButtonGroup = new QButtonGroup(this);
	fontButtonGroup->addButton(persNameFont_button);
	fontButtonGroup->addButton(gameTextFont_button);
	connect(fontButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(chooseFont(QAbstractButton*)));
	// Виджеты страницы статистики
	statisticWidgets[Statistic::StatLastGameJid] = (StatWidgets) {lastGameJidCaption_label, lastGameJidValue_label};
	statisticWidgets[Statistic::StatLastChatJid] = (StatWidgets) {lastGameChatCaption_label, lastGameChatValue_label};
	statisticWidgets[Statistic::StatMessagesCount] = (StatWidgets) {gameMessagesCountCaption_label, gameMessagesCountValue_label};
	connect(resetCommonStatBtn, SIGNAL(released()), SLOT(resetCommonStatistic()));
	statisticWidgets[Statistic::StatFightsCount] = (StatWidgets) {fightsCountCaption_label, fightsCountValue_label};
	statisticWidgets[Statistic::StatDamageMaxFromPers] = (StatWidgets) {damageMaxFromPersCaption_label, damageMaxFromPersValue_label};
	statisticWidgets[Statistic::StatDamageMinFromPers] = (StatWidgets) {damageMinFromPersCaption_label, damageMinFromPersValue_label};
	statisticWidgets[Statistic::StatDropMoneys] = (StatWidgets) {dropMoneysCaption_label, dropMoneysValue_label};
	statisticWidgets[Statistic::StatThingsDropCount] = (StatWidgets) {thingsDropCountCaption_label, thingsDropCountValue_label};
	statisticWidgets[Statistic::StatThingDropLast] = (StatWidgets) {thingDropLastCaption_label, thingDropLastValue_label};
	statisticWidgets[Statistic::StatExperienceDropCount] = (StatWidgets) {experienceDropCountCaption_label, experienceDropCountValue_label};
	statisticWidgets[Statistic::StatKilledEnemies] = (StatWidgets) {killedEnemiesCaption_label, killedEnemiesValue_label};
	connect(resetFightsStatBtn, SIGNAL(released()), SLOT(resetFightStatistic()));
	// Таблицы для настройки фильтров
	thingFiltersTable->init(&filtersList);
	thingRulesTable->init(&filtersList);
	connect(thingFiltersTable, SIGNAL(currFilterChanged(int)), thingRulesTable, SLOT(currFilterChanged(int)));
	// Настройки слотов
	fillSlotCombo(slot1Combo);
	fillSlotCombo(slot2Combo);
	fillSlotCombo(slot3Combo);
	fillSlotCombo(slot4Combo);
	fillSlotCombo(slot5Combo);
	fillSlotCombo(slot6Combo);
	fillSlotCombo(slot7Combo);
	fillSlotCombo(slot8Combo);
	fillSlotCombo(slot9Combo);
	// Кнопки сохранения и применения настроек
	connect(settingSaveBtn, SIGNAL(released()), SLOT(saveSettings()));
	connect(settingApplyBtn, SIGNAL(released()), SLOT(applySettings()));
	// Нижние слоты статистики
	footerStatWidgets[1] = (StatWidgets) {slot1Caption_label, slot1Value_label};
	footerStatWidgets[2] = (StatWidgets) {slot2Caption_label, slot2Value_label};
	footerStatWidgets[3] = (StatWidgets) {slot3Caption_label, slot3Value_label};
	footerStatWidgets[4] = (StatWidgets) {slot4Caption_label, slot4Value_label};
	footerStatWidgets[5] = (StatWidgets) {slot5Caption_label, slot5Value_label};
	footerStatWidgets[6] = (StatWidgets) {slot6Caption_label, slot6Value_label};
	footerStatWidgets[7] = (StatWidgets) {slot7Caption_label, slot7Value_label};
	footerStatWidgets[8] = (StatWidgets) {slot8Caption_label, slot8Value_label};
	footerStatWidgets[9] = (StatWidgets) {slot9Caption_label, slot9Value_label};
	// Окно статистики
	setStatisticCaptionText();
	// Виджеты параметров персонажа
	experienceBar->setValue(0);
	experienceBar->setRange(0, 0);
	healthBar->setValue(0);
	healthBar->setRange(0, 0);
	energyBar->setValue(0);
	energyBar->setRange(0, 0);
	// Сглаживание карты
	//gameMapView->setRenderHint(QPainter::Antialiasing);
	// Соединение с картой
	gameMapView->setScene(GameMap::instance()->getGraphicsScene());
	gameMapView->show();
	// Сигнал на смену страницы в стеке
	connect(stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(changePage(int)));
	// Соединение с параметрами персонажа
	Pers *pers = Pers::instance();
	connect(pers, SIGNAL(thingsChanged()), this, SLOT(persThingsChanged()));
	connect(pers, SIGNAL(persParamChanged(int, int, int)), this, SLOT(persParamChanged(int, int, int)));
	// Сигнал на смену фильтров
	connect(pers, SIGNAL(filtersChanged()), this, SLOT(updateThingFiltersTab()));
	// Соединение на обновление статистики
	connect(Statistic::instance(), SIGNAL(valueChanged(int)), this, SLOT(updateStatistic(int)));
	// Завязки в настройках
	connect(restPopup, SIGNAL(toggled(bool)), restDurationPopup, SLOT(setEnabled(bool)));
	// Инициируем форму
	init();
	// Устанавливаем фокус ввода
	userCommandLine->setFocus();
}

SofMainWindow::~SofMainWindow()
{
	if (timeoutStamp)
		delete timeoutStamp;
	if (timeoutTimer)
		delete timeoutTimer;
	while (!filtersList.isEmpty()) {
		delete filtersList.takeFirst();
	}
}

void SofMainWindow::init()
{
	// Инициализация переменных
	if (timeoutTimer)
		delete timeoutTimer;
	timeoutTimer = 0;
	if (timeoutStamp)
		delete timeoutStamp;
	timeoutStamp = new QDateTime();
	currentPage = 0;
	settingTimeOutDisplay = 0;
	selectedMapElement = -1;
	thingsChanged = false;
	timeoutEventSlot = 0;
	queueEventSlot = 0;
	queueShowFlag = false;
	experienceMax = -1;
	experienceCurr = -1;
	settingWindowSizePos = 1;
	// Настройки шрифтов
	QFont f = persNameLabel->font();
	persNameFont_label->setFont(f.toString());
	f = serverTextView->font();
	gameTextFont_label->setFont(f.toString());
	// Получаем основные настройки окна
	getAllDataFromCore();
	// Размеры сплиттера
	QList<int> sizes;
	sizes.push_back(100);
	sizes.push_back(100);
	mapSplitter->setSizes(sizes);
	// Загружаем настройки слотов
	Settings *settings = Settings::instance();
	loadSlotsSettings(settings->getSlotsData());
	// Таблица вещей
	thingsTable->init();
	// Загружаем настройки внешнего вида
	loadAppearanceSettings(settings->getAppearanceData());
	// Убираем табы фильтров вещей
	updateThingFiltersTab();
	// Сбрасываем цвет текста кнопки вещей
	thingsModeBtn->setStyleSheet(QString());
	// Сбрасываем режим автоввода
	setAutoEnterMode(false);
	// Инициируем слоты событий и уведомлений
	initEventSlots();
	// Фрейм аватара
	if (!avatarFrame->updateAvatar())
		avatarFrame->showPluginInfo();
	// Особые враги
	specificEnemiesTable->init();
}

void SofMainWindow::setAutoEnterMode(bool mode)
{
	if (autoEnterMode != mode) {
		autoEnterMode = mode;
		QPalette pal;
		QBrush brush1;
		QBrush brush2;
		if (mode) {
			cmd_send->setStyleSheet("background-color: rgba(255,0,0,64);");
			brush1.setColor(QColor(255, 0, 0, 64)); // Красный
			brush2.setColor(QColor(255, 0, 0, 64)); // Красный
		} else {
			cmd_send->setStyleSheet(QString());
			brush1.setColor(QApplication::palette().color(QPalette::Active, QPalette::Base));
			brush2.setColor(QApplication::palette().color(QPalette::Inactive, QPalette::Base));
		}
		brush1.setStyle(Qt::SolidPattern);
		brush2.setStyle(Qt::SolidPattern);
		pal.setBrush(QPalette::Active, QPalette::Base, brush1);
		pal.setBrush(QPalette::Inactive, QPalette::Base, brush2);
		userCommandLine->setPalette(pal);
	}
}

/**
 * Реакция на закрытие окна
 */
void SofMainWindow::closeEvent(QCloseEvent *evnt)
{
	PluginCore::instance()->sendCommandToCore(COMMAND_CLOSE_WINDOW);
	evnt->accept();
}

void SofMainWindow::fillSlotCombo(QComboBox* slotCombo)
{
	slotCombo->clear();
	slotCombo->addItem(QString::fromUtf8("* Пустой *"), -1);
	foreach (int key, statisticCapInitVal.keys()) {
		slotCombo->addItem(statisticCapInitVal.value(key), key);
	}
}

void SofMainWindow::initStatisticData()
{
	// Заполняем заголовки наименований элементов статистики и начальные значения
	statisticCapInitVal[Statistic::StatLastGameJid] = QString::fromUtf8("JID игры");
	statisticCapInitVal[Statistic::StatLastChatJid] = QString::fromUtf8("JID чата");
	statisticCapInitVal[Statistic::StatMessagesCount] = QString::fromUtf8("Сообщений");
	statisticCapInitVal[Statistic::StatDamageMaxFromPers] = QString::fromUtf8("Лучший удар");
	statisticCapInitVal[Statistic::StatDamageMinFromPers] = QString::fromUtf8("Худший удар");
	statisticCapInitVal[Statistic::StatFightsCount] = QString::fromUtf8("Всего боев");
	statisticCapInitVal[Statistic::StatDropMoneys] = QString::fromUtf8("Денег собрано");
	statisticCapInitVal[Statistic::StatThingsDropCount] = QString::fromUtf8("Вещей собрано");
	statisticCapInitVal[Statistic::StatThingDropLast] = QString::fromUtf8("Последняя вещь");
	statisticCapInitVal[Statistic::StatExperienceDropCount] = QString::fromUtf8("Полученный опыт");
	statisticCapInitVal[Statistic::StatKilledEnemies] = QString::fromUtf8("Противников повержено");
}

void SofMainWindow::setStatisticCaptionText()
{
	// Заполняем текст статистики в окне статистика
	foreach (int statKey, statisticWidgets.keys()) {
		QString capt = statisticCapInitVal.value(statKey, "?");
		statisticWidgets.value(statKey).caption->setText(capt + ":");
	}
}

/**
 * Заполняет QLabel-ы слотов статистики согласно настроек
 */
void SofMainWindow::fullUpdateFooterStatistic()
{
	QList<int> slotKeys = footerStatWidgets.keys();
	foreach (int slot, slotKeys) {
		QString captText;
		QString valText;
		int statVal = statisticFooterPos.key(slot, -1);
		if (statVal != -1) {
			captText = statisticCapInitVal.value(statVal, "?");
			captText.append(":");
			if (statisticWidgets.contains(statVal)) {
				valText = statisticWidgets.value(statVal).value->text();
			} else {
				valText = "?";
			}
		} else {
			captText = "-";
			valText = "-";
		}
		footerStatWidgets.value(slot).caption->setText(captText);
		footerStatWidgets.value(slot).value->setText(valText);
	}
	// Скрываем неиспользуемые виджеты слотов
	if (statisticFooterPos.key(1, -1) == -1 &&
	    statisticFooterPos.key(2, -1) == -1 &&
	    statisticFooterPos.key(3, -1) == -1) {
		slotsWidget1->setHidden(true);
	} else {
		slotsWidget1->setHidden(false);
	}
	if (statisticFooterPos.key(4, -1) == -1 &&
	    statisticFooterPos.key(5, -1) == -1 &&
	    statisticFooterPos.key(6, -1) == -1) {
		slotsWidget2->setHidden(true);
	} else {
		slotsWidget2->setHidden(false);
	}
	if (statisticFooterPos.key(7, -1) == -1 &&
	    statisticFooterPos.key(8, -1) == -1 &&
	    statisticFooterPos.key(9, -1) == -1) {
		slotsWidget3->setHidden(true);
	} else {
		slotsWidget3->setHidden(false);
	}
}

/**
 * Отображает значение статистики в слотах подвала, если она там настроена
 */
void SofMainWindow::setFooterStatisticValue(int statisticId, const QString &valString)
{
	if (statisticFooterPos.contains(statisticId)) {
		int slotId = statisticFooterPos.value(statisticId);
		if (footerStatWidgets.contains(slotId)) {
			footerStatWidgets.value(slotId).value->setText(valString);
		}
	}
}

/**
 * Обновляет значения статистики в окне статистики
 */
void SofMainWindow::setStatisticValue(int statisticId, const QString &valString)
{
	if (statisticWidgets.contains(statisticId)) {
		statisticWidgets.value(statisticId).value->setText(valString);
	}
}

void SofMainWindow::getAllDataFromCore() {
	// Фунцкия опрашивает ядро для получения данных
	int newIntValue;
	QString newStrValue;
	// *** Настройки плагина ***
	Settings *settings = Settings::instance();
	GameMap *maps = GameMap::instance();
	// Имя персонажа
	newStrValue = settings->getStringSetting(Settings::SettingPersName);
	if (newStrValue.isEmpty())
		newStrValue = "n/a";
	setPersName->setText(newStrValue);
	// Режим переключения зеркал
	mirrorChangeModeCombo->setCurrentIndex(Sender::instance()->getGameMirrorsMode());
	// Отслеживание восстановления здоровья и энергии
	restHealthEnergyCombo->setCurrentIndex(settings->getIntSetting(Settings::SettingWatchRestHealthEnergy));
	// Таймер в бою
	settingTimeOutDisplay = settings->getIntSetting(Settings::SettingFightTimerMode);
	fightTimerCombo->setCurrentIndex(settingTimeOutDisplay);
	// Выбор боя
	fightSelectAction->setCurrentIndex(settings->getIntSetting(Settings::SettingFightSelectAction));
	// Автозакрытие боя
	setAutoCloseFight->setCurrentIndex(settings->getIntSetting(Settings::SettingFightAutoClose));
	// Попап при дропе вещей
	checkbox_ThingDropPopup->setChecked(settings->getBoolSetting(Settings::SettingThingDropPopup));
	// Попап при заказе в клубе убийц
	checkbox_InKillersCupPopup->setChecked(settings->getBoolSetting(Settings::SettingInKillersCupPopup));
	// Попап при нападении убийцы
	checkbox_KillerClubAttack->setChecked(settings->getBoolSetting(Settings::SettingKillerAttackPopup));
	// Отображение длины очереди
	bool flag = settings->getBoolSetting(Settings::SettingShowQueryLength);
	if (flag) {
		if (!queueShowFlag) {
			connect(Sender::instance(), SIGNAL(queueSizeChanged(int)), this, SLOT(showQueueLen(int)));
			queueShowFlag = true;
		}
	} else {
		if (queueShowFlag) {
			disconnect(Sender::instance(), SIGNAL(queueSizeChanged(int)), this, SLOT(showQueueLen(int)));
			queueShowFlag = false;
			showQueueLen(0);
		}
	}
	checkbox_ShowQueueLength->setChecked(flag);
	// Сброс очереди при неизвестном статусе
	checkbox_ResetQueueForUnknowStatus->setChecked(settings->getBoolSetting(Settings::SettingResetQueueForUnknowStatus));
	// Попап при сбросе очереди
	checkbox_ResetQueuePopup->setChecked(settings->getBoolSetting(Settings::SettingResetQueuePopup));
	// Сохранение основных параметров персонажа
	persParamSaveMode_combo->setCurrentIndex(settings->getIntSetting(Settings::SettingPersSaveMode));
	// Сохранение основных параметров персонажа
	saveMainPersParam_checkbox->setChecked(settings->getBoolSetting(Settings::SettingSavePersParams));
	// Сохранение рюкзака персонажа
	savePersBackpack_checkbox->setChecked(settings->getBoolSetting(Settings::SettingSaveBackpack));
	// Сохранение статистики
	saveStatistic_checkbox->setChecked(settings->getBoolSetting(Settings::SettingSaveStatistic));
	// Цвет позиции персонажа на карте
	btnPersPosColor->setColor(maps->getPersPosColor());
	// Автовыгрузка карт
	mapsUnloadInterval->setValue(maps->getUnloadInterval());
	// Позиция персонажа в центре карты
	checkbox_PersPosInCenter->setChecked(settings->getBoolSetting(Settings::SettingPersPosInCenter));
	// *** Основные данные ***
	Pers *pers = Pers::instance();
	changePersStatus();
	newStrValue = pers->name();
	if (newStrValue.isEmpty())
		newStrValue = NA_TEXT;
	persNameLabel->setText(newStrValue);
	if (!pers->getIntParamValue(Pers::ParamPersLevel, &newIntValue)) {
		newStrValue = NA_TEXT;
	} else {
		newStrValue = QString::number(newIntValue);
	}
	levelLabel->setText(newStrValue);
	long long newLongValue;
	if (!pers->getLongParamValue(Pers::ParamExperienceCurr, &newLongValue)) {
		newLongValue = 0;
	}
	setCurrentExperience(newLongValue);
	if (!pers->getLongParamValue(Pers::ParamExperienceMax, &newLongValue)) {
		newLongValue = 0;
	}
	setMaximumExperience(newLongValue);
	if (!pers->getIntParamValue(Pers::ParamHealthCurr, &newIntValue)) {
		newIntValue = 0;
	}
	setCurrentHealth(newIntValue);
	if (!pers->getIntParamValue(Pers::ParamHealthMax, &newIntValue)) {
		newIntValue = 0;
	}
	healthBar->setRange(0, newIntValue);
	if (!pers->getIntParamValue(Pers::ParamEnergyCurr, &newIntValue)) {
		newIntValue = 0;
	}
	setCurrentEnergy(newIntValue);
	if (!pers->getIntParamValue(Pers::ParamEnergyMax, &newIntValue)) {
		newIntValue = 0;
	}
	energyBar->setRange(0, newIntValue);
	newIntValue = pers->moneysCount();
	if (newIntValue == QINT32_MIN) {
		labelMoneysCount->setText(NA_TEXT);
	} else {
		labelMoneysCount->setText(numToStr(newIntValue, "'"));
	}
	// Прокрутка карты к позиции персонажа
	scrollMapToPersPosition();
	// *** Статистические данные ***
	updateStatistic(Statistic::StatMessagesCount);
	updateStatistic(Statistic::StatLastGameJid);
	updateStatistic(Statistic::StatLastChatJid);
	updateStatistic(Statistic::StatDamageMinFromPers);
	updateStatistic(Statistic::StatDamageMaxFromPers);
	updateStatistic(Statistic::StatFightsCount);
	updateStatistic(Statistic::StatDropMoneys);
	updateStatistic(Statistic::StatThingsDropCount);
	updateStatistic(Statistic::StatThingDropLast);
	updateStatistic(Statistic::StatExperienceDropCount);
	updateStatistic(Statistic::StatKilledEnemies);
	//--
	newIntValue = settings->getIntSetting(Settings::SettingServerTextBlocksCount);
	if (newIntValue < 0) {
		newIntValue = 0;
	} else if (newIntValue > 0 && newIntValue < 100) {
		newIntValue = 100;
	}
	maxTextBlocksCount->setValue(newIntValue);
	serverTextView->setMaximumBlockCount(newIntValue);
	consoleTextView->setMaximumBlockCount(newIntValue);
	// Расцветка текста игры
	gameTextColoring->setChecked(settings->getBoolSetting(Settings::SettingGameTextColoring));
	// Режим сохранения карт
	mapsParamSaveMode->setCurrentIndex(GameMap::instance()->getMapsSettingParam(GameMap::AutoSaveMode));
	// Длительность регена для отображения Popup-а
	newIntValue = settings->getIntSetting(Settings::SettingRegenDurationForPopup);
	if (newIntValue > 0) {
		if (newIntValue < restDurationPopup->minimum())
			newIntValue = restDurationPopup->minimum();
		restDurationPopup->setValue(newIntValue);
		restDurationPopup->setEnabled(true);
		restPopup->setChecked(true);
	} else {
		restDurationPopup->setValue(restDurationPopup->minimum());
		restDurationPopup->setEnabled(false);
		restPopup->setChecked(false);
	}
}

/**
 * Обновляет значения статистики в виджетах, в том числе и в footer-е.
 */
void SofMainWindow::updateValue(int valueId, const QString &valString) {

	// Обновляем основной виджет статистики
	setStatisticValue(valueId, valString.left(20));
	// Обновляем footer
	setFooterStatisticValue(valueId, valString.left(20));
}

void SofMainWindow::valueChanged(int eventId, int valueType, int value)
{
	// Функция обработки события изменения данных.
	// Есть ли данные в value и какие они, зависит от valueType
	QString str1;
	if (valueType == TYPE_INTEGER_FULL) {
		// В событии есть полные данные
		str1 = numToStr(value, "'");
		if (eventId == VALUE_TIMEOUT) {
			// Пришло событие таймаута
			if (settingTimeOutDisplay != 0) {
				setTimeout(value);
			}
		}
	} else if (valueType == TYPE_NA) {
		// Данные недоступны или неопределены (n/a)
		str1 = NA_TEXT;
		if (eventId == Pers::ParamHealthCurr) {
			// Текущее здоровье
			healthLabel->setText(str1);
			healthBar->setValue(0);
		} else if (eventId == Pers::ParamHealthMax) {
			// Максимальное здоровье
			healthBar->setRange(0, 0);
		} else if (eventId == Pers::ParamEnergyCurr) {
			// Текущая энергия
			energyLabel->setText(str1);
			energyBar->setValue(0);
		} else if (eventId == Pers::ParamEnergyMax) {
			// Максимальная энергия
			energyBar->setRange(0, 0);
		}
	}
}

void SofMainWindow::changePersStatus()
{
	persStatusLabel->setText(Pers::instance()->getPersStatusString());
}

void SofMainWindow::setGameText(const QString &gameText, int type)
{
	/**
	* gameText - строка для отображения, если gameText == 0, то очищаем
	* type - 1: исходящее, 2: входящее, 3 - информационное
	**/
	if (gameText.isEmpty()) {
		serverTextView->clear();
	}
	if (!gameText.isEmpty()) {
		TextView::TextType type_ = (type == 1) ? TextView::LocalText : (type == 2) ? TextView::GameText : TextView::PluginText;
		serverTextView->appendText("<span>" + gameText + "</span>", type_);
	} else {
		serverTextView->appendText(QString::fromUtf8("<span>--- <b>очищено</b> ---</span>"), TextView::PluginText);
	}
	if (type == 1) {
		text_tabWidget->setCurrentIndex(0);
	}
}

/**
 * text - строка для отображения, если text == "", то очищаем
 * type - 1: исходящее, 2: входящее, 3 - информационное
 * switch_ - переключать или нет на консоль
 */
void SofMainWindow::setConsoleText(const QString &text, int type, bool switch_)
{
	if (text.isEmpty()) {
		consoleTextView->clear();
	}
	if (!text.isEmpty()) {
		TextView::TextType type_ = (type == 1) ? TextView::LocalText : (type == 2) ? TextView::GameText : TextView::PluginText;
		consoleTextView->appendText("<span>" + text + "</span>", type_);
	} else {
		consoleTextView->appendText(QString::fromUtf8("<span>--- <b>очищено</b> ---</span>"), TextView::PluginText);
	}
	consoleTextView->verticalScrollBar()->setValue(consoleTextView->verticalScrollBar()->maximum());
	if (switch_) {
		text_tabWidget->setCurrentIndex(1);
	}
}

/**
 * Возвращает DOM элемент, содержащий настройки внешнего вида
 */
QDomElement SofMainWindow::exportAppearanceSettings(QDomDocument &xmlDoc) const
{
	QDomElement eAppearance = xmlDoc.createElement("appearance");
	QDomElement ePersNameAppe = xmlDoc.createElement("pers-name");
	eAppearance.appendChild(ePersNameAppe);
	QDomElement ePersNameFontAppe = xmlDoc.createElement("font");
	ePersNameAppe.appendChild(ePersNameFontAppe);
	ePersNameFontAppe.setAttribute("value", persNameLabel->font().toString());
	QDomElement eServerTextAppe = xmlDoc.createElement("server-text");
	eAppearance.appendChild(eServerTextAppe);
	QDomElement eServerTextFontAppe = xmlDoc.createElement("font");
	eServerTextAppe.appendChild(eServerTextFontAppe);
	eServerTextFontAppe.setAttribute("value", serverTextView->document()->defaultFont().toString());
	QDomElement eThingsTable = thingsTable->saveSettingsToXml(xmlDoc);
	eAppearance.appendChild(eThingsTable);
	if (settingWindowSizePos == 1) {
		QDomElement eWindowParams = xmlDoc.createElement("window-save-params");
		eWindowParams.setAttribute("mode", "position-and-size");
		eWindowParams.setAttribute("pos-x", x());
		eWindowParams.setAttribute("pos-y", y());
		eWindowParams.setAttribute("width", width());
		eWindowParams.setAttribute("height", height());
		eAppearance.appendChild(eWindowParams);
	}

	return eAppearance;
}

/**
 * Возвращает DOM элемент, содержащий настройки слотов
 */
QDomElement SofMainWindow::exportSlotsSettings(QDomDocument &xmlDoc) const
{
	QDomElement eSlots = xmlDoc.createElement("slots");
	for (int i = 0, cnt = SofMainWindow::statisticXmlStrings.size(); i < cnt; i++) {
		const int nStat = SofMainWindow::statisticXmlStrings.at(i).first;
		if (statisticFooterPos.contains(nStat)) {
			const int nSlot = statisticFooterPos.value(nStat);
			QDomElement eSlot = xmlDoc.createElement("slot");
			eSlot.setAttribute("num", nSlot);
			eSlot.setAttribute("param", SofMainWindow::statisticXmlStrings.at(i).second);
			eSlots.appendChild(eSlot);
		}
	}
	return eSlots;
}

/**
 * Применяет настройки внешнего вида
 */
void SofMainWindow::loadAppearanceSettings(const QDomElement &xml)
{
	QDomElement ePersName = xml.firstChildElement("pers-name");
	if (!ePersName.isNull()) {
		QDomElement ePersNameFont = ePersName.firstChildElement("font");
		if (!ePersNameFont.isNull()) {
			QString fontStr = ePersNameFont.attribute("value");
			if (!fontStr.isEmpty()) {
				QFont f;
				if (f.fromString(fontStr)) {
					persNameFont_label->setFont(f.toString());
					persNameLabel->setFont(f);
				}
			}
		}
	}
	QDomElement eServerText = xml.firstChildElement("server-text");
	if (!eServerText.isNull()) {
		QDomElement eServerTextFont = eServerText.firstChildElement("font");
		if (!eServerTextFont.isNull()) {
			QString fontStr = eServerTextFont.attribute("value");
			if (!fontStr.isEmpty()) {
				QFont f;
				if (f.fromString(fontStr)) {
					gameTextFont_label->setFont(f.toString());
					serverTextView->setFont(f);
					consoleTextView->setFont(f);
				}
			}
		}
	}
	QDomElement eThingsSettings = xml.firstChildElement("things-table");
	thingsTable->loadSettingsFromXml(eThingsSettings);
	QDomElement eGeometry = xml.firstChildElement("window-save-params");
	if (!eGeometry.isNull()) {
		if (eGeometry.attribute("mode") == "position-and-size") {
			settingWindowSizePos = 1;
			int posX = eGeometry.attribute("pos-x").toInt();
				int posY = eGeometry.attribute("pos-y").toInt();
			int width_ = eGeometry.attribute("width").toInt();
			int height_ = eGeometry.attribute("height").toInt();
			// Определяем геометрию рабочих столов (доступное пространство)
			QRect scRect = QApplication::desktop()->availableGeometry(-1); //-- default system screen
			// Корректируем наши параметры, если превышают допустимые
			if (scRect.x() > posX)
				posX = scRect.x();
			if (scRect.y() > posY)
				posY = scRect.y();
			if (posX - scRect.x() + width_ > scRect.width()) {
				// * Не вписываемся по горизонтали
				// Исправляем за счет смещения окна
				posX -= (posX + width_) - (scRect.x() + scRect.width());
				if (posX < scRect.x()) {
					posX = scRect.x();
					// Исправляем за счет ширины окна
					width_ = scRect.width() - posX + scRect.x();
				}
			}
			if (posY - scRect.y() + height_ > scRect.height()) {
				// * Не вписываемся по вертикали
				// Исправляем за счет смещения окна
				posY -= (posY + height_) - (scRect.y() + scRect.height());
				if (posY < scRect.y()) {
					posY = scRect.y();
					// Исправляем за счет высоты окна
					height_ = scRect.height() - posY + scRect.y();
				}
			}
			// Перемещаем окно
			move(posX, posY);
			// Изменяем размер окна
			resize(width_, height_);
		}
	}
	windowSizePosCombo->setCurrentIndex(settingWindowSizePos);
}

/**
 * Загружает и применяет настройки слотов статистики
 */
void SofMainWindow::loadSlotsSettings(const QDomElement &xml)
{
	statisticFooterPos.clear();
	if (!xml.isNull()) {
		QDomElement eSlot = xml.firstChildElement("slot");
		while (!eSlot.isNull()) {
			QString param = eSlot.attribute("param").toLower();
			if (!param.isEmpty()) {
				int slotNum = eSlot.attribute("num").toInt();
				if (slotNum >= 1 && slotNum <= SLOTS_COUNT) {
					for (int i = 0, cnt = SofMainWindow::statisticXmlStrings.size(); i < cnt; i++) {
						if (SofMainWindow::statisticXmlStrings.at(i).second == param) {
							statisticFooterPos[SofMainWindow::statisticXmlStrings.at(i).first] = slotNum;
							break;
						}
					}
				}
			}
			eSlot = eSlot.nextSiblingElement("slot");
		}
	} else {
		// Значения по умолчанию
		statisticFooterPos[Statistic::StatLastGameJid] = 1;
		statisticFooterPos[Statistic::StatFightsCount] = 2;
		statisticFooterPos[Statistic::StatKilledEnemies] = 3;
		statisticFooterPos[Statistic::StatExperienceDropCount] = 4;
		statisticFooterPos[Statistic::StatDamageMaxFromPers] = 5;
		statisticFooterPos[Statistic::StatDamageMinFromPers] = 6;
		statisticFooterPos[Statistic::StatDropMoneys] = 7;
		statisticFooterPos[Statistic::StatThingsDropCount] = 8;
		statisticFooterPos[Statistic::StatThingDropLast] = 9;
	}
	fullUpdateFooterStatistic();
	// Меняем элементы настроек слотов
	slot1Combo->setCurrentIndex(slot1Combo->findData(statisticFooterPos.key(1, -1)));
	slot2Combo->setCurrentIndex(slot2Combo->findData(statisticFooterPos.key(2, -1)));
	slot3Combo->setCurrentIndex(slot3Combo->findData(statisticFooterPos.key(3, -1)));
	slot4Combo->setCurrentIndex(slot4Combo->findData(statisticFooterPos.key(4, -1)));
	slot5Combo->setCurrentIndex(slot5Combo->findData(statisticFooterPos.key(5, -1)));
	slot6Combo->setCurrentIndex(slot6Combo->findData(statisticFooterPos.key(6, -1)));
	slot7Combo->setCurrentIndex(slot7Combo->findData(statisticFooterPos.key(7, -1)));
	slot8Combo->setCurrentIndex(slot8Combo->findData(statisticFooterPos.key(8, -1)));
	slot9Combo->setCurrentIndex(slot9Combo->findData(statisticFooterPos.key(9, -1)));
}

/**
 * Инициирует слоты событий
 */
void SofMainWindow::initEventSlots()
{
	maxEventSlotId = 0;
	usedEventSlots = 0;
	eventSlots.clear();
	eventSlots.push_back(0);
	eventSlots.push_back(0);
	eventSlots.push_back(0);
	event1Caption->setText("");
	event1Value->setText("");
	event2Caption->setText("");
	event2Value->setText("");
	event3Caption->setText("");
	event3Value->setText("");
}

int SofMainWindow::getEventSlot()
{
	/**
	* Выделяет слот для размещения событий.
	* Если слот успешно выделен, то возвращаемое значение - id слота. Иначе 0;
	**/
	int slotsCnt = eventSlots.size();
	if (usedEventSlots < slotsCnt) {
		for (int i = 0; i < slotsCnt; i++) {
			if (eventSlots[i] == 0) {
				maxEventSlotId++;
				usedEventSlots++;
				eventSlots[i] = maxEventSlotId;
				return maxEventSlotId;
			}
		}
	}
	return 0;
}

bool SofMainWindow::freeEventSlot(int id)
{
	/**
	* Освобождает занятый слот из списка занятых
	**/
	if (id <= 0)
		return false;
	int slotsCnt = eventSlots.size();
	for (int i = 0; i < slotsCnt; i++) {
		if (eventSlots[i] == id) {
			eventSlots[i] = 0;
			usedEventSlots--;
			if (usedEventSlots == 0)
				maxEventSlotId = 0;
			if (i == 0) {
				event1Caption->setText("");
				event1Value->setText("");
			} else if (i == 1) {
				event2Caption->setText("");
				event2Value->setText("");
			} else if (i == 2) {
				event3Caption->setText("");
				event3Value->setText("");
			}
			// Возможно нужно предусмотреть перемещение слотов, что бы убрать пустые места
			//--
			return true;
		}
	}
	return false;
}

bool SofMainWindow::setEventValue(int id, QString* caption, QString* value)
{
	/**
	* Устанавливает текст для слота события id
	* Если один из текстовых параметров ноль, то значение для него не устанавливается
	**/
	if (id <= 0)
		return false;
	// Ищем позицию слота для id-а
	int slotsCnt = eventSlots.size();
	for (int i = 0; i < slotsCnt; i++) {
		if (eventSlots[i] == id) {
			// Устанавливаем значения
			QLabel* captionLabelPtr = 0;
			QLabel* valueLabelPtr = 0;
			if (i == 0) {
				captionLabelPtr = event1Caption;
				valueLabelPtr = event1Value;
			} else if (i == 1) {
				captionLabelPtr = event2Caption;
				valueLabelPtr = event2Value;
			} else if (i == 2) {
				captionLabelPtr = event3Caption;
				valueLabelPtr = event3Value;
			}
			if (caption && captionLabelPtr) {
				captionLabelPtr->setText(*caption);
			}
			if (value && valueLabelPtr) {
				valueLabelPtr->setText(*value);
			}
			return true;
		}
	}
	return false;
}

void SofMainWindow::setTimeout(int value)
{
	/**
	* Устанавливаем таймаут (value in sec)
	* Если value = 0, то удаляем таймаут
	**/
	if (value > 0) {
		// Установка нового отсчета
		if (timeoutTimer && timeoutTimer->isActive()) {
			// Отсчет еще ведется
			timeoutTimer->stop();
		} else {
			// Получаем новый слот
			timeoutEventSlot = getEventSlot();
			if (timeoutEventSlot == -1) {
				return;
			}
		}
		// Подготавливаем данные
		*timeoutStamp = QDateTime::currentDateTime().addSecs(value);
		QString str1 = QString::fromUtf8("Отсчет:");
		QString str2 = QString::number(value);
		setEventValue(timeoutEventSlot, &str1, 0);
		// Первый раз вызываем принудительно
		timeoutEvent();
		// Запускаем таймер
		if (!timeoutTimer) {
			timeoutTimer = new QTimer(this);
			connect(timeoutTimer, SIGNAL(timeout()), SLOT(timeoutEvent()));
		}
		timeoutTimer->start(500);
	} else {
		// Удаление таймаута
		freeEventSlot(timeoutEventSlot);
		if (timeoutTimer)
			timeoutTimer->stop();
	}
}

/**
 * Прокручивает карту так, что бы была видна часть карты с указанными координатами
 */
void SofMainWindow::scrollMapNewPosition(const MapPos &pos)
{
	QRectF coordinates = GameMap::instance()->gameToSceneCoordinates(pos);
	if (!coordinates.isNull()) {
		if (Settings::instance()->getBoolSetting(Settings::SettingPersPosInCenter)) { // TODO Откэшировать!
			gameMapView->centerOn(coordinates.center());
		} else {
			gameMapView->ensureVisible(coordinates, 50, 50);
		}
	}
}

/**
 * Прокручивает карту так, что бы была видна позиция персонажа
 */
void SofMainWindow::scrollMapToPersPosition()
{
	const MapPos &pos = Pers::instance()->getMapPosition();
	if (pos.isValid()) {
		scrollMapNewPosition(pos);
	}
}

void SofMainWindow::setCurrentHealth(int health)
{
	/**
	* Прописывает текущее здоровье в виджеты
	**/
	int maxValue;
	if (!Pers::instance()->getIntParamValue(Pers::ParamHealthMax, &maxValue)) {
		maxValue = 0;
	}
	QPalette palette;
	QBrush brush;
	if (health >= maxValue) {
		brush.setColor(QColor(0, 0, 100, 255));
	} else {
		brush.setColor(QColor(128, 0, 0, 255));
	}
	brush.setStyle(Qt::SolidPattern);
	palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
	palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
	healthLabel->setPalette(palette);
	QString str1 = numToStr(health, "'");
/*	if (health >= 0 && maxValue > 0) {
		str1.append(" (" + QString::number((100*health/maxValue)) + "%)");
	}*/
	healthLabel->setText(str1);
	int he = (health > maxValue) ? maxValue : health;
	if (he < 0)
		he = 0;
	healthBar->setValue(he);
}

void SofMainWindow::setCurrentEnergy(int energy)
{
	/**
	* Прописывает текущую энергию в виджеты
	**/
	int maxValue;
	if (!Pers::instance()->getIntParamValue(Pers::ParamEnergyMax, &maxValue)) {
		maxValue = 0;
	}
	QPalette palette;
	QBrush brush;
	if (energy >= maxValue) {
		brush.setColor(QColor(0, 0, 100, 255));
	} else {
		brush.setColor(QColor(128, 0, 0, 255));
	}
	brush.setStyle(Qt::SolidPattern);
	palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
	palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
	energyLabel->setPalette(palette);
	QString str1 = numToStr(energy, "'");
/*	if (energy >= 0 && maxValue > 0) {
		str1.append(" (" + QString::number((100*energy/maxValue)) + "%)");
	}*/
	energyLabel->setText(str1);
	int en = (energy > maxValue) ? maxValue : energy;
	if (en < 0)
		en = 0;
	energyBar->setValue(en);
}

/**
 * Прописывает текущее значение опыта в виджеты окна
 */
void SofMainWindow::setCurrentExperience(long long exp)
{
	experienceCurr = exp;
	if (experienceMax != -1 ) {
		// Формируем делитель, чтоб вписаться в значение int
		long long div = experienceMax / 1000000000 + 1;
		if (div <= 0)
			div = 1;
		long long val = exp / div;
		if (exp % div >= 500000000) // Округляем
			++val;
		if (val < 0) {
			val = 0;
		} else if (val > experienceMax) {
			val = experienceMax;
		}
		experienceBar->setValue(val);
	}
	experienceLabel->setText(numToStr(exp, "'"));
}

void SofMainWindow::setMaximumExperience(long long mExp)
{
	experienceMax = mExp;
	if (mExp != -1) {
		long long div = mExp / 1000000000 + 1;
		if (div <= 0)
			div = 1;
		long long max = mExp / div;
		if (mExp % div >= 500000000) // Округляем
			++max;
		experienceBar->setValue(0);
		experienceBar->setRange(0, max);
		setCurrentExperience(experienceCurr);
	}
}

// ***************** Slots ***********************

void SofMainWindow::changePage(int index)
{
	/**
	* Произошла смена страницы у плагина
	**/
	currentPage = index;
	if (currentPage == 3) { // Вещи
		if (thingsChanged) { // Список вещей менялся
			// Сбрасываем цвет текста кнопки
			thingsModeBtn->setStyleSheet(QString());
		}
		// Инициируем табы и таблицу
		if (thingsTabBar->count() == 0) {
			updateThingFiltersTab();
			showThings(0);
		//} else {
		//	if (thingsChanged) {
		//		showThings(thingsTabBar->currentIndex());
		//	}
		}
		thingsChanged = false;
	}
}

void SofMainWindow::activateMainPage()
{
	if (currentPage == 0) {
		return;
	}
	stackedWidget->setCurrentIndex(0);
	currentPage = 0;
	userCommandLine->setFocus();
}

void SofMainWindow::activateFightPage()
{
	if (currentPage == 1) {
		return;
	}
	stackedWidget->setCurrentIndex(1);
	currentPage = 1;
}

void SofMainWindow::activatePersInfoPage()
{
	if (currentPage == 2) {
		return;
	}
	stackedWidget->setCurrentIndex(2);
	currentPage = 2;
}

void SofMainWindow::activateThingsPage()
{
	stackedWidget->setCurrentIndex(3);
}

void SofMainWindow::activateStatPage()
{
	if (currentPage == 4) {
		return;
	}
	stackedWidget->setCurrentIndex(4);
	currentPage = 4;
}

void SofMainWindow::activateSettingsPage()
{
	if (currentPage == 5) {
		return;
	}
	stackedWidget->setCurrentIndex(5);
	currentPage = 5;
}

void SofMainWindow::resetCommonStatistic()
{
	Statistic *stat = Statistic::instance();
	stat->setValue(::Statistic::StatLastGameJid, QVariant());
	stat->setValue(::Statistic::StatLastChatJid, QVariant());
	stat->setValue(::Statistic::StatMessagesCount, QVariant());
}

void SofMainWindow::resetFightStatistic()
{
	Statistic *stat = Statistic::instance();
	stat->setValue(::Statistic::StatFightsCount, QVariant());
	stat->setValue(::Statistic::StatDamageMaxFromPers, QVariant());
	stat->setValue(::Statistic::StatDamageMinFromPers, QVariant());
	stat->setValue(::Statistic::StatDropMoneys, QVariant());
	stat->setValue(::Statistic::StatThingsDropCount, QVariant());
	stat->setValue(::Statistic::StatThingDropLast, QVariant());
	stat->setValue(::Statistic::StatExperienceDropCount, QVariant());
	stat->setValue(::Statistic::StatKilledEnemies, QVariant());
}

void SofMainWindow::applySettings()
{
	// *** Применяем новые настройки ***
	Settings *settings = Settings::instance();
	Pers *pers = Pers::instance();
	// Имя персонажа
	if (!pers->name().isEmpty()) {
		settings->setStringSetting(Settings::SettingPersName, pers->name());
	}
	// Режим переключения зеркал
	Sender::instance()->setGameMirrorsMode(mirrorChangeModeCombo->currentIndex());
	// Сохранение параметров окна
	settingWindowSizePos = windowSizePosCombo->currentIndex();
	// Отслеживание восстановления здоровья и энергии
	int i = restHealthEnergyCombo->currentIndex();
	settings->setIntSetting(Settings::SettingWatchRestHealthEnergy, i);
	pers->setSetting(Settings::SettingWatchRestHealthEnergy, i);
	// Таймер в бою
	settingTimeOutDisplay = fightTimerCombo->currentIndex();
	settings->setIntSetting(Settings::SettingFightTimerMode, settingTimeOutDisplay);
	// Выбор боя
	settings->setIntSetting(Settings::SettingFightSelectAction, fightSelectAction->currentIndex());
	// Автозакрытие боя
	settings->setIntSetting(Settings::SettingFightAutoClose, setAutoCloseFight->currentIndex());
	// Попап при дропе вещей
	settings->setBoolSetting(Settings::SettingThingDropPopup, checkbox_ThingDropPopup->isChecked());
	// Попап при заказе в клубе убийц
	settings->setBoolSetting(Settings::SettingInKillersCupPopup, checkbox_InKillersCupPopup->isChecked());
	// Попап при нападении убийцы
	settings->setBoolSetting(Settings::SettingKillerAttackPopup, checkbox_KillerClubAttack->isChecked());
	// Отображение очереди команд
	bool flag = checkbox_ShowQueueLength->isChecked();
	if (flag) {
		if (!queueShowFlag) {
			connect(Sender::instance(), SIGNAL(queueSizeChanged(int)), this, SLOT(showQueueLen(int)));
			queueShowFlag = true;
		}
	} else {
		if (queueShowFlag) {
			disconnect(Sender::instance(), SIGNAL(queueSizeChanged(int)), this, SLOT(showQueueLen(int)));
			showQueueLen(0);
			queueShowFlag = false;
		}
	}
	settings->setBoolSetting(Settings::SettingShowQueryLength, flag);
	// Сброс очереди при неизвестном статусе
	settings->setBoolSetting(Settings::SettingResetQueueForUnknowStatus, checkbox_ResetQueueForUnknowStatus->isChecked());
	// Попап при сбросе очереди
	settings->setBoolSetting(Settings::SettingResetQueuePopup, checkbox_ResetQueuePopup->isChecked());
	// Настройка слотов
	statisticFooterPos.clear();
	int statParam = slot1Combo->itemData(slot1Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 1;
	}
	statParam = slot2Combo->itemData(slot2Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 2;
	}
	statParam = slot3Combo->itemData(slot3Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 3;
	}
	statParam = slot4Combo->itemData(slot4Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 4;
	}
	statParam = slot5Combo->itemData(slot5Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 5;
	}
	statParam = slot6Combo->itemData(slot6Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 6;
	}
	statParam = slot7Combo->itemData(slot7Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 7;
	}
	statParam = slot8Combo->itemData(slot8Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 8;
	}
	statParam = slot9Combo->itemData(slot9Combo->currentIndex()).toInt();
	if (statParam != -1) {
		statisticFooterPos[statParam] = 9;
	}
	fullUpdateFooterStatistic();
	// Режим сохранения параметров персонажа
	settings->setIntSetting(Settings::SettingPersSaveMode, persParamSaveMode_combo->currentIndex());
	// Сохранение параметров персонажа
	settings->setBoolSetting(Settings::SettingSavePersParams, saveMainPersParam_checkbox->isChecked());
	// Сохранение рюкзака персонажа
	settings->setBoolSetting(Settings::SettingSaveBackpack, savePersBackpack_checkbox->isChecked());
	// Сохранение статистики
	settings->setBoolSetting(Settings::SettingSaveStatistic, saveStatistic_checkbox->isChecked());
	// Применение фильтров вещей
	pers->setThingsFiltersEx(thingFiltersTable->getFilters());
	// Применение шрифтов
	QFont f;
	if (f.fromString(persNameFont_label->fontName())) {
		persNameLabel->setFont(f);
	}
	if (f.fromString(gameTextFont_label->fontName())) {
		serverTextView->setFont(f);
		consoleTextView->setFont(f);
	}
	// Максимальное количество блоков текста
	int textBlocksCount = maxTextBlocksCount->value();
	if (textBlocksCount < 0) {
		textBlocksCount = 0;
	} else if (textBlocksCount > 0 && textBlocksCount < 100) {
		textBlocksCount = 100;
	}
	serverTextView->setMaximumBlockCount(textBlocksCount);
	consoleTextView->setMaximumBlockCount(textBlocksCount);
	settings->setIntSetting(Settings::SettingServerTextBlocksCount, textBlocksCount);
	// Расцветка текста игры
	settings->setBoolSetting(Settings::SettingGameTextColoring, gameTextColoring->isChecked());
	// Режим сохранения карт
	GameMap *maps = GameMap::instance();
	maps->setMapsParam(GameMap::AutoSaveMode, mapsParamSaveMode->currentIndex());
	// Цвет позиции персонажа
	maps->setPersPosColor(btnPersPosColor->getColor());
	// Период автовыгрузки карт
	maps->setUnloadInterval(mapsUnloadInterval->value());
	// Длительность регена для отображения popup-а
	i = (restPopup->isChecked()) ? restDurationPopup->value() : 0;
	settings->setIntSetting(Settings::SettingRegenDurationForPopup, i);
	// Позиция персонажа в центре карты
	settings->setBoolSetting(Settings::SettingPersPosInCenter, checkbox_PersPosInCenter->isChecked());
	// Особые враги
	specificEnemiesTable->save();
}

void SofMainWindow::saveSettings()
{
	applySettings();
	Settings *settings = Settings::instance();
	QDomDocument xmlDoc;
	settings->setSlotsData(exportSlotsSettings(xmlDoc));
	settings->setAppearanceData(exportAppearanceSettings(xmlDoc));
	settings->save();
}

void SofMainWindow::userCommandChanged()
{
	if (!autoEnterMode)
		return;
	QString sText = userCommandLine->toPlainText().trimmed();
	int strLen = sText.length();
	if (strLen == 1) {
		if (sText != "/") {
			if (sText >= "0" && sText <= "9") {
				userCommandLine->setText(QString());
				PluginCore::instance()->sendString(sText);
			}
		}
	}
}

void SofMainWindow::userCommandReturnPressed()
{
	QString sText = userCommandLine->toPlainText().trimmed();
	if (!sText.isEmpty()) {
		userCommandLine->appendMessageHistory(sText);
		PluginCore::instance()->sendString(sText);
	}
	userCommandLine->setText(QString());
}

void SofMainWindow::timeoutEvent()
{
	int secs = QDateTime::currentDateTime().secsTo(*timeoutStamp);
	if (secs > 0) {
		QString str1 = "";
		if (settingTimeOutDisplay == 2 && secs >= 60) {
			int mins = secs / 60;
			str1.append(QString::number(mins) + QString::fromUtf8(" мин."));
			secs = secs - mins * 60;
		}
		if (secs != 0) {
			if (!str1.isEmpty()) {
				str1.append(QString::fromUtf8(" "));
			}
			str1.append(QString::number(secs) + QString::fromUtf8(" сек."));
		}
		setEventValue(timeoutEventSlot, 0, &str1);
	} else {
		timeoutTimer->stop();
		freeEventSlot(timeoutEventSlot);
	}
}

void SofMainWindow::saveMap()
{
	GameMap::instance()->saveMap();
}

void SofMainWindow::createMap()
{
	while (1) {
		bool fOk = false;
		QString newMapName = QInputDialog::getText(this, QString::fromUtf8("Новая карта"), QString::fromUtf8("Введите имя новой карты"), QLineEdit::Normal, QString(), &fOk);
		//QString newMapName = QInputDialog::getText(this, QString::fromUtf8("Новая карта"), QString::fromUtf8("Введите имя новой карты"), QLineEdit::Normal, QString::Null, &fOk);
		if (!fOk)
			break;
		if (newMapName.isEmpty())
			continue;
		if (GameMap::instance()->createMap(newMapName) >= 0)
			break;
		QMessageBox::critical(this, QString::fromUtf8("Создание карты"), QString::fromUtf8("Произошла ошибка. Возможно такая карта уже существует"));
	}
}

void SofMainWindow::clearMap()
{
	struct GameMap::maps_info mapsInfo;
	GameMap *map = GameMap::instance();
	map->mapsInfo(&mapsInfo);
	int currIndex = mapsInfo.curr_map_index;
	if (currIndex == -1)
		return;
	if (QMessageBox::critical(this, QString::fromUtf8("Очистка карты"), QString::fromUtf8("Вы действительно хотите очистить текущую карту?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		map->clearMap(currIndex);
	}
	return;
}

void SofMainWindow::mapShowContextMenu(const QPoint &pos)
{
	/**
	* Обработчик события вызова контекстного меню
	**/
	QPointF viewPos = gameMapView->mapToScene(pos);
	selectedMapElement = GameMap::instance()->getIndexByCoordinate(viewPos);
	if (selectedMapElement != -1) {
		actionMarkMapElement->setEnabled(true);
		actionMoveMapElement->setEnabled(true);
		actionRemoveMapElement->setEnabled(true);
	} else {
		actionMarkMapElement->setEnabled(false);
		actionMoveMapElement->setEnabled(false);
		actionRemoveMapElement->setEnabled(false);
	}
	mapMenu->exec(gameMapView->mapToGlobal(pos));
}

void SofMainWindow::moveMapElement()
{
	/**
	* Вызов окна переноса элемента карты на другую карту
	* А также сам перенос элемента карты
	**/
	if (selectedMapElement == -1)
		return;
	// Определяем имя текущей карты
	struct GameMap::maps_info mapsInfo;
	GameMap *map = GameMap::instance();
	map->mapsInfo(&mapsInfo);
	int currIndex = mapsInfo.curr_map_index;
	if (currIndex == -1)
		return;
	// Запрашиваем список карт
	QVector<GameMap::maps_list2> mapsList;
	map->getMapsList(&mapsList);
	// Выкидываем текущую карту из списка
	QStringList maps;
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		if (i != mapsInfo.curr_map_index) {
			maps.push_back(mapsList[i].name);
		}
	}
	if (maps.size() == 0) {
		QMessageBox::critical(this, QString::fromUtf8("Перенос элемента карты"), QString::fromUtf8("Нет доступных карт для переноса"));
		return;
	}
	// Отображаем окно выбора карты
	bool fOk = false;
	int idx = maps.indexOf(lastMapForMoveElement);
	if (idx == -1)
		idx = 0;
	QString mapName = QInputDialog::getItem(this, QString::fromUtf8("Выбор карты"), QString::fromUtf8("Выберите карту, в которую перенести элемент:"), maps, idx, false, &fOk);
	if (fOk && !mapName.isEmpty()) {
		// Ищем индекс выбранной карты по ее имени
		int mapIndex = -1;
		int i;
		for (i = 0; i < cnt; i++) {
			if (mapsList[i].name == mapName) {
				mapIndex = mapsList[i].index;
				break;
			}
		}
		// Переносим элемент
		if (mapIndex != -1) {
			lastMapForMoveElement = mapsList.at(i).name;
			map->moveMapElement(currIndex, mapIndex, selectedMapElement);
		} else {
			QMessageBox::critical(this, QString::fromUtf8("Перенос элемента карты"), QString::fromUtf8("Выбранная карта не найдена"));
		}
	}
}

/**
 * Удаление элемента карты с запросом
 */
void SofMainWindow::removeMapElement()
{
	if (selectedMapElement == -1)
		return;
	// Определяем имя текущей карты
	struct GameMap::maps_info mapsInfo;
	GameMap *map = GameMap::instance();
	map->mapsInfo(&mapsInfo);
	int currIndex = mapsInfo.curr_map_index;
	if (currIndex == -1)
		return;
	if (QMessageBox::critical(this, QString::fromUtf8("Удаление элемента карты"), QString::fromUtf8("Вы действительно хотите удалить элемент карты?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		map->removeMapElement(currIndex, selectedMapElement);
	}
	return;
}

/**
 * Вызов диалога настройки метки элемента карты
 */
void SofMainWindow::markMapElement()
{
	if (selectedMapElement == -1)
		return;
	GameMap *map = GameMap::instance();
	// Получаем данные об метке
	GameMap::MapElementMark mark = map->getMapElementMark(selectedMapElement);
	// Создаем и вызываем диалог
	MapMarkEdit *dlg = new MapMarkEdit(mark.enabled, mark.title, mark.color, this);
	dlg->exec();
	// Получаем результат
	int res = dlg->getResult();
	if (res != MapMarkEdit::ResNone) {
		if (res == MapMarkEdit::ResSave) {
			map->setMapElementMark(selectedMapElement, dlg->getMarkTitle(), dlg->getMarkColor());
		} else if (res == MapMarkEdit::ResRemove) {
			map->removeMapElementMark(selectedMapElement);
		}
	}
	delete dlg;
}

/**
 * Заполняет данные вещей в табличном виджете
 */
void SofMainWindow::showThings(int tab_num)
{
	thingsTable->setFilter(thingsTabBar->tabData(tab_num).toInt() + 1);
}

/**
 * Отображает итог под таблицей с вещами
 */
void SofMainWindow::showThingsSummary()
{
	int nCountAll = thingsTable->summaryCount();
	int nPriceAll = thingsTable->summaryPriceAll();
	int noPrice = thingsTable->summaryNoPriceCount();
	labelThingsCountAll->setText(numToStr(nCountAll, "'"));
	QString str1 = numToStr(nPriceAll, "'");
	if (noPrice != 0)
		str1.append("+");
	labelThingsPriceAll->setText(str1);
}

/**
 * Изменился состав вещей у персонажа
 */
void SofMainWindow::persThingsChanged()
{
	if (currentPage != 3) {
		// Если текущая страница не вещи
		thingsChanged = true;
		// Меняем цвет текста кнопки
		thingsModeBtn->setStyleSheet("color:red;");
	}
}

/**
* Изменились параметры персонажа
*/
void SofMainWindow::persParamChanged(int paramId, int paramType, int paramValue)
{
	if (paramType == TYPE_INTEGER_FULL) {
		if (paramId == Pers::ParamPersStatus) {
			// Статус персонажа
			changePersStatus();
		} else if (paramId == Pers::ParamHealthCurr) {
			// Текущее здоровье
			setCurrentHealth(paramValue);
		} else if (paramId == Pers::ParamEnergyCurr) {
			// Текущая энергия
			setCurrentEnergy(paramValue);
		} else if (paramId == Pers::ParamEnergyMax) {
			// Максимальная энергия
			int i = energyBar->value();
			energyBar->setValue(0);
			energyBar->setRange(0, paramValue);
			energyBar->setValue(i);
		} else if (paramId == Pers::ParamHealthMax) {
			// Максимальное здоровье
			int i = healthBar->value();
			healthBar->setValue(0);
			healthBar->setRange(0, paramValue);
			healthBar->setValue(i);
		} else if (paramId == Pers::ParamPersLevel) {
			// Смена уровня персонажа
			levelLabel->setText(QString::number(paramValue));
		} else if (paramId == Pers::ParamCoordinates) {
			scrollMapToPersPosition();
		} else if (paramId == Pers::ParamMoneysCount) {
			// Количество денег
			labelMoneysCount->setText(numToStr(paramValue, "'"));
		}
	} else if (paramType == TYPE_LONGLONG_FULL) {
		if (paramId == Pers::ParamExperienceCurr) {
			// Изменился опыт
			long long exp;
			if (!Pers::instance()->getLongParamValue(Pers::ParamExperienceCurr, &exp)) {
				exp = 0;
			}
			setCurrentExperience(exp);
		} else if (paramId == Pers::ParamExperienceMax) {
			// Изменился максимальный опыт для уровня
			long long exp;
			if (!Pers::instance()->getLongParamValue(Pers::ParamExperienceMax, &exp)) {
				exp = 0;
			}
			setMaximumExperience(exp);
		}
	} else if (paramType == TYPE_STRING) {
		if (paramId == Pers::ParamPersName) {
			// Имя персонажа
			QString str1 = Pers::instance()->name();
			if (str1.isEmpty())
				str1 = "n/a";
			persNameLabel->setText(str1);
		}
	} else if (paramType == TYPE_NA) {
		if (paramId == Pers::ParamPersLevel) {
			levelLabel->setText("n/a");
		} else if (paramId == Pers::ParamPersName) {
			persNameLabel->setText("n/a");
		} else if (paramId == Pers::ParamExperienceCurr) {
			// Максимальный опыт для уровня
			setCurrentExperience(0);
		} else if (paramId == Pers::ParamExperienceMax) {
			// Максимальный опыт для уровня
			setMaximumExperience(0);
		} else if (paramId == Pers::ParamExperienceMax) {
			// Количество денег
			labelMoneysCount->setText(NA_TEXT);
		}
	}
}

void SofMainWindow::updateThingFiltersTab()
{
	int current_tab = thingsTabBar->currentIndex();
	disconnect(thingsTabBar, SIGNAL(currentChanged(int)), this, SLOT(showThings(int)));
	while (thingsTabBar->count() > 0)
		thingsTabBar->removeTab(0);
	thingsTabBar->setTabData(thingsTabBar->addTab(QString::fromUtf8("Все вещи")), -1);
	int fltr_index = 0;
	foreach (ThingFilter* tf, Pers::instance()->thingsFiltersList()) {
		if (tf->isActive()) {
			thingsTabBar->setTabData(thingsTabBar->addTab(tf->name()), fltr_index);
		}
		fltr_index++;
	}
	if (current_tab < 0 || current_tab >= thingsTabBar->count())
		current_tab = 0;
	connect(thingsTabBar, SIGNAL(currentChanged(int)), this, SLOT(showThings(int)));
	thingsTabBar->setCurrentIndex(current_tab);
	showThings(current_tab);
}

/**
 * Отображает длину очереди команд
 */
void SofMainWindow::showQueueLen(int len)
{
	if (len != 0) {
		if (queueEventSlot == 0) {
			queueEventSlot = getEventSlot();
			if (queueEventSlot != 0) {
				QString str1 = QString::fromUtf8("Очередь:");
				QString str2 = QString::number(len);
				setEventValue(queueEventSlot, &str1, &str2);
			}
		} else {
			QString str2 = QString::number(len);
			setEventValue(queueEventSlot, 0, &str2);
		}
	} else {
		if (queueEventSlot != 0) {
			freeEventSlot(queueEventSlot);
			queueEventSlot = 0;
		}
	}
}

/**
 * Вызывает диалог выбора фонта
 */
void SofMainWindow::chooseFont(QAbstractButton* button)
{
	bool fOk;
	QFont font;
	int i = (fontButtonGroup->buttons()).indexOf(button);
	if (i != -1) {
		font.fromString(fontLabelGroup.at(i)->fontName());
		// ensure we don't use the new native font dialog on mac with Qt 4.5,
		//   since it was broken last we checked (qt task #252000)
#if QT_VERSION >= 0x040500
		QString fnt = QFontDialog::getFont(&fOk, font, this, QString(), QFontDialog::DontUseNativeDialog).toString();
#else
		QString fnt = QFontDialog::getFont(&fOk, font, this).toString();
#endif
		if (fOk) {
			fontLabelGroup[i]->setFont(fnt);
		}
	}
}

void SofMainWindow::updateStatistic(int type)
{
	updateValue(type, Statistic::instance()->toString(type));
}
