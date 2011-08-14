/*
 * main_window.h - Sof Game Psi plugin
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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QWidget>

#include "pers.h"
#include "ui_main_window.h"

QT_BEGIN_NAMESPACE
	class QDateTime;
	class QLabel;
	class QCheckBox;
	class QComboBox;
	class QProgressBar;
	class QStackedWidget;
	class QLineEdit;
	class QScrollArea;
	class QScrollBar;
	class QPlainTextEdit;
	class QGraphicsView;
	class QSplitter;
	class QMenu;
	class QAction;
	class QTabBar;
	class QTableWidget;
	class QToolButton;
QT_END_NAMESPACE

#define NA_TEXT                "n/a"
#define MINUS_TEXT             "-"

class SofMainWindow : public QWidget, public Ui::SofMainWindowWnd
{
  Q_OBJECT

	public:
		SofMainWindow();
		~SofMainWindow();
		void init();
		void setGameText(QString, int);
		void setConsoleText(QString, int, bool);

	protected:
		QVector<QLabel*> statLabelsCaption; // Для хранения указателей на элементы QLabel окна статистики
		QVector<QLabel*> statLabelsValues;  // Для хранения указателей на элементы QLabel окна статистики
		QVector<QLabel*> footerStatLabels;  // Для хранения указателей на элементы QLabel footer-а
		QVector<int> footerStatSettings;    // Индексы массива статистики
		QVector<int> statisticFooterPos;    // Номер позиции элемента статистики в fotter-е
		QStringList statisticCaption;       // Заголовок элемента статистики
		QStringList statisticStartValue;    // Начальные значения для статистики
		int  maxEventSlotId;                // Максимальный текущий идентификатор слота
		int usedEventSlots;                 // Использовано слотов
		QVector<int> eventSlots;            // Cлужебные структуры слотов событий и уведомлений
		void setAutoEnterMode(bool);
		void closeEvent(QCloseEvent *evnt);
		void initStatisticData();           // Заполнение массивов статистики начальными значениями
		void fillSlotCombo(QComboBox* slotCombo);
		void setFooterStatisticValue(int statisticId, QString* valuePtr);
		void setStatisticValue(int statisticId, QString* valuePtr);
		void fullUpdateFooterStatistic();
		void setStatisticCaptionText();
		void getAllDataFromCore();
		void updateValue(int valueId, QString* valuePtr);
		void changePersStatus(int status);
		void initEventSlots();
		int  getEventSlot();
		bool freeEventSlot(int id);
		bool setEventValue(int id, QString* caption, QString* value);
		void setTimeout(int value);
		void scrollMapNewPosition(int x, int y);
		void scrollMapToPersPosition();
		void setCurrentHealth(int);
		void setCurrentEnergy(int);
		void showThingsSummary();

	private:
		int statMessagesCount;
		long statDropMoneys; // ???? Это откуда лонг???
		int statDamageMinFromPers;
		int statDamageMaxFromPers;
		int statFingsCount;
		int statFightsCount;
		int settingTimeOutDisplay;
		int selectedMapElement;
		QDateTime* timeoutStamp;
		QTimer* timeoutTimer;
		int timeoutEventSlot;
		int queueEventSlot;
		bool queueShowFlag;
		bool fingsChanged;
		QString statGameJid;
		QString statFingDropLast;
		int currentPage;
		QMenu* mapMenu;
		QMenu* fingsMenu;
		QAction* actionMarkMapElement;
		QAction* actionMoveMapElement;
		QAction* actionRemoveMapElement;
		QAction* actionSetFingPrice;
		QAction* actionFingsParamToConsole;
		QTabBar* fingsTabBar;
		QList<FontLabel*>fontLabelGroup;
		QButtonGroup* fontButtonGroup;
		ThingsModel* fingsTableModel;
		QList<FingFilter*> filtersList;
		bool autoEnterMode;
		int  thingsIface;

	public slots:
		void valueChanged(int eventId, int valueType, int value); // Приходят сообщения и событиях, часто об обновлении статистики.
		void updateFingFiltersTab(); // Обновляет список (имена) табов у вещей
		void showQueueLen(int); // Отображение очереди команд
		void chooseFont(QAbstractButton* button); // Нажата кнопка выбора фонта

	private slots:
		void changePage(int index);
		void activateMainPage();
		void activateFightPage();
		void activatePersInfoPage();
		void activateFingsPage();
		void activateStatPage();
		void activateSettingsPage();
		void resetCommonStatistic();
		void resetFightStatistic();
		void applySettings();
		void saveSettings();
		void resetGameJid();
		void userCommandChanged();
		void userCommandReturnPressed();
		void timeoutEvent();
		void saveMap();
		void createMap();
		void clearMap();
		void mapShowContextMenu(const QPoint &);
		void textShowContextMenu(const QPoint &);
		void consoleShowContextMenu(const QPoint &);
		void moveMapElement();
		void removeMapElement();
		void markMapElement();
		void showFings(int);
		void fingsShowContextMenu(const QPoint &);
		void setFingPrice();
		void fingParamToConsole();
		void persFingsChanged();
		void persParamChanged(int, int, int);

};

#endif
