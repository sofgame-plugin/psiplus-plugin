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
	statLabelsCaption.fill(0, STAT_PARAMS_COUNT);
	statLabelsValues.fill(0, STAT_PARAMS_COUNT);
	footerStatLabels.fill(0, SLOT_ITEMS_COUNT * 2);
	// Верхние кнопочки
	connect(mainModeBtn, SIGNAL(released()), SLOT(activateMainPage()));
	connect(fightModeBtn, SIGNAL(released()), SLOT(activateFightPage()));
	connect(persInfoModeBtn, SIGNAL(released()), SLOT(activatePersInfoPage()));
	connect(fingsModeBtn, SIGNAL(released()), SLOT(activateFingsPage()));
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
	// Текст игры
	connect (serverTextLabel, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(textShowContextMenu(const QPoint &)));
	// Консоль игры
	connect (console_textedit, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(consoleShowContextMenu(const QPoint &)));
	// Строка команд
	connect(userCommandLine, SIGNAL(returnPressed()), SLOT(userCommandReturnPressed()));
	connect(userCommandLine, SIGNAL(textChanged(QString)), SLOT(userCommandChanged()));
	// Кнопка ввода команд
	connect(cmd_send, SIGNAL(clicked()), SLOT(userCommandReturnPressed()));
	// Таббар вещей (фильтры)
	fingsTabBar = new QTabBar(page_4);
	connect(fingsTabBar, SIGNAL(currentChanged(int)), this, SLOT(showFings(int)));
	// Таблица вещей
	thingsIface = 0;
	QLayout* lt = page_4->layout();
	lt->removeWidget(fingsTable);
	lt->removeItem(fingsSummaryLayout);
	lt->addWidget(fingsTabBar);
	lt->addWidget(fingsTable);
	lt->addItem(fingsSummaryLayout);
	// Создаем контекстное меню вещей
	actionSetFingPrice = new QAction(fingsTable);
	actionSetFingPrice->setText(QString::fromUtf8("Цена у торговца"));
	actionSetFingPrice->setStatusTip(QString::fromUtf8("Цена для продажи торговцу"));
	connect(actionSetFingPrice, SIGNAL(triggered()), this, SLOT(setFingPrice()));
	actionFingsParamToConsole = new QAction(fingsTable);
	actionFingsParamToConsole->setText(QString::fromUtf8("Сбросить параметры в консоль"));
	actionFingsParamToConsole->setStatusTip(QString::fromUtf8("Сбросить параметры вещи в консоль"));
	connect(actionFingsParamToConsole, SIGNAL(triggered()), this, SLOT(fingParamToConsole()));
	fingsMenu = new QMenu(this);
	fingsMenu->addAction(actionSetFingPrice);
	fingsMenu->addAction(actionFingsParamToConsole);
	connect(fingsTable, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fingsShowContextMenu(const QPoint &)));
	fingsTable->setContextMenuPolicy(Qt::CustomContextMenu);
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
	statLabelsCaption[VALUE_LAST_GAME_JID] = lastGameJidCaption_label;
	statLabelsValues[VALUE_LAST_GAME_JID] = lastGameJidValue_label;
	statLabelsCaption[VALUE_LAST_CHAT_JID] = lastGameChatCaption_label;
	statLabelsValues[VALUE_LAST_CHAT_JID] = lastGameChatValue_label;
	statLabelsCaption[VALUE_MESSAGES_COUNT] = gameMessagesCountCaption_label;
	statLabelsValues[VALUE_MESSAGES_COUNT] = gameMessagesCountValue_label;
	connect(resetCommonStatBtn, SIGNAL(released()), SLOT(resetCommonStatistic()));
	statLabelsCaption[VALUE_FIGHTS_COUNT] = fingsCountCaption_label;
	statLabelsValues[VALUE_FIGHTS_COUNT] = fingsCountValue_label;
	statLabelsCaption[VALUE_DAMAGE_MAX_FROM_PERS] = damageMaxFromPersCaption_label;
	statLabelsValues[VALUE_DAMAGE_MAX_FROM_PERS] = damageMaxFromPersValue_label;
	statLabelsCaption[VALUE_DAMAGE_MIN_FROM_PERS] = damageMinFromPersCaption_label;
	statLabelsValues[VALUE_DAMAGE_MIN_FROM_PERS] = damageMinFromPersValue_label;
	statLabelsCaption[VALUE_DROP_MONEYS] = dropMoneysCaption_label;
	statLabelsValues[VALUE_DROP_MONEYS] = dropMoneysValue_label;
	statLabelsCaption[VALUE_FINGS_DROP_COUNT] = fingsDropCountCaption_label;
	statLabelsValues[VALUE_FINGS_DROP_COUNT] = fingsDropCountValue_label;
	statLabelsCaption[VALUE_FING_DROP_LAST] = fingDropLastCaption_label;
	statLabelsValues[VALUE_FING_DROP_LAST] = fingDropLastValue_label;
	statLabelsCaption[VALUE_EXPERIENCE_DROP_COUNT] = experienceDropCountCaption_label;
	statLabelsValues[VALUE_EXPERIENCE_DROP_COUNT] = experienceDropCountValue_label;
	statLabelsCaption[VALUE_KILLED_ENEMIES] = killedEnemiesCaption_label;
	statLabelsValues[VALUE_KILLED_ENEMIES] = killedEnemiesValue_label;
	connect(resetFightsStatBtn, SIGNAL(released()), SLOT(resetFightStatistic()));
	// Таблицы для настройки фильтров
	fingFiltersTable->init(&filtersList);
	fingRulesTable->init(&filtersList);
	connect(fingFiltersTable, SIGNAL(currFilterChanged(int)), fingRulesTable, SLOT(currFilterChanged(int)));
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
	footerStatLabels[0] = slot1Caption_label;
	footerStatLabels[1] = slot1Value_label;
	footerStatLabels[2] = slot2Caption_label;
	footerStatLabels[3] = slot2Value_label;
	footerStatLabels[4] = slot3Caption_label;
	footerStatLabels[5] = slot3Value_label;
	footerStatLabels[6] = slot4Caption_label;
	footerStatLabels[7] = slot4Value_label;
	footerStatLabels[8] = slot5Caption_label;
	footerStatLabels[9] = slot5Value_label;
	footerStatLabels[10] = slot6Caption_label;
	footerStatLabels[11] = slot6Value_label;
	footerStatLabels[12] = slot7Caption_label;
	footerStatLabels[13] = slot7Value_label;
	footerStatLabels[14] = slot8Caption_label;
	footerStatLabels[15] = slot8Value_label;
	footerStatLabels[16] = slot9Caption_label;
	footerStatLabels[17] = slot9Value_label;
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
	connect(pers, SIGNAL(fingsChanged()), this, SLOT(persFingsChanged()));
	connect(pers, SIGNAL(persParamChanged(int, int, int)), this, SLOT(persParamChanged(int, int, int)));
	// Сигнал на смену фильтров
	connect(pers, SIGNAL(filtersChanged()), this, SLOT(updateFingFiltersTab()));
	// Инициируем форму
	init();
	// Устанавливаем фокус ввода
	userCommandLine->setFocus();
}

