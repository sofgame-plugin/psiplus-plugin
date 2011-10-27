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
		void setGameText(const QString &, int);
		void setConsoleText(const QString &, int, bool);
		bool getAutoEnterMode() const {return autoEnterMode;};
		void setAutoEnterMode(bool);
		QDomElement exportAppearanceSettings(QDomDocument &xmlDoc) const;
		QDomElement exportSlotsSettings(QDomDocument &xmlDoc) const;

	protected:
		struct StatWidgets {
			QLabel *caption;
			QLabel *value;
		};
		struct StatCapInitVal {
			QString caption;
			QString initVal;
		};
		QHash<int, StatWidgets> footerStatWidgets;
		QHash<int, int> statisticFooterPos;             // Номер позиции элемента статистики в footer-е
		QHash<int, StatCapInitVal> statisticCapInitVal; // Название элемента статистики и начальные значения
		QHash<int, StatWidgets> statisticWidgets;       // Для хранения указателей на элементы QLabel окна статистики
		int  maxEventSlotId;                // Максимальный текущий идентификатор слота
		int usedEventSlots;                 // Использовано слотов
		QVector<int> eventSlots;            // Cлужебные структуры слотов событий и уведомлений
		void closeEvent(QCloseEvent *evnt);
		void initStatisticData();           // Заполнение массивов статистики начальными значениями
		void fillSlotCombo(QComboBox* slotCombo);
		void setFooterStatisticValue(int statisticId, const QString &valString);
		void setStatisticValue(int statisticId, const QString &valString);
		void fullUpdateFooterStatistic();
		void setStatisticCaptionText();
		void getAllDataFromCore();
		void updateValue(int valueId, const QString &valString);
		void changePersStatus();
		void initEventSlots();
		int  getEventSlot();
		bool freeEventSlot(int id);
		bool setEventValue(int id, QString* caption, QString* value);
		void setTimeout(int value);
		void scrollMapNewPosition(const MapPos &pos);
		void scrollMapToPersPosition();
		void setCurrentHealth(int);
		void setCurrentEnergy(int);
		void setCurrentExperience(long long);
		void setMaximumExperience(long long);
		void showThingsSummary();
		void loadSlotsSettings(const QDomElement &xml);
		void loadAppearanceSettings(const QDomElement &xml);

	private:
		static QList< QPair<int, QString> > statisticXmlStrings;
		int settingTimeOutDisplay;
		int selectedMapElement;
		QDateTime* timeoutStamp;
		QTimer* timeoutTimer;
		int timeoutEventSlot;
		int queueEventSlot;
		bool queueShowFlag;
		bool thingsChanged;
		int currentPage;
		QMenu* mapMenu;
		QMenu* thingsMenu;
		QAction* actionMarkMapElement;
		QAction* actionMoveMapElement;
		QAction* actionRemoveMapElement;
		QAction* actionSetThingPrice;
		QAction* actionThingsParamToConsole;
		QTabBar* thingsTabBar;
		QList<FontLabel*>fontLabelGroup;
		QButtonGroup* fontButtonGroup;
		ThingsModel* thingsTableModel;
		QList<ThingFilter*> filtersList;
		bool autoEnterMode;
		int  thingsIface;
		QString lastMapForMoveElement; // Запоминается имя карты последнего переноса элемента
		long long experienceMax; // Нужно для расчета делителя в QProgressBar
		long long experienceCurr;
		int settingWindowSizePos;

	public slots:
		void valueChanged(int eventId, int valueType, int value); // Приходят сообщения и событиях, часто об обновлении статистики.
		void updateThingFiltersTab(); // Обновляет список (имена) табов у вещей
		void showQueueLen(int); // Отображение очереди команд
		void chooseFont(QAbstractButton* button); // Нажата кнопка выбора фонта

	private slots:
		void changePage(int index);
		void activateMainPage();
		void activateFightPage();
		void activatePersInfoPage();
		void activateThingsPage();
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
		void showThings(int);
		void thingsShowContextMenu(const QPoint &);
		void setThingPrice();
		void thingParamToConsole();
		void persThingsChanged();
		void persParamChanged(int, int, int);

};

#endif