SofMainWindow::~SofMainWindow()
{
	Pers *pers = Pers::instance();
	if (thingsIface)
		pers->removeThingsInterface(thingsIface);
	if (timeoutStamp)
		delete timeoutStamp;
	if (timeoutTimer)
		delete timeoutTimer;
	while (!filtersList.isEmpty()) {
		delete filtersList.takeFirst();
	}
	disconnect(pers, SIGNAL(fingsChanged()), this, SLOT(persFingsChanged()));
	disconnect(pers, SIGNAL(persParamChanged(int, int, int)), this, SLOT(persParamChanged(int, int, int)));
	disconnect(stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(changePage(int)));
	disconnect (serverTextLabel, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(textShowContextMenu(const QPoint &)));
	disconnect (console_textedit, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(consoleShowContextMenu(const QPoint &)));
	disconnect(pers, SIGNAL(filtersChanged()), this, SLOT(updateFingFiltersTab()));

	disconnect(fontButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(chooseFont(QAbstractButton*)));
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
	statMessagesCount = 0;
	statDropMoneys = 0;
	statFingsCount = 0;
	statFightsCount = 0;
	statDamageMinFromPers = -1;
	statDamageMaxFromPers = -1;
	statGameJid = "";
	statFingDropLast = "";
	currentPage = 0;
	settingTimeOutDisplay = 0;
	selectedMapElement = -1;
	fingsChanged = false;
	timeoutEventSlot = 0;
	queueEventSlot = 0;
	queueShowFlag = false;
	// Настройки шрифтов
	QFont f = persNameLabel->font();
	persNameFont_label->setFont(f.toString());
	f = serverTextLabel->font();
	gameTextFont_label->setFont(f.toString());
	console_textedit->setFont(f.toString());
	// Получаем основные настройки окна
	getAllDataFromCore();
	// Считываем параметры окна
	if (windowSizePosCombo->currentIndex() != 0) {
		int wnd_x = 0;
		int wnd_y = 0;
		PluginCore *core = PluginCore::instance();
		if (core->getIntSettingValue(SETTING_WINDOW_POS_X, &wnd_x)) {
			if (core->getIntSettingValue(SETTING_WINDOW_POS_Y, &wnd_y)) {
				int wnd_width = 0; int wnd_height = 0;
				if (core->getIntSettingValue(SETTING_WINDOW_WIDTH, &wnd_width)) {
					if (core->getIntSettingValue(SETTING_WINDOW_HEIGHT, &wnd_height)) {
						// Определяем геометрию рабочих столов (доступное пространство)
						QRect screenRect = QApplication::desktop()->availableGeometry(-1); //-- default system screen
						// Корректируем наши параметры, если превышают допустимые
						if (screenRect.x() > wnd_x) {
							wnd_x = screenRect.x();
						}
						if (screenRect.y() > wnd_y) {
							wnd_y = screenRect.y();
						}
						if (wnd_x - screenRect.x() + wnd_width > screenRect.width()) {
							// * Не вписываемся по горизонтали
							// Исправляем за счет смещения окна
							wnd_x -= (wnd_x + wnd_width) - (screenRect.x() + screenRect.width());
							if (wnd_x < screenRect.x()) {
								wnd_x = screenRect.x();
								// Исправляем за счет ширины окна
								wnd_width = screenRect.width() - wnd_x + screenRect.x();
							}
						}
						if (wnd_y - screenRect.y() + wnd_height > screenRect.height()) {
							// * Не вписываемся по вертикали
							// Исправляем за счет смещения окна
							wnd_y -= (wnd_y + wnd_height) - (screenRect.y() + screenRect.height());
							if (wnd_y < screenRect.y()) {
								wnd_y = screenRect.y();
								// Исправляем за счет высоты окна
								wnd_height = screenRect.height() - wnd_y + screenRect.y();
							}
						}
						// Перемещаем окно
						move(wnd_x, wnd_y);
						// Изменяем размер окна
						resize(wnd_width, wnd_height);
					}
				}
			}
		}
	}
	// Размеры сплиттера
	QList<int> sizes;
	sizes.push_back(1);
	sizes.push_back(1);
	mapSplitter->setSizes(sizes);
	// Инициируем Label-ы
	fullUpdateFooterStatistic();
	// Таблица вещей
	thingsIface = Pers::instance()->getThingsInterface();
	//myPers->setThingsInterfaceFilter(thingsIface, 2);
	if (thingsIface)
		fingsTable->setModel(Pers::instance()->getThingsModel(thingsIface));
	fingsTable->init();
	// Убираем табы фильтров вещей
	updateFingFiltersTab();
	// Меняем цвет текста кнопки вещей
	QPalette palette;
	QBrush brush;
	brush.setColor(QColor(0, 0, 0, 255)); // Черный
	brush.setStyle(Qt::SolidPattern);
	palette.setBrush(QPalette::Active, QPalette::ButtonText, brush);
	palette.setBrush(QPalette::Inactive, QPalette::ButtonText, brush);
	fingsModeBtn->setPalette(palette);
	// Сбрасываем режим автоввода
	setAutoEnterMode(false);
	// Инициируем слоты событий и уведомлений
	initEventSlots();
}

void SofMainWindow::setAutoEnterMode(bool mode)
{
	if (autoEnterMode != mode) {
		autoEnterMode = mode;
		QPalette palette1;
		QPalette palette2;
		QBrush brush1;
		QBrush brush2;
		QBrush brush3;
		QBrush brush4;
		if (mode) {
			brush1.setColor(QColor(255, 0, 0, 64)); // Красный
			brush2.setColor(QColor(255, 0, 0, 64)); // Красный
			brush3.setColor(QColor(255, 0, 0, 64)); // Красный
			brush4.setColor(QColor(255, 0, 0, 64)); // Красный
		} else {
			brush1.setColor(QApplication::palette().color(QPalette::Active, QPalette::Button));
			brush2.setColor(QApplication::palette().color(QPalette::Inactive, QPalette::Button));
			brush3.setColor(QApplication::palette().color(QPalette::Active, QPalette::Base));
			brush4.setColor(QApplication::palette().color(QPalette::Inactive, QPalette::Base));
		}
		brush1.setStyle(Qt::SolidPattern);
		brush2.setStyle(Qt::SolidPattern);
		brush3.setStyle(Qt::SolidPattern);
		brush4.setStyle(Qt::SolidPattern);
		palette1.setBrush(QPalette::Active, QPalette::Button, brush1);
		palette1.setBrush(QPalette::Inactive, QPalette::Button, brush2);
		palette2.setBrush(QPalette::Active, QPalette::Base, brush3);
		palette2.setBrush(QPalette::Inactive, QPalette::Base, brush4);
		cmd_send->setPalette(palette1);
		userCommandLine->setPalette(palette2);
	}
}

void SofMainWindow::closeEvent(QCloseEvent *evnt)
{
	/**
	* Реакция на закрытие окна
	**/
	// Отправка координат окна
	QPoint wndPos = pos();
	qint32 i = wndPos.x();
	PluginCore *core = PluginCore::instance();
	core->setIntSettingValue(SETTING_WINDOW_POS_X, i);
	i = wndPos.y();
	core->setIntSettingValue(SETTING_WINDOW_POS_Y, i);
	// Отправка размеров окна
	i = width();
	core->setIntSettingValue(SETTING_WINDOW_WIDTH, i);
	i = height();
	core->setIntSettingValue(SETTING_WINDOW_HEIGHT, i);
	// Отправка команды сохранения настроек ядру плагина
	core->sendCommandToCore(COMMAND_CLOSE_WINDOW);
	evnt->accept();
}

void SofMainWindow::fillSlotCombo(QComboBox* slotCombo)
{
	slotCombo->clear();
	slotCombo->insertItem(0, QString::fromUtf8("* Пустой *"));
	slotCombo->insertItem(1, statisticCaption.at(VALUE_LAST_GAME_JID));
	slotCombo->insertItem(2, statisticCaption.at(VALUE_LAST_CHAT_JID));
	slotCombo->insertItem(3, statisticCaption.at(VALUE_MESSAGES_COUNT));
	slotCombo->insertItem(4, statisticCaption.at(VALUE_DAMAGE_MAX_FROM_PERS));
	slotCombo->insertItem(5, statisticCaption.at(VALUE_DAMAGE_MIN_FROM_PERS));
	slotCombo->insertItem(6, statisticCaption.at(VALUE_FIGHTS_COUNT));
	slotCombo->insertItem(7, statisticCaption.at(VALUE_DROP_MONEYS));
	slotCombo->insertItem(8, statisticCaption.at(VALUE_FINGS_DROP_COUNT));
	slotCombo->insertItem(9, statisticCaption.at(VALUE_FING_DROP_LAST));
	slotCombo->insertItem(10, statisticCaption.at(VALUE_EXPERIENCE_DROP_COUNT));
	slotCombo->insertItem(11, statisticCaption.at(VALUE_KILLED_ENEMIES));
}

void SofMainWindow::initStatisticData()
{
	// Заполняем заголовки наименований элементов статистики
	statisticCaption.push_back(QString::fromUtf8("JID игры"));
	statisticStartValue.push_back(QString::fromUtf8("n/a"));
	statisticCaption.push_back(QString::fromUtf8("JID чата"));
	statisticStartValue.push_back(QString::fromUtf8("n/a"));
	statisticCaption.push_back(QString::fromUtf8("Сообщений"));
	statisticStartValue.push_back(QString::fromUtf8("0"));
	statisticCaption.push_back(QString::fromUtf8("Лучший удар"));
	statisticStartValue.push_back(QString::fromUtf8("n/a"));
	statisticCaption.push_back(QString::fromUtf8("Худший удар"));
	statisticStartValue.push_back(QString::fromUtf8("n/a"));
	statisticCaption.push_back(QString::fromUtf8("Всего боев"));
	statisticStartValue.push_back(QString::fromUtf8("0"));
	statisticCaption.push_back(QString::fromUtf8("Денег собрано"));
	statisticStartValue.push_back(QString::fromUtf8("0"));
	statisticCaption.push_back(QString::fromUtf8("Вещей собрано"));
	statisticStartValue.push_back(QString::fromUtf8("0"));
	statisticCaption.push_back(QString::fromUtf8("Последняя вещь"));
	statisticStartValue.push_back(QString::fromUtf8("n/a"));
	statisticCaption.push_back(QString::fromUtf8("Полученный опыт"));
	statisticStartValue.push_back(QString::fromUtf8("0"));
	statisticCaption.push_back(QString::fromUtf8("Противников повержено"));
	statisticStartValue.push_back(QString::fromUtf8("0"));
	// Заполняем привязку статистики к fotter-у начальными данными
	statisticFooterPos.fill(-1, STAT_PARAMS_COUNT);
	// Заполняем массив индекса fotter-а начальными значениями
	footerStatSettings.fill(-1, SLOT_ITEMS_COUNT);
}

void SofMainWindow::setStatisticCaptionText()
{
	// Заполняем текст статистики в окне статистика
	for (int i = 0; i < STAT_PARAMS_COUNT; i++) {
		if (statLabelsCaption.at(i)) {
			statLabelsCaption.at(i)->setText(statisticCaption[i] + ":");
		}
	}
}

void SofMainWindow::fullUpdateFooterStatistic()
{
	// Заполняем значения QLabel-ов данными из массива
	// TODO Сделать нормальное обновление, в том числе скрывать неиспользуемые элементы и разделители
	int j;
	for (int i = 0; i < footerStatLabels.size(); i += 2) {
		footerStatLabels.at(i)->setText(MINUS_TEXT);
		footerStatLabels.at(i+1)->setText(MINUS_TEXT);
	}
	for (int i = 0; i < STAT_PARAMS_COUNT; i++) {
		j = statisticFooterPos[i];
		if (j >= 0 && j < SLOT_ITEMS_COUNT) {
			footerStatLabels.at(j*2)->setText(statisticCaption[i] + ":");
			footerStatLabels.at(j*2+1)->setText(statLabelsValues[i]->text());
		}
	}
}

void SofMainWindow::setFooterStatisticValue(int statisticId, QString* valuePtr)
{
	// Функция обновляет значения статистики в футере, если эта статистика там отображается
	int i;
	i = statisticFooterPos[statisticId];
	if (i >= 0 && i < SLOT_ITEMS_COUNT) {
		footerStatLabels.at(i*2+1)->setText(*valuePtr);
	}
}

void SofMainWindow::setStatisticValue(int statisticId, QString* valuePtr)
{
	// Функция обновляет значения статистики в в окне статистики
	if (statisticId >= 0 && statisticId < STAT_PARAMS_COUNT) {
		if (statLabelsValues.at(statisticId)) {
			statLabelsValues.at(statisticId)->setText(*valuePtr);
		}
	}
}

void SofMainWindow::getAllDataFromCore() {
	// Фунцкия опрашивает ядро для получения данных
	int newIntValue;
	QString newStrValue;
	// *** Настройки плагина ***
	PluginCore *core = PluginCore::instance();
	// Имя персонажа
	if (!core->getStringSettingValue(SETTING_PERS_NAME, &newStrValue)) {
		newStrValue = NA_TEXT;
	}
	setPersName->setText(newStrValue);
	// Режим переключения зеркал
	if (!core->getIntSettingValue(SETTING_CHANGE_MIRROR_MODE, &newIntValue)) {
		newIntValue = 0;
	}
	mirrorChangeModeCombo->setCurrentIndex(newIntValue);
	// Сохранение параметров окна
	if (!core->getIntSettingValue(SETTING_WINDOW_SIZE_POS, &newIntValue)) {
		newIntValue = 0;
	}
	windowSizePosCombo->setCurrentIndex(newIntValue);
	// Отслеживание восстановления здоровья и энергии
	if (!core->getIntSettingValue(SETTING_REST_HEALTH_ENERGY, &newIntValue)) {
		newIntValue = 0;
	}
	restHealthEnergyCombo->setCurrentIndex(newIntValue);
	// Таймер в бою
	if (!core->getIntSettingValue(SETTING_FIGHT_TIMER, &newIntValue)) {
		newIntValue = 0;
	}
	settingTimeOutDisplay = newIntValue;
	fightTimerCombo->setCurrentIndex(newIntValue);
	// Выбор боя
	if (!core->getIntSettingValue(SETTING_FIGHT_SELECT_ACTION, &newIntValue)) {
		newIntValue = 0;
	}
	fightSelectAction->setCurrentIndex(newIntValue);
	// Автозакрытие боя
	if (!core->getIntSettingValue(SETTING_AUTOCLOSE_FIGHT, &newIntValue)) {
		newIntValue = 0;
	}
	setAutoCloseFight->setCurrentIndex(newIntValue);
	// Попап при дропе вещей
	if (!core->getIntSettingValue(SETTING_FING_DROP_POPUP, &newIntValue)) {
		newIntValue = 0;
	}
	Qt::CheckState state = Qt::Unchecked;
	if (newIntValue == 1) {
		state = Qt::Checked;
	}
	checkbox_FingDropPopup->setCheckState(state);
	// Попап при заказе в клубе убийц
	if (!core->getIntSettingValue(SETTING_IN_KILLERS_CUP_POPUP, &newIntValue)) {
		newIntValue = 0;
	}
	state = Qt::Unchecked;
	if (newIntValue == 1) {
		state = Qt::Checked;
	}
	checkbox_InKillersCupPopup->setCheckState(state);
	// Попап при нападении убийцы
	if (!core->getIntSettingValue(SETTING_KILLER_ATTACK_POPUP, &newIntValue)) {
		newIntValue = 0;
	}
	state = Qt::Unchecked;
	if (newIntValue == 1) {
		state = Qt::Checked;
	}
	checkbox_KillerClubAttack->setCheckState(state);
	// Отображение длины очереди
	if (!core->getIntSettingValue(SETTING_SHOW_QUEUE_LENGTH, &newIntValue)) {
		newIntValue = 0;
	}
	state = Qt::Unchecked;
	if (newIntValue == 1) {
		state = Qt::Checked;
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
	checkbox_ShowQueueLength->setCheckState(state);
	// Сброс очереди при неизвестном статусе
	if (!core->getIntSettingValue(SETTING_RESET_QUEUE_FOR_UNKNOW_STATUS, &newIntValue)) {
		newIntValue = 0;
	}
	state = Qt::Unchecked;
	if (newIntValue == 1) {
		state = Qt::Checked;
	}
	checkbox_ResetQueueForUnknowStatus->setCheckState(state);
	// Попап при сбросе очереди
	if (!core->getIntSettingValue(SETTING_RESET_QUEUE_POPUP_SHOW, &newIntValue) || newIntValue != 1) {
		checkbox_ResetQueuePopup->setChecked(false);
	} else {
		checkbox_ResetQueuePopup->setChecked(true);
	}
	// Позиции в слотах
	statisticFooterPos.fill(-1, STAT_PARAMS_COUNT);
	if (core->getIntSettingValue(SETTING_SLOT1, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 0;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot1Combo->setCurrentIndex(newIntValue);
	if (core->getIntSettingValue(SETTING_SLOT2, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 1;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot2Combo->setCurrentIndex(newIntValue);
	if (core->getIntSettingValue(SETTING_SLOT3, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 2;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot3Combo->setCurrentIndex(newIntValue);
	if (core->getIntSettingValue(SETTING_SLOT4, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 3;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot4Combo->setCurrentIndex(newIntValue);
	if (core->getIntSettingValue(SETTING_SLOT5, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 4;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot5Combo->setCurrentIndex(newIntValue);
	if (core->getIntSettingValue(SETTING_SLOT6, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 5;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot6Combo->setCurrentIndex(newIntValue);
	if (core->getIntSettingValue(SETTING_SLOT7, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 6;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot7Combo->setCurrentIndex(newIntValue);
	if (core->getIntSettingValue(SETTING_SLOT8, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 7;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot8Combo->setCurrentIndex(newIntValue);
	if (core->getIntSettingValue(SETTING_SLOT9, &newIntValue)) {
		if (newIntValue >= 0 && newIntValue < STAT_PARAMS_COUNT) {
			statisticFooterPos[newIntValue] = 8;
			++newIntValue;
		} else {
			newIntValue = 0;
		}
	} else {
		newIntValue = 0;
	}
	slot9Combo->setCurrentIndex(newIntValue);
	// Сохранение статистики
	//if (!core->getIntSettingValue(SETTING_SAVE_STAT, &newIntValue)) {
	//	newIntValue = 0;
	//}
	//setSaveStatAtExit->setCurrentIndex(newIntValue);
	// Сохранение основных параметров персонажа
	if (!core->getIntSettingValue(SETTING_PERS_PARAM_SAVE_MODE, &newIntValue)) {
		newIntValue = 0;
	}
	persParamSaveMode_combo->setCurrentIndex(newIntValue);
	// Сохранение основных параметров персонажа
	if (!core->getIntSettingValue(SETTING_SAVE_PERS_PARAM, &newIntValue)) {
		newIntValue = 0;
	}
	state = Qt::Unchecked;
	if (newIntValue == 1) {
		state = Qt::Checked;
	}
	saveMainPersParam_checkbox->setCheckState(state);
	// Сохранение рюкзака персонажа
	if (!core->getIntSettingValue(SETTING_SAVE_PERS_BACKPACK, &newIntValue)) {
		newIntValue = 0;
	}
	state = Qt::Unchecked;
	if (newIntValue == 1) {
		state = Qt::Checked;
	}
	savePersBackpack_checkbox->setCheckState(state);
	// Сохранение статистики
	if (!core->getIntSettingValue(SETTING_SAVE_PERS_STAT, &newIntValue)) {
		newIntValue = 0;
	}
	state = Qt::Unchecked;
	if (newIntValue == 1) {
		state = Qt::Checked;
	}
	saveStatistic_checkbox->setCheckState(state);
	// *** Основные данные ***
	Pers *pers = Pers::instance();
	changePersStatus();
	if (!pers->getStringParamValue(VALUE_PERS_NAME, &newStrValue)) {
		newStrValue = NA_TEXT;
	}
	persNameLabel->setText(newStrValue);
	if (!pers->getIntParamValue(VALUE_PERS_LEVEL, &newIntValue)) {
		newStrValue = NA_TEXT;
	} else {
		newStrValue = QString::number(newIntValue);
	}
	levelLabel->setText(newStrValue);
	if (!core->getIntValue(VALUE_EXPERIENCE_CURR, &newIntValue)) {
		newIntValue = 0;
	}
	experienceLabel->setText(numToStr(newIntValue, "'"));
	experienceBar->setValue(newIntValue);
	if (!core->getIntValue(VALUE_EXPERIENCE_MAX, &newIntValue)) {
		newIntValue = 0;
	}
	experienceBar->setRange(0, newIntValue);
	if (!pers->getIntParamValue(VALUE_HEALTH_CURR, &newIntValue)) {
		newIntValue = 0;
	}
	setCurrentHealth(newIntValue);
	if (!pers->getIntParamValue(VALUE_HEALTH_MAX, &newIntValue)) {
		newIntValue = 0;
	}
	healthBar->setRange(0, newIntValue);
	if (!pers->getIntParamValue(VALUE_ENERGY_CURR, &newIntValue)) {
		newIntValue = 0;
	}
	setCurrentEnergy(newIntValue);
	if (!pers->getIntParamValue(VALUE_ENERGY_MAX, &newIntValue)) {
		newIntValue = 0;
	}
	energyBar->setRange(0, newIntValue);

	if (core->getIntValue(VALUE_CHANGE_PERS_POS, &newIntValue)) {
		// Новая позиция персонажа
		//scrollMapNewPosition(newIntValue % 100000, newIntValue / 100000);
		scrollMapToPersPosition();
	}
	// *** Статистические данные ***
	if (core->getIntValue(VALUE_DROP_MONEYS, &newIntValue)) {
		newStrValue = numToStr(newIntValue, "'");
	} else {
		newStrValue = statisticStartValue.at(VALUE_DROP_MONEYS);
	}
	updateValue(VALUE_DROP_MONEYS, &newStrValue);
	if (core->getIntValue(VALUE_MESSAGES_COUNT, &newIntValue)) {
		newStrValue = numToStr(newIntValue, "'");
	} else {
		newStrValue = statisticStartValue.at(VALUE_MESSAGES_COUNT);
	}
	updateValue(VALUE_MESSAGES_COUNT, &newStrValue);
	if (!core->getTextValue(VALUE_LAST_GAME_JID, &newStrValue)) {
		newStrValue = statisticStartValue.at(VALUE_LAST_GAME_JID);
	}
	updateValue(VALUE_LAST_GAME_JID, &newStrValue);
	if (!core->getTextValue(VALUE_LAST_CHAT_JID, &newStrValue)) {
		newStrValue = statisticStartValue.at(VALUE_LAST_CHAT_JID);
	}
	updateValue(VALUE_LAST_CHAT_JID, &newStrValue);
	if (core->getIntValue(VALUE_DAMAGE_MIN_FROM_PERS, &newIntValue)) {
		newStrValue = numToStr(newIntValue, "'");
	} else {
		newStrValue = statisticStartValue.at(VALUE_DAMAGE_MIN_FROM_PERS);
	}
	updateValue(VALUE_DAMAGE_MIN_FROM_PERS, &newStrValue);
	if (core->getIntValue(VALUE_DAMAGE_MAX_FROM_PERS, &newIntValue)) {
		newStrValue = numToStr(newIntValue, "'");
	} else {
		newStrValue = statisticStartValue.at(VALUE_DAMAGE_MAX_FROM_PERS);
	}
	updateValue(VALUE_DAMAGE_MAX_FROM_PERS, &newStrValue);
	if (core->getIntValue(VALUE_FINGS_DROP_COUNT, &newIntValue)) {
		newStrValue = numToStr(newIntValue, "'");
	} else {
		newStrValue = statisticStartValue.at(VALUE_FINGS_DROP_COUNT);
	}
	updateValue(VALUE_FINGS_DROP_COUNT, &newStrValue);
	if (core->getIntValue(VALUE_FIGHTS_COUNT, &newIntValue)) {
		newStrValue = numToStr(newIntValue, "'");
	} else {
		newStrValue = statisticStartValue.at(VALUE_FIGHTS_COUNT);
	}
	updateValue(VALUE_FIGHTS_COUNT, &newStrValue);
	if (!core->getTextValue(VALUE_FING_DROP_LAST, &newStrValue)) {
		newStrValue = statisticStartValue.at(VALUE_FING_DROP_LAST);
	}
	updateValue(VALUE_FING_DROP_LAST, &newStrValue);
	if (core->getIntValue(VALUE_EXPERIENCE_DROP_COUNT, &newIntValue)) {
		newStrValue = numToStr(newIntValue, "'");
	} else {
		newStrValue = statisticStartValue.at(VALUE_EXPERIENCE_DROP_COUNT);
	}
	updateValue(VALUE_EXPERIENCE_DROP_COUNT, &newStrValue);
	if (core->getIntValue(VALUE_KILLED_ENEMIES, &newIntValue)) {
		newStrValue = numToStr(newIntValue, "'");
	} else {
		newStrValue = statisticStartValue.at(VALUE_KILLED_ENEMIES);
	}
	updateValue(VALUE_KILLED_ENEMIES, &newStrValue);
	// Шрифты
	if (core->getStringSettingValue(SETTING_PERS_NAME_FONT, &newStrValue)) {
		persNameFont_label->setFont(newStrValue);
		QFont f;
		if (f.fromString(persNameFont_label->fontName())) {
			persNameLabel->setFont(f);
		}
	}
	if (core->getStringSettingValue(SETTING_SERVER_TEXT_FONT, &newStrValue)) {
		gameTextFont_label->setFont(newStrValue);
		QFont f;
		if (f.fromString(gameTextFont_label->fontName())) {
			serverTextLabel->setFont(f);
			console_textedit->setFont(f);
		}
	}
	newIntValue = 0;
	if (core->getIntSettingValue(SETTING_SERVER_TEXT_BLOCKS_COUNT, &newIntValue)) {
		if (newIntValue < 0) {
			newIntValue = 0;
		} else if (newIntValue > 0 && newIntValue < 100) {
			newIntValue = 100;
		}
	}
	maxTextBlocksCount->setValue(newIntValue);
	serverTextLabel->setMaximumBlockCount(newIntValue);
	console_textedit->setMaximumBlockCount(newIntValue);
	// Режим сохранения карт
	if (!core->getIntSettingValue(SETTING_MAPS_PARAM_SAVE_MODE, &newIntValue)) {
		newIntValue = 0;
	}
	mapsParamSaveMode->setCurrentIndex(newIntValue);
}

void SofMainWindow::updateValue(int valueId, QString* valuePtr) {
	// Функция обновляет элементы отображения данных. В том числе и в footer-е.

	// Проверка выхода индекса за пределы массива
	if (valueId < 0 || valueId >= STAT_PARAMS_COUNT) {
		return;
	}
	// Обновляем основной элемент
	QString str1 = *valuePtr;
	setStatisticValue(valueId, valuePtr);
	// Обновляем footer
	setFooterStatisticValue(valueId, valuePtr);
}

void SofMainWindow::valueChanged(int eventId, int valueType, int value)
{
	// Функция обработки события изменения данных.
	// Есть ли данные в value и какие они, зависит от valueType
	QString str1;
	if (valueType == TYPE_INTEGER_FULL) {
		// В событии есть полные данные
		str1 = numToStr(value, "'");
		if (eventId == VALUE_DROP_MONEYS) {
			// Это упавшие деньги
			updateValue(VALUE_DROP_MONEYS, &str1);
		} else if (eventId == VALUE_MESSAGES_COUNT) {
			// Счетчик сообщений
			updateValue(VALUE_MESSAGES_COUNT, &str1);
		} else if (eventId == VALUE_KILLED_ENEMIES) {
			// Повержено противников
			updateValue(VALUE_KILLED_ENEMIES, &str1);
		} else if (eventId == VALUE_CHANGE_PERS_POS) {
			// Перемещение персонажа
			//scrollMapNewPosition(value % 100000, value / 100000);
			scrollMapToPersPosition();
		} else if (eventId == VALUE_EXPERIENCE_CURR) {
			// Изменился опыт
			experienceLabel->setText(numToStr(value, "'"));
			experienceBar->setValue(value);
		} else if (eventId == VALUE_TIMEOUT) {
			// Пришло событие таймаута
			if (settingTimeOutDisplay != 0) {
				setTimeout(value);
			}
		} else if (eventId == VALUE_EXPERIENCE_DROP_COUNT) {
			// Добавился опыт после боя
			updateValue(VALUE_EXPERIENCE_DROP_COUNT, &str1);
		} else if (eventId == VALUE_DAMAGE_MIN_FROM_PERS) {
			// Минимальный урон от персонажа
			updateValue(VALUE_DAMAGE_MIN_FROM_PERS, &str1);
		} else if (eventId == VALUE_DAMAGE_MAX_FROM_PERS) {
			// Максимальный урон от персонажа
			updateValue(VALUE_DAMAGE_MAX_FROM_PERS, &str1);
		} else if (eventId == VALUE_FINGS_DROP_COUNT) {
			// Количество упавших вещей
			statFingsCount = value;
			updateValue(VALUE_FINGS_DROP_COUNT, &str1);
			// Последняя упавшая вещь
			if (PluginCore::instance()->getTextValue(VALUE_FING_DROP_LAST, &str1)) {
				str1 = str1.left(20);
			} else {
				str1 = NA_TEXT;
			}
			updateValue(VALUE_FING_DROP_LAST, &str1);
		} else if (eventId == VALUE_FIGHTS_COUNT) {
			updateValue(VALUE_FIGHTS_COUNT, &str1);
		} else if (eventId == VALUE_EXPERIENCE_MAX) {
			// Изменился максимальный опыт для уровня
			int i = experienceBar->value();
			experienceBar->setValue(0);
			experienceBar->setRange(0, value);
			experienceBar->setValue(i);
		}
	} else if (valueType == TYPE_STRING) {
		// Строковые данные. За значением нужно обращаться к ядру плагина.
		if (eventId == VALUE_LAST_GAME_JID) {
			// Текущий игровой JID
			if (PluginCore::instance()->getTextValue(eventId, &str1)) {
				str1 = str1.left(20);
			} else {
				str1 = NA_TEXT;
			}
			updateValue(VALUE_LAST_GAME_JID, &str1);
		}
	} else if (valueType == TYPE_NA) {
		// Данные недоступны или неопределены (n/a)
		str1 = NA_TEXT;
		if (eventId == VALUE_DAMAGE_MIN_FROM_PERS) {
			// Минимальный урон от персонажа
			updateValue(VALUE_DAMAGE_MIN_FROM_PERS, &str1);
		} else if (eventId == VALUE_DAMAGE_MAX_FROM_PERS) {
			// Максимальный урон от персонажа
			updateValue(VALUE_DAMAGE_MAX_FROM_PERS, &str1);
		} else if (eventId == VALUE_LAST_GAME_JID) {
			// JID игры
			updateValue(VALUE_LAST_GAME_JID, &str1);
		} else if (eventId == VALUE_LAST_CHAT_JID) {
			// JID чата
			updateValue(VALUE_LAST_CHAT_JID, &str1);
		} else if (eventId == VALUE_FING_DROP_LAST) {
			// Последняя найденная вещь
			updateValue(VALUE_FING_DROP_LAST, &str1);
		} else if (eventId == VALUE_EXPERIENCE_CURR) {
			// Максимальный опыт для уровня
			experienceLabel->setText(str1);
			experienceBar->setValue(0);
		} else if (eventId == VALUE_EXPERIENCE_MAX) {
			// Максимальный опыт для уровня
			experienceBar->setRange(0, 0);
		} else if (eventId == VALUE_HEALTH_CURR) {
			// Текущее здоровье
			healthLabel->setText(str1);
			healthBar->setValue(0);
		} else if (eventId == VALUE_HEALTH_MAX) {
			// Максимальное здоровье
			healthBar->setRange(0, 0);
		} else if (eventId == VALUE_ENERGY_CURR) {
			// Текущая энергия
			energyLabel->setText(str1);
			energyBar->setValue(0);
		} else if (eventId == VALUE_ENERGY_MAX) {
			// Максимальная энергия
			energyBar->setRange(0, 0);
		}
	//} else if (valueType == TYPE_INTEGER_INC) {
		// Данные есть в value, причем для прибавления к прошлым

	}
}

void SofMainWindow::changePersStatus()
{
	persStatusLabel->setText(Pers::instance()->getPersStatusString());
}

void SofMainWindow::setGameText(QString gameText, int type)
{
	/**
	* gameText - строка для отображения, если gameText == 0, то очищаем
	* type - 1: исходящее, 2: входящее
	**/
	if (gameText.isEmpty()) {
		serverTextLabel->clear();
	}
	if (gameText.isEmpty()) {
		serverTextLabel->clear();
	}
	// Значение 32 тупо взято из аналогичного кода psi+
	bool do_scroll = (serverTextLabel->verticalScrollBar()->maximum() - serverTextLabel->verticalScrollBar()->value() <= 32);
	//serverTextLabel->moveCursor(QTextCursor::End);
	serverTextLabel->appendPlainText(QString::fromUtf8("") + (type == 1 ? "^" : "v") + " ***** [" + QTime::currentTime().toString("hh:mm:ss") + "] ***** " + (type == 1 ? "^" : "v"));
	if (!gameText.isEmpty()) {
		serverTextLabel->appendPlainText(gameText);
	} else {
		serverTextLabel->appendPlainText(QString::fromUtf8("--- очищено ---"));
	}
	if (type == 1) {
		text_tabWidget->setCurrentIndex(0);
	}
	if (do_scroll || type == 1) {
		serverTextLabel->verticalScrollBar()->setValue(serverTextLabel->verticalScrollBar()->maximum());
	}
}

/**
 * text - строка для отображения, если text == "", то очищаем
 * type - 1: исходящее, 2: входящее
 * switch_ - переключать или нет на консоль
 */
void SofMainWindow::setConsoleText(QString text, int type, bool switch_)
{
	if (text.isEmpty()) {
		console_textedit->clear();
	}
	//console_textedit->moveCursor(QTextCursor::End);
	console_textedit->appendPlainText(QString::fromUtf8("") + (type == 1 ? "^" : "v") + " ***** [" + QTime::currentTime().toString("hh:mm:ss") + "] ***** " + (type == 1 ? "^" : "v"));
	if (!text.isEmpty()) {
		console_textedit->appendPlainText(text);
	} else {
		console_textedit->appendPlainText(QString::fromUtf8("--- очищено ---"));
	}
	console_textedit->verticalScrollBar()->setValue(console_textedit->verticalScrollBar()->maximum());
	//console_textedit->moveCursor(QTextCursor::End);
	if (switch_) {
		text_tabWidget->setCurrentIndex(1);
	}
}

/**
 * Возвращает DOM элемент, содержащий настройки внешнего вида
 */
QDomElement SofMainWindow::getAppearanceSettings(QDomDocument &xmlDoc) const
{
	QDomElement eAppearance = xmlDoc.createElement("appearance");
	QDomElement ePersNameAppe = xmlDoc.createElement("pers-name");
	eAppearance.appendChild(ePersNameAppe);
	QDomElement ePersNameFontAppe = xmlDoc.createElement("font");
	ePersNameAppe.appendChild(ePersNameFontAppe);
	ePersNameFontAppe.setAttribute("value", persNameFont_label->fontName());
	QDomElement eServerTextAppe = xmlDoc.createElement("server-text");
	eAppearance.appendChild(eServerTextAppe);
	QDomElement eServerTextFontAppe = xmlDoc.createElement("font");
	eServerTextAppe.appendChild(eServerTextFontAppe);
	eServerTextFontAppe.setAttribute("value", gameTextFont_label->fontName());
	QDomElement eThingsTable = fingsTable->saveSettingsToXml(xmlDoc);
	eAppearance.appendChild(eThingsTable);
	return eAppearance;
}

/**
 * Применяет настройки внешнего вида
 */
void SofMainWindow::setAppearanceSetting(QDomElement &xml)
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
					serverTextLabel->setFont(f);
					console_textedit->setFont(f);
				}
			}
		}
	}
	QDomElement eThingsSettings = xml.firstChildElement("things-table");
	fingsTable->loadSettingsFromXml(eThingsSettings);
}

void SofMainWindow::initEventSlots()
{
	/**
	* Инициирует слоты событий
	**/
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

void SofMainWindow::scrollMapNewPosition(int x, int y)
{
	/**
	* Прокручивает карту так, что бы была видна часть карты с указанными координатами
	**/
	QRectF coordinates = GameMap::instance()->getSceneCoordinates(x, y);
	if (!coordinates.isNull()) {
		gameMapView->ensureVisible(coordinates, 50, 50);
	}
}

void SofMainWindow::scrollMapToPersPosition()
{
	/**
	* Прокручивает карту так, что бы была видна позиция персонажа
	**/
	QGraphicsItem* persItem = GameMap::instance()->getPersItem();
	if (persItem) {
		gameMapView->ensureVisible(persItem, 50, 50);
	}
}

void SofMainWindow::setCurrentHealth(int health)
{
	/**
	* Прописывает текущее здоровье в виджеты
	**/
	int maxValue;
	if (!Pers::instance()->getIntParamValue(VALUE_HEALTH_MAX, &maxValue)) {
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
	if (health > 0) {
		healthBar->setValue(health);
	} else {
		healthBar->setValue(0);
	}
}

void SofMainWindow::setCurrentEnergy(int energy)
{
	/**
	* Прописывает текущую энергию в виджеты
	**/
	int maxValue;
	if (!Pers::instance()->getIntParamValue(VALUE_ENERGY_MAX, &maxValue)) {
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
	if (energy > 0) {
		energyBar->setValue(energy);
	} else {
		energyBar->setValue(0);
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
		if (fingsChanged) { // Список вещей менялся
			// Меняем цвет текста кнопки
			QPalette palette;
			QBrush brush;
			brush.setColor(QColor(0, 0, 0, 255)); // Черный
			brush.setStyle(Qt::SolidPattern);
			palette.setBrush(QPalette::Active, QPalette::ButtonText, brush);
			palette.setBrush(QPalette::Inactive, QPalette::ButtonText, brush);
			fingsModeBtn->setPalette(palette);
		}
		// Инициируем табы и таблицу
		if (fingsTabBar->count() == 0) {
			updateFingFiltersTab();
			showFings(0);
		//} else {
		//	if (fingsChanged) {
		//		showFings(fingsTabBar->currentIndex());
		//	}
		}
		fingsChanged = false;
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

void SofMainWindow::activateFingsPage()
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
	PluginCore *core = PluginCore::instance();
	core->resetStatistic(VALUE_LAST_GAME_JID);
	core->resetStatistic(VALUE_LAST_CHAT_JID);
	core->resetStatistic(VALUE_MESSAGES_COUNT);
}

void SofMainWindow::resetFightStatistic()
{
	PluginCore *core = PluginCore::instance();
	core->resetStatistic(VALUE_FIGHTS_COUNT);
	core->resetStatistic(VALUE_DAMAGE_MAX_FROM_PERS);
	core->resetStatistic(VALUE_DAMAGE_MIN_FROM_PERS);
	core->resetStatistic(VALUE_DROP_MONEYS);
	core->resetStatistic(VALUE_FINGS_DROP_COUNT);
	core->resetStatistic(VALUE_FING_DROP_LAST);
	core->resetStatistic(VALUE_EXPERIENCE_DROP_COUNT);
	core->resetStatistic(VALUE_KILLED_ENEMIES);
}

void SofMainWindow::applySettings()
{
	// *** Отправляем новые настройки ядру плагина ***
	int i, j;
	PluginCore *core = PluginCore::instance();
	// Имя персонажа
	QString str1 = setPersName->text();
	core->setStringSettingValue(SETTING_PERS_NAME, &str1);
	// Режим переключения зеркал
	i = mirrorChangeModeCombo->currentIndex();
	core->setIntSettingValue(SETTING_CHANGE_MIRROR_MODE, i);
	// Сохранение параметров окна
	i = windowSizePosCombo->currentIndex();
	core->setIntSettingValue(SETTING_WINDOW_SIZE_POS, i);
	// Отслеживание восстановления здоровья и энергии
	i = restHealthEnergyCombo->currentIndex();
	core->setIntSettingValue(SETTING_REST_HEALTH_ENERGY, i);
	// Таймер в бою
	i = fightTimerCombo->currentIndex();
	settingTimeOutDisplay = i;
	core->setIntSettingValue(SETTING_FIGHT_TIMER, i);
	// Выбор боя
	i = fightSelectAction->currentIndex();
	core->setIntSettingValue(SETTING_FIGHT_SELECT_ACTION, i);
	// Автозакрытие боя
	i = setAutoCloseFight->currentIndex();
	core->setIntSettingValue(SETTING_AUTOCLOSE_FIGHT, i);
	// Попап при дропе вещей
	i = checkbox_FingDropPopup->checkState();
	if (i == Qt::Checked) {
		i = 1;
	} else {
		i = 0;
	}
	core->setIntSettingValue(SETTING_FING_DROP_POPUP, i);
	// Попап при заказе в клубе убийц
	i = checkbox_InKillersCupPopup->checkState();
	if (i == Qt::Checked) {
		i = 1;
	} else {
		i = 0;
	}
	core->setIntSettingValue(SETTING_IN_KILLERS_CUP_POPUP, i);
	// Попап при нападении убийцы
	i = checkbox_KillerClubAttack->checkState();
	if (i == Qt::Checked) {
		i = 1;
	} else {
		i = 0;
	}
	core->setIntSettingValue(SETTING_KILLER_ATTACK_POPUP, i);
	// Отображение очереди команд
	i = checkbox_ShowQueueLength->checkState();
	if (i == Qt::Checked) {
		i = 1;
		if (!queueShowFlag) {
			connect(Sender::instance(), SIGNAL(queueSizeChanged(int)), this, SLOT(showQueueLen(int)));
			queueShowFlag = true;
		}
	} else {
		i = 0;
		if (queueShowFlag) {
			disconnect(Sender::instance(), SIGNAL(queueSizeChanged(int)), this, SLOT(showQueueLen(int)));
			showQueueLen(0);
			queueShowFlag = false;
		}
	}
	core->setIntSettingValue(SETTING_SHOW_QUEUE_LENGTH, i);
	// Сброс очереди при неизвестном статусе
	i = checkbox_ResetQueueForUnknowStatus->checkState();
	if (i == Qt::Checked) {
		i = 1;
	} else {
		i = 0;
	}
	core->setIntSettingValue(SETTING_RESET_QUEUE_FOR_UNKNOW_STATUS, i);
	// Попап при сбросе очереди
	i = (checkbox_ResetQueuePopup->isChecked()) ? 1 : 0;
	core->setIntSettingValue(SETTING_RESET_QUEUE_POPUP_SHOW, i);
	// Настройка слотов
	statisticFooterPos.fill(-1, STAT_PARAMS_COUNT);
	j = slot1Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 0;
	} else {
		core->setIntSettingValue(SETTING_SLOT1, 0);
	}
	j = slot2Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 1;
	} else {
		core->setIntSettingValue(SETTING_SLOT2, 0);
	}
	j = slot3Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 2;
	} else {
		core->setIntSettingValue(SETTING_SLOT3, 0);
	}
	j = slot4Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 3;
	} else {
		core->setIntSettingValue(SETTING_SLOT4, 0);
	}
	j = slot5Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 4;
	} else {
		core->setIntSettingValue(SETTING_SLOT5, 0);
	}
	j = slot6Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 5;
	} else {
		core->setIntSettingValue(SETTING_SLOT6, 0);
	}
	j = slot7Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 6;
	} else {
		core->setIntSettingValue(SETTING_SLOT7, 0);
	}
	j = slot8Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 7;
	} else {
		core->setIntSettingValue(SETTING_SLOT8, 0);
	}
	j = slot9Combo->currentIndex() - 1;
	if (j >= 0 && j < STAT_PARAMS_COUNT) {
		statisticFooterPos[j] = 8;
	} else {
		core->setIntSettingValue(SETTING_SLOT9, 0);
	}
	for (i = 0; i < STAT_PARAMS_COUNT; i++) {
		j = statisticFooterPos[i];
		core->setIntSettingValue(j + SETTING_SLOT1, i);
	}
	fullUpdateFooterStatistic();
	// Режим сохранения параметров персонажа
	i = persParamSaveMode_combo->currentIndex();
	core->setIntSettingValue(SETTING_PERS_PARAM_SAVE_MODE, i);
	// Сохранение параметров персонажа
	i = saveMainPersParam_checkbox->checkState();
	if (i == Qt::Checked) {
		i = 1;
	} else {
		i = 0;
	}
	core->setIntSettingValue(SETTING_SAVE_PERS_PARAM, i);
	// Сохранение рюкзака персонажа
	i = savePersBackpack_checkbox->checkState();
	if (i == Qt::Checked) {
		i = 1;
	} else {
		i = 0;
	}
	core->setIntSettingValue(SETTING_SAVE_PERS_BACKPACK, i);
	// Сохранение статистики
	i = saveStatistic_checkbox->checkState();
	if (i == Qt::Checked) {
		i = 1;
	} else {
		i = 0;
	}
	core->setIntSettingValue(SETTING_SAVE_PERS_STAT, i);
	// Применение фильтров вещей
	Pers::instance()->setFingsFiltersEx(fingFiltersTable->getFilters());
	// Применение шрифтов
	QFont f;
	if (f.fromString(persNameFont_label->fontName())) {
		persNameLabel->setFont(f);
		str1 = f.toString();
		core->setStringSettingValue(SETTING_PERS_NAME_FONT, &str1);
	}
	if (f.fromString(gameTextFont_label->fontName())) {
		serverTextLabel->setFont(f);
		console_textedit->setFont(f);
		str1 = f.toString();
		core->setStringSettingValue(SETTING_SERVER_TEXT_FONT, &str1);
	}
	int textBlocksCount = maxTextBlocksCount->value();
	if (textBlocksCount < 0) {
		textBlocksCount = 0;
	} else if (textBlocksCount > 0 && textBlocksCount < 100) {
		textBlocksCount = 100;
	}
	serverTextLabel->setMaximumBlockCount(textBlocksCount);
	core->setIntSettingValue(SETTING_SERVER_TEXT_BLOCKS_COUNT, textBlocksCount);
	// Режим сохранения карт
	i = mapsParamSaveMode->currentIndex();
	core->setIntSettingValue(SETTING_MAPS_PARAM_SAVE_MODE, i);

}

void SofMainWindow::saveSettings()
{
	// Отправляем настройки ядру
	applySettings();
	// Отправка команды сохранения настроек ядру плагина
	PluginCore::instance()->sendCommandToCore(COMMAND_SAVE_SETTINGS);
}

void SofMainWindow::resetGameJid()
{
	PluginCore::instance()->resetStatistic(VALUE_LAST_GAME_JID);
}

void SofMainWindow::userCommandChanged()
{
	if (!autoEnterMode)
		return;
	QString sText = userCommandLine->text().trimmed();
	int strLen = sText.length();
	if (strLen == 1) {
		if (sText != "/") {
			if (sText >= "0" && sText <= "9") {
				setGameText(sText, 1);
				userCommandLine->setText("");
				PluginCore::instance()->sendString(sText);
			}
		}
	}
}

void SofMainWindow::userCommandReturnPressed()
{
	QString sText = userCommandLine->text().trimmed();
	if (!sText.isEmpty()) {
		if (sText.startsWith("/")) {
			if (sText == "/1+") {
				setAutoEnterMode(true);
				userCommandLine->setText("");
				return;
			} else if (sText.startsWith("/1-")) {
				if (sText.length() == 3) {
					setAutoEnterMode(false);
				} else {
					sText = sText.mid(3);
					setGameText(sText, 1);
					PluginCore::instance()->sendString(sText);
				}
				userCommandLine->setText("");
				return;
			} else if (sText == "/1") {
				sText = "Auto enter mode is ";
				if (autoEnterMode) {
					sText.append("ON");
				} else {
					sText.append("OFF");
				}
				setGameText(sText, 2);
				setConsoleText(sText, 2, false);
				userCommandLine->setText("");
				return;
			} else {
				setConsoleText(sText, 1, true);
			}
		} else {
			setGameText(sText, 1);
		}
		userCommandLine->setText("");
		PluginCore::instance()->sendString(sText);
	}
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
		if (GameMap::instance()->createMap(&newMapName) >= 0)
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
	selectedMapElement = GameMap::instance()->getIndexByCoordinate(viewPos.x(), viewPos.y());
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

void SofMainWindow::textShowContextMenu(const QPoint &pos)
{
	QMenu *menu = serverTextLabel->createStandardContextMenu();
	menu->exec(serverTextLabel->mapToGlobal(pos));
	delete menu;
}

void SofMainWindow::consoleShowContextMenu(const QPoint &pos)
{
	QMenu *menu = console_textedit->createStandardContextMenu();
	menu->exec(console_textedit->mapToGlobal(pos));
	delete menu;
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
void SofMainWindow::showFings(int tab_num)
{
	int flt_num = fingsTabBar->tabData(tab_num).toInt() + 1;
	Pers::instance()->setThingsInterfaceFilter(thingsIface, flt_num);
	showThingsSummary();
}

/**
 * Отображает итог под таблицей с вещами
 */
void SofMainWindow::showThingsSummary()
{
	Pers *pers = Pers::instance();
	int nCountAll = pers->getFingsCount(thingsIface);
	int nPriceAll = pers->getPriceAll(thingsIface);
	int noPrice = pers->getNoPriceCount(thingsIface);
	labelFingsCountAll->setText(numToStr(nCountAll, "'"));
	QString str1 = numToStr(nPriceAll, "'");
	if (noPrice != 0)
		str1.append("+");
	labelFingsPriceAll->setText(str1);
}

void SofMainWindow::fingsShowContextMenu(const QPoint &pos)
{
	/**
	* Отображает контекстное меню для таблицы с вещами
	**/
	if (fingsTable->currentIndex().row() >= 0) {
		actionSetFingPrice->setEnabled(true);
		actionFingsParamToConsole->setEnabled(true);
	} else {
		actionSetFingPrice->setEnabled(false);
		actionFingsParamToConsole->setEnabled(false);
	}
	fingsMenu->exec(fingsTable->mapToGlobal(pos));
}

/**
 * Вызывает и обрабатывает диалог редактирования цены вещи
 */
void SofMainWindow::setFingPrice()
{
	Pers *pers = Pers::instance();
	// Получаем номер строки
	int row = fingsTable->currentIndex().row();
	// Получаем указатель на вещь
	const Thing *thg = pers->getFingByRow(row, thingsIface);
	if (!thg || !thg->isValid())
		return;
	// Получаем имя выбранной вещи и цену
	QString s_name = QString::fromUtf8("Укажите цену для ") + thg->name() + QString::fromUtf8(", или -1 для сброса цены.");
	int n_price = thg->price();
	bool f_ok = false;
	int new_price = QInputDialog::getInt(this, QString::fromUtf8("Цена торговца"), s_name, n_price, -1, 2147483647, 1, &f_ok, 0);
	if (f_ok && n_price != new_price) {
		// Меняем цену вещи
		pers->setFingPrice(thingsIface, row, new_price);
		// Пересчитываем итоги таблицы
		showThingsSummary();
	}
}

/**
 * Формирует строку с параметрами вещи и отсылает ее в консоль плагина
 */
void SofMainWindow::fingParamToConsole()
{
	int row = fingsTable->currentIndex().row();
	const Thing *thg = Pers::instance()->getFingByRow(row, thingsIface);
	if (thg) {
		if (thg->isValid()) {
			setConsoleText(thg->toString(Thing::ShowAll), 2, true);
		}
	}
}

/**
 * Изменился состав вещей у персонажа
 */
void SofMainWindow::persFingsChanged()
{
	if (currentPage != 3) {
		// Если текущая страница не вещи
		fingsChanged = true;
		// Меняем цвет текста кнопки
		QPalette palette;
		QBrush brush;
		brush.setColor(QColor(255, 0, 0, 255)); // Красный
		brush.setStyle(Qt::SolidPattern);
		palette.setBrush(QPalette::Active, QPalette::ButtonText, brush);
		palette.setBrush(QPalette::Inactive, QPalette::ButtonText, brush);
		fingsModeBtn->setPalette(palette);
	}
}

/**
* Изменились параметры персонажа
*/
void SofMainWindow::persParamChanged(int paramId, int paramType, int paramValue)
{
	if (paramType == TYPE_INTEGER_FULL) {
		if (paramId == VALUE_PERS_STATUS) {
			// Статус персонажа
			changePersStatus();
		} else if (paramId == VALUE_HEALTH_CURR) {
			// Текущее здоровье
			setCurrentHealth(paramValue);
		} else if (paramId == VALUE_ENERGY_CURR) {
			// Текущая энергия
			setCurrentEnergy(paramValue);
		} else if (paramId == VALUE_ENERGY_MAX) {
			// Максимальная энергия
			int i = energyBar->value();
			energyBar->setValue(0);
			energyBar->setRange(0, paramValue);
			energyBar->setValue(i);
		} else if (paramId == VALUE_HEALTH_MAX) {
			// Максимальное здоровье
			int i = healthBar->value();
			healthBar->setValue(0);
			healthBar->setRange(0, paramValue);
			healthBar->setValue(i);
		} else if (paramId == VALUE_PERS_LEVEL) {
			// Смена уровня персонажа
			levelLabel->setText(QString::number(paramValue));
		}
	} else if (paramType == TYPE_STRING) {
		if (paramId == VALUE_PERS_NAME) {
			// Имя персонажа
			QString str1;
			if (!Pers::instance()->getStringParamValue(VALUE_PERS_NAME, &str1)) {
				str1 = NA_TEXT;
			}
			persNameLabel->setText(str1);
		}
	} else if (paramType == TYPE_NA) {
		if (paramId == VALUE_PERS_LEVEL) {
			levelLabel->setText(NA_TEXT);
		} else if (paramId == VALUE_PERS_NAME) {
			persNameLabel->setText(NA_TEXT);
		}
	}
}

void SofMainWindow::updateFingFiltersTab()
{
	int current_tab = fingsTabBar->currentIndex();
	disconnect(fingsTabBar, SIGNAL(currentChanged(int)), this, SLOT(showFings(int)));
	while (fingsTabBar->count() > 0)
		fingsTabBar->removeTab(0);
	fingsTabBar->setTabData(fingsTabBar->addTab(QString::fromUtf8("Все вещи")), -1);
	QList<FingFilter*> filtersList;
	Pers::instance()->getFingsFiltersEx(&filtersList);
	int fltr_index = 0;
	while (!filtersList.isEmpty()) {
		FingFilter* ff = filtersList.takeFirst();
		if (ff->isActive()) {
			fingsTabBar->setTabData(fingsTabBar->addTab(ff->name()), fltr_index);
		}
		fltr_index++;
	}
	if (current_tab < 0 || current_tab >= fingsTabBar->count())
		current_tab = 0;
	connect(fingsTabBar, SIGNAL(currentChanged(int)), this, SLOT(showFings(int)));
	fingsTabBar->setCurrentIndex(current_tab);
	showFings(current_tab);
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
