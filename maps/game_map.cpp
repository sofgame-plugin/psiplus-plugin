/*
 * game_map.cpp - Sof Game Psi plugin
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
#include <QGraphicsItem>
#include <QDomDocument>


#include "common.h"
#include "game_map.h"
#include "utils.h"

GameMap::GameMap(QObject *parent) : QGraphicsScene(parent),
	currAccJid(QString()),
	null_element_x(QINT32_MIN),
	null_element_y(QINT32_MIN),
	null_pers_pos_x(QINT32_MIN),
	null_pers_pos_y(QINT32_MIN),
	lastX(QINT32_MIN),
	lastY(QINT32_MIN),
	persPosX(QINT32_MIN),
	persPosY(QINT32_MIN),
	lastIndex(-1),
	persPosIndex(-1),
	mapCurrIndex(-1),
	mapCurrArrayPtr(NULL),
	persGraphicItem(NULL),
	modifiedMapsCount(false),
	saveMode(1),
	saveTimer(NULL),
	autoSaveInterval(0),
	unloadTimer(NULL),
	autoUnloadInterval(0)
{
}

GameMap::~GameMap()
{
	// Сохраняем карты
	if (saveMode > 0)
		saveMap();
	// Освобождаем память занятую под загруженные карты
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		if (mapsList[i].status == InMemory) {
			delete mapsList[i].map;
		}
	}
}

GameMap *GameMap::instace_ = NULL;

GameMap *GameMap::instance() {
	if (GameMap::instace_ == NULL) {
		GameMap::instace_ = new GameMap();
	}
	return GameMap::instace_;
}

void GameMap::reset() {
	if (GameMap::instace_ != NULL) {
		delete GameMap::instace_;
		GameMap::instace_ = NULL;
	}
}

void GameMap::init(QString acc_jid)
{
	if (!currAccJid.isEmpty() && saveMode > 0) {
		saveMap();
	}
	// Очищаем картинку
	clear();
	// Сохраняем новый джид
	currAccJid = acc_jid;
	// Инициализация переменных
	null_element_x = QINT32_MIN;
	null_element_y = QINT32_MIN;
	null_pers_pos_x = QINT32_MIN;
	null_pers_pos_y = QINT32_MIN;
	persPosX = QINT32_MIN;
	persPosY = QINT32_MIN;
	persGraphicItem = NULL;
	lastX = QINT32_MIN;
	lastY = QINT32_MIN;
	lastIndex = -1;
	persPosIndex = -1;
	mapCurrIndex = -1;
	mapCurrArrayPtr = NULL;
	otherPers.clear();
	// Загружаем список карт
	loadMapsList();
}

QGraphicsScene* GameMap::getGraphicsScene()
{
	return this;
}

bool GameMap::loadMapsList()
{
	/**
	* Загружает список карт с файла
	**/
	// Очищаем массив со старыми картами
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		if (mapsList[i].status == InMemory) {
			delete mapsList[i].map;
		}
	}
	mapsList.clear();
	// Загружаем из файла
	QDomDocument xmlDoc;
	if (!loadPluginXml(&xmlDoc, "sofgame_maps.xml")) {
		return false;
	}
	// Проверяем корневой элемент
	QDomElement eRoot = xmlDoc.documentElement();
	if (eRoot.tagName() != "maps") {
		return false;
	}
	bool oldVersion = false;
	QString sVer = eRoot.attribute("version");
	if (sVer == "0.1") {
		oldVersion = true; // Старый формат карты
	} else if (sVer != "0.2") {
		return false; // Неизвестный формат карт
	}
	QDomNode eMapNode;
	if (oldVersion) {
		eMapNode = eRoot.firstChild();
	} else {
		if (currAccJid.isEmpty())
			return false;
		QDomNode eAccNode = eRoot.firstChild();
		while (!eAccNode.isNull()) {
			if (eAccNode.toElement().tagName() == "account") {
				if (eAccNode.toElement().attribute("jid") == currAccJid) {
					eMapNode = eAccNode.firstChild();
					break;
				}
			}
			eAccNode = eAccNode.nextSibling();
		}
	}
	while (!eMapNode.isNull()) {
		QDomElement eMap = eMapNode.toElement();
		if (eMap.tagName() == "map") {
			QString sName = eMap.attribute("name").trimmed();
			if (!sName.isEmpty()) {
				// Добавляем карту в список карт
				struct maps_list map_head;
				map_head.status = HeaderOnly; // Карта существует но не загружена
				map_head.name = sName;
				map_head.old_name = "";
				bool flag;
				map_head.min_x = eMap.attribute("min-x").toInt(&flag);
				if (!flag) {
					map_head.min_x = QINT32_MIN;
				}
				map_head.max_x = eMap.attribute("max-x").toInt(&flag);
				if (!flag) {
					map_head.max_x = QINT32_MIN;
				}
				map_head.min_y = eMap.attribute("min-y").toInt(&flag);
				if (!flag) {
					map_head.min_y = QINT32_MIN;
				}
				map_head.max_y = eMap.attribute("max-y").toInt(&flag);
				if (!flag) {
					map_head.max_y = QINT32_MIN;
				}
				map_head.map = 0;
				map_head.modified = false;
				mapsList.push_back(map_head);
			}
		}
		eMapNode = eMapNode.nextSibling();
	}
	modifiedMapsCount = 0;
	initSaveTimer();
	return true;
}

int GameMap::createMap(QString* map_name)
{
	/**
	* Создает пустую карту с именем map_name
	* Возвращает индекс новой созданной карты или -1 в случае ошибки
	**/
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		if (mapsList[i].name == *map_name) {
			// Такое имя карты уже есть
			return -1;
		}
	}
	struct maps_list mapHeader;
	mapHeader.status = NewMap; // Отмечаем как новую
	mapHeader.name = *map_name;
	mapHeader.old_name = "";
	mapHeader.min_x = QINT32_MIN;
	mapHeader.max_x = QINT32_MIN;
	mapHeader.min_y = QINT32_MIN;
	mapHeader.max_y = QINT32_MIN;
	mapHeader.map = new QVector<struct GameMap::map_element>;
	mapHeader.modified = true;
	modifiedMapsCount++;
	mapHeader.last_access = QDateTime::currentDateTime();
	initSaveTimer();
	mapsList.push_back(mapHeader);
	int res = mapsList.size() - 1;
	return res;
}

bool GameMap::loadMap(int map_index)
{
	/**
	* Загрузка карты с индексом map_index из списка карт
	* Если карта уже загружена, то она удаляется из памяти и загружается заново
	**/
	if (map_index < 0 || map_index >= mapsList.size()) {
		return false;
	}
	QDomDocument xmlDoc;
	if (!loadPluginXml(&xmlDoc, "sofgame_maps.xml")) {
		return false;
	}
	QDomElement eRoot = xmlDoc.documentElement();
	if (eRoot.tagName() != "maps") {
		return false;
	}
	// Проверяем, загружена ли карта
	//if (mapsList[map_index].status == InMemory) {
	//	// Очищаем
	//	mapsList[map_index].map->clear();
	//} else {
	//	// Выделяем память под новую карту
	//	mapsList[map_index].map = new QVector<map_element>;
	//}
	QString sName = mapsList[map_index].old_name;
	if (sName.isEmpty())
		sName = mapsList[map_index].name;
	bool oldVersion = false;
	QString sVer = eRoot.attribute("version");
	if (sVer == "0.1") {
		oldVersion = true; // Старый формат карты
	} else if (sVer != "0.2") {
		return false; // Неизвестный формат карт
	}
	QDomNode eMapNode;
	if (oldVersion) {
		eMapNode = eRoot.firstChild();
	} else {
		if (currAccJid.isEmpty())
			return false;
		QDomNode eAccNode = eRoot.firstChild();
		while (!eAccNode.isNull()) {
			if (eAccNode.toElement().tagName() == "account") {
				if (eAccNode.toElement().attribute("jid") == currAccJid) {
					eMapNode = eAccNode.firstChild();
					break;
				}
			}
			eAccNode = eAccNode.nextSibling();
		}
	}
	while (!eMapNode.isNull()) {
		QDomElement eMap = eMapNode.toElement();
		if (eMap.tagName() == "map") {
			if (eMap.attribute("name") == sName) {
				break;
			}
		}
		eMapNode = eMapNode.nextSibling();
	}
	if (eMapNode.isNull()) {
		return false;
	}
	// Нашли нужную нам карту, теперь грузим ее в память
	makeMapFromDomNode(&mapsList[map_index], eMapNode);
	// Отмечаем карту как загруженную
	mapsList[map_index].status = InMemory;
	// Другие флаги
	mapsList[map_index].modified = false;
	mapsList[map_index].last_access = QDateTime::currentDateTime();
	return true;
}

bool GameMap::saveMap()
{
	if (currAccJid.isEmpty())
		return false;
	int mapsCnt = mapsList.size();
	// Сначала грузим XML файл карт
	QDomDocument xmlDoc;
	QDomElement eRoot;
	QDomElement eAcc;
	bool oldVer = false;
	if (loadPluginXml(&xmlDoc, "sofgame_maps.xml")) {
		// Документ загружен, проверяем корневой элемент
		eRoot = xmlDoc.documentElement();
		if (eRoot.tagName() == "maps") {
			// Проверяем версию карт
			QString sVer = eRoot.toElement().attribute("version");
			QDomNode eMap;
			if (sVer != "0.2") {
				oldVer = true;
				eMap = eRoot.firstChild();
			} else {
				// Карта нового формата, ищем наш jid
				QDomNode accNode = eRoot.firstChild();
				while (!accNode.isNull()) {
					if (accNode.toElement().tagName() == "account") {
						if (accNode.toElement().attribute("jid") == currAccJid) {
							// Получаем первый элемент карты
							eMap = accNode.firstChild();
							break;
						}
					}
					accNode = accNode.nextSibling();
				}
				if (!accNode.isNull()) {
					eAcc = accNode.toElement();
				} else {
					eAcc = xmlDoc.createElement("account");
					eAcc.setAttribute("jid", currAccJid);
					eRoot.appendChild(eAcc);
				}
			}
			// Читаем элементы карт
			while (!eMap.isNull()) {
				QDomNode nextMap = eMap.nextSibling();
				if (eMap.toElement().tagName() == "map") {
					bool fFound = false;
					QString domMapName = eMap.toElement().attribute("name");
					for (int i = 0; i < mapsCnt; i++) {
						QString sName = mapsList[i].old_name;
						if (sName.isEmpty())
							sName = mapsList[i].name;
						if (domMapName == sName) {
							MapStatus nStatus = mapsList[i].status;
							if (nStatus == InMemory) {
								// Эта карта загружена в память, формируем XML элемент
								QDomNode eNewMap = makeMapXmlElement(xmlDoc, &mapsList[i]);
								if (!eNewMap.isNull()) {
									if (oldVer) {
										eRoot.replaceChild(eNewMap, eMap);
									} else {
										eAcc.replaceChild(eNewMap, eMap);
									}
								}
								mapsList[i].modified = false;
								fFound = true;
								break;
							} else if (nStatus == HeaderOnly) {
								// Карта есть в списке, но не загружена в память
								fFound = true;
								break;
							}
						}
					}
					if (!fFound) {
						// Не нашли описание карты в памяти. Возможно была удалена
						if (oldVer) {
							eRoot.removeChild(eMap);
						} else {
							eAcc.removeChild(eMap);
						}
					}
				}
				eMap = nextMap;
			}
		} else {
			// Некорректный корневой элемент
			xmlDoc.removeChild(eRoot);
			eRoot = xmlDoc.createElement("maps");
			eRoot.setAttribute("version", "0.2");
			eAcc = xmlDoc.createElement("account");
			eAcc.setAttribute("jid", currAccJid);
			eRoot.appendChild(eAcc);
		}
	} else {
		// Не загрузился, создаем новый XML документ
		eRoot = xmlDoc.createElement("maps");
		eRoot.setAttribute("version", "0.2");
		eAcc = xmlDoc.createElement("account");
		eAcc.setAttribute("jid", currAccJid);
		eRoot.appendChild(eAcc);
	}
	// Переносим элементы, если карты старого образца
	if (oldVer) {
		eAcc = xmlDoc.createElement("account");
		eAcc.setAttribute("jid", currAccJid);
		while (true) {
			QDomNode mapNode = eRoot.firstChild();
			if (mapNode.isNull())
				break;
			eAcc.appendChild(mapNode);
		}
		eRoot.appendChild(eAcc);
		eRoot.setAttribute("version", "0.2");
	}
	// Добавляем новые карты
	for (int i = 0; i < mapsCnt; i++) {
		MapStatus nStatus = mapsList[i].status;
		if (nStatus == NewMap) {
			// Новая карта
			QDomNode eNewMap = makeMapXmlElement(xmlDoc, &mapsList[i]);
			if (!eNewMap.isNull()) {
				eAcc.appendChild(eNewMap);
			}
			mapsList[i].status = InMemory;
		}
		if (!mapsList[i].old_name.isEmpty())
			mapsList[i].old_name = "";
		mapsList[i].modified = false;
	}
	xmlDoc.appendChild(eRoot);
	// Сохраняем в файл
	if (savePluginXml(&xmlDoc, "sofgame_maps.xml")) {
		return true;
	}
	//--
	modifiedMapsCount = 0;
	initSaveTimer();
	initUnloadTimer(false);
	return false;
}

/**
 * Экспорт карты с индексом map_index, тип exp_type, имя файла exp_file
 */
int GameMap::exportMaps(QStringList maps_list, int exp_type, QString exp_file)
{
	if (maps_list.isEmpty() || exp_file.isEmpty())
		return 1;
	if (exp_type == 1) { // XML файл
		QDomDocument xmlDoc;
		QDomElement eRoot = xmlDoc.createElement("maps-export");
		eRoot.setAttribute("version", "0.1");
		xmlDoc.appendChild(eRoot);
		bool fAll = false;
		QList<int> maps_index;
		int cnt = maps_list.size();
		for (int i = 0; i < cnt; i++) {
			QString sMapNum = maps_list.at(i);
			if (sMapNum == "*") {
				fAll = true;
				break;
			}
			bool fOk;
			int nMapNum = sMapNum.toInt(&fOk);
			if (fOk)
				maps_index.push_back(nMapNum);
		}
		if (!fAll && maps_index.isEmpty())
			return 1;
		// Перебираем карты и сбрасываем нужные нам в xmlDoc
		int mapsCnt = mapsList.size();
		int expCnt = 0;
		for (int i = 0; i < mapsCnt; i++) {
			if (!fAll) {
				// Проверяем есть ли карта в запросе на экспорт
				if (maps_index.indexOf(i) == -1)
					continue;
			}
			MapStatus status = mapsList.at(i).status;
			if (status == HeaderOnly || status == InMemory || status == NewMap) {
				if (status == HeaderOnly) {
					// Загружаем карту в память
					if (!loadMap(i))
						continue;
				}
				// Формируем DOM элемент карты
				QDomNode mapElement = makeMapXmlElement(xmlDoc, &mapsList[i]);
				if (!mapElement.isNull()) {
					eRoot.appendChild(mapElement);
					expCnt++;
				}
				mapsList[i].last_access = QDateTime::currentDateTime();
			}
		}
		if (expCnt > 0) {
			// Сохраняем количество карт
			eRoot.setAttribute("maps-count", QString::number(expCnt));
			// Сохраняем в файл
			if (saveXmlToFile(&xmlDoc, exp_file)) {
				return 0;
			}
			return 3;
		}
		return 2;
	}
	return 1;
}

/**
 * Импорт карты из XML файла
 */
int GameMap::importMaps(QString imp_file)
{
	QDomDocument xmlDoc;
	// Грузим из файла
	if (!loadXmlFromFile(&xmlDoc, imp_file))
		return 1;
	// Проверяем корневой элемент
	QDomElement eRoot = xmlDoc.documentElement();
	if (eRoot.tagName() != "maps-export")
		return 2;
	// Проверяем версию
	if (eRoot.attribute("version") != "0.1")
		return 2;
	// Запускаем цикл для просмотра всех дочерних элементов
	QDomNode eRootChildNode = eRoot.firstChild();
	int mapsCnt = mapsList.size();
	int expMapsCnt = 0;
	while (!eRootChildNode.isNull()) {
		if (eRootChildNode.toElement().tagName() == "map") {
			// Получаем карту из DOM ноды
			struct maps_list ml;
			ml.map = 0;
			if (makeMapFromDomNode(&ml, eRootChildNode)) {
				QString mapName = ml.name;
				expMapsCnt++;
				// Проверяем наличие карты с таким же именем
				for (int i = 0; i < mapsCnt; i++) {
					if (mapsList.at(i).status != None && mapsList.at(i).name == mapName) {
						// Есть карта с таким именем, модифицируем имя импортируемой карты
						mapName.append("_imp" + QString::number(QDateTime::currentDateTime().toTime_t()));
						mapName.append("_" + QString::number(expMapsCnt));
						ml.name = mapName;
					}
				}
				// Отмечаем карту как новую
				ml.status = NewMap;
				// Карта модифицирована
				ml.modified = true;
				modifiedMapsCount++;
				ml.last_access = QDateTime::currentDateTime();
				initSaveTimer();
				// Добавляем карту в список карт
				mapsList.push_back(ml);
			}
		}
		eRootChildNode = eRootChildNode.nextSibling();
	}
	return 0;
}

/**
 * Функция удаляет карту.
 */
bool GameMap::removeMap(int map_index)
{
	if (map_index >= 0 && map_index < mapsList.size()) {
		if (mapsList[map_index].status == InMemory) {
			unloadMap(map_index);
		}
		mapsList[map_index].status = None;
		modifiedMapsCount++;
		initSaveTimer();
		return true;
	}
	return false;
}

/**
 * Функция объединяет две карты, причем первая карта используется как базовая
 */
bool GameMap::mergeMaps(int map1_index, int map2_index)
{
	// Проверяем индексы карт
	if (map1_index < 0 || map2_index < 0 || map1_index == map2_index)
		return false;
	int mapsCnt = mapsList.size();
	if (map1_index >= mapsCnt || map2_index >= mapsCnt)
		return false;
	// Загружаем карты, если не загружены
	struct maps_list* ml1 = &mapsList[map1_index];
	struct maps_list* ml2 = &mapsList[map2_index];
	if (ml1->status == HeaderOnly)
		loadMap(map1_index);
	if (ml2->status == HeaderOnly)
		loadMap(map2_index);
	// Проверяем готовность карт
	if ((ml1->status != InMemory && ml1->status != NewMap) || (ml2->status != InMemory  && ml2->status != NewMap))
		return false;
	// Начинаем сканирование и объединение карт
	qint32 nMinX = ml1->min_x;
	qint32 nMaxX = ml1->max_x;
	qint32 nMinY = ml1->min_y;
	qint32 nMaxY = ml1->max_y;
	QVector<struct GameMap::map_element>* mel1 = ml1->map;
	QVector<struct GameMap::map_element>* mel2 = ml2->map;
	int cnt = mel2->size();
	for (int i = 0; i < cnt; i++) {
		const struct map_element* me2 = &mel2->at(i);
		if (me2->status == 1) {
			qint32 x = me2->x;
			qint32 y = me2->y;
			bool fNew = true;
			struct map_element* me1 = 0;
			int me1_index = getMapElementIndex(map1_index, x, y);
			if (me1_index != -1) {
				me1 = &(*mel1)[me1_index];
				if (me1->status == 1)
					fNew = false;
			}
			if (nMinX == QINT32_MIN || x < nMinX) {
				nMinX = x;
			}
			if (nMaxX == QINT32_MIN || x > nMaxX) {
				nMaxX = x;
			}
			if (nMinY == QINT32_MIN || y < nMinY) {
				nMinY = y;
			}
			if (nMaxY == QINT32_MIN || y > nMaxY) {
				nMaxY = y;
			}
			if (fNew) {
				// Нет такого элемента. Добавляем новый
				struct map_element me_new = *me2;
				mel1->push_back(me_new);
			} else {
				// Есть такой элемент в базовой карте
				if (me1->type == GameMap::TypeNormal)
					me1->type = me2->type;
				if ((me1->north_type == 0 && me2->north_type != 0) || (me1->north_type == 1 && me2->north_type == 2)) {
					me1->north_type = me2->north_type;
					me1->can_north = me2->can_north;
				}
				if ((me1->south_type == 0 && me2->south_type != 0) || (me1->south_type == 1 && me2->south_type == 2)) {
					me1->south_type = me2->south_type;
					me1->can_south = me2->can_south;
				}
				if ((me1->west_type == 0 && me2->west_type != 0) || (me1->west_type == 1 && me2->west_type == 2)) {
					me1->west_type = me2->west_type;
					me1->can_west = me2->can_west;
				}
				if ((me1->east_type == 0 && me2->east_type != 0) || (me1->east_type == 1 && me2->east_type == 2)) {
					me1->east_type = me2->east_type;
					me1->can_east = me2->can_east;
				}
				if (me1->enemies_min > me2->enemies_min)
					me1->enemies_min = me2->enemies_min;
				if (me1->enemies_max < me2->enemies_max)
					me1->enemies_max = me2->enemies_max;
				me1->enemies_list = me2->enemies_list;
				if (me2->mark.enabled) {
					if (me1->mark.enabled) { // Уже есть метка на базовой карте
						if (me1->mark.title.isEmpty()) // Заголовок метки меняем только если он пустой
							me1->mark.title = me2->mark.title;
					} else {
						me1->mark.title = me2->mark.title;
						me1->mark.color = me2->mark.color;
						me1->mark.enabled = true;
					}
				}
			}
		}
	}
	ml1->min_x = nMinX;
	ml1->max_x = nMaxX;
	ml1->min_y = nMinY;
	ml1->max_y = nMaxY;
	// Удаляем вторую карту
	removeMap(map2_index);
	// Обновляем время последнего доступа
	ml1->last_access = QDateTime::currentDateTime();
	// Отмечаем первую карту как модифицированную
	if (!ml1->modified) {
		ml1->modified = true;
		modifiedMapsCount++;
		initSaveTimer();
	}
	// Перерисовываем карту, если активна базовая карта
	if (map1_index == mapCurrIndex) {
		redrawMap();
		setSceneRect(getMapRect());
	}
	return true;
}

QDomNode GameMap::makeMapXmlElement(QDomDocument xmlDoc, struct maps_list* map_head)
{
	// Создаем элемент с нашей картой
	QDomElement eMap = xmlDoc.createElement("map");
	eMap.setAttribute("name", map_head->name);
	qint32 nPos = map_head->min_x;
	if (nPos != QINT32_MIN) {
		eMap.setAttribute("min-x", nPos);
	}
	nPos = map_head->max_x;
	if (nPos != QINT32_MIN) {
		eMap.setAttribute("max-x", nPos);
	}
	nPos = map_head->min_y;
	if (nPos != QINT32_MIN) {
		eMap.setAttribute("min-y", nPos);
	}
	nPos = map_head->max_y;
	if (nPos != QINT32_MIN) {
		eMap.setAttribute("max-y", nPos);
	}
	// Создаем временный указатель
	QVector<struct GameMap::map_element>* mapPtr = map_head->map;
	if (mapPtr) {
		int map_size = mapPtr->size();
		// Выгружаем элементы массива
		QDomElement eMapItems = xmlDoc.createElement("map-items");
		eMap.appendChild(eMapItems);
		for (int mapIdx = 0; mapIdx < map_size; mapIdx++) {
			map_element *me = &(*mapPtr)[mapIdx];
			if (me->status == 1) { // Только если элемент карты в памяти
				QDomElement eMapItem = xmlDoc.createElement("map-item");
				// Координаты карты
				eMapItem.setAttribute("pos-x", QString::number(me->x));
				eMapItem.setAttribute("pos-y", QString::number(me->y));
				// Отметка на карте
				if (me->mark.enabled) {
					QDomElement eMapItemMark = xmlDoc.createElement("mark");
					eMapItem.appendChild(eMapItemMark);
					if (!me->mark.title.isEmpty())
						eMapItemMark.setAttribute("title", me->mark.title);
					eMapItemMark.setAttribute("color", me->mark.color.name());
				}
				// Порталы, секреты и др.
				int nType = me->type;
				if (nType == GameMap::TypePortal) {
					QDomElement eMapItemPortal = xmlDoc.createElement("portal");
					eMapItem.appendChild(eMapItemPortal);
				} else if (nType == GameMap::TypeSecret) {
					QDomElement eMapItemSecret = xmlDoc.createElement("secret");
					eMapItem.appendChild(eMapItemSecret);
				}
				// Описание врагов
				if (me->enemies_max != 0) {
					QDomElement eMapItemEnemies = xmlDoc.createElement("enemies");
					eMapItemEnemies.setAttribute("enemies-min", QString::number(me->enemies_min));
					eMapItemEnemies.setAttribute("enemies-max", QString::number(me->enemies_max));
					eMapItem.appendChild(eMapItemEnemies);
					for (int i = 0; i < me->enemies_list.size(); i++) {
						QDomElement eMapItemEnemy = xmlDoc.createElement("enemy");
						eMapItemEnemy.setAttribute("name", me->enemies_list.at(i));
						eMapItemEnemies.appendChild(eMapItemEnemy);
					}
				}
				// Возможные пути перемещения
				if (me->north_type != 1 || me->can_north != 0 || me->south_type != 1 || me->can_south != 0 || me->west_type != 1 || me->can_west != 0 || me->east_type != 1 || me->can_east != 0) {
					QDomElement eMapItemPaths = xmlDoc.createElement("paths");
					eMapItem.appendChild(eMapItemPaths);
					// Север
					int nType = me->north_type;
					int nCan = me->can_north;
					if (nType != 1 || nCan != 0) {
						QDomElement eMapItemPathNorth = xmlDoc.createElement("north");
						QString str1 = "";
						if (nType == 2) {
							str1 = "variable";
						} else if (nType == 0) {
							str1 = "not-set";
						}
						if (!str1.isEmpty()) {
							eMapItemPathNorth.setAttribute("type", str1);
						}
						str1 = "";
						if (nCan == 0) {
							str1 = "false";
						} else if (nCan == 1) {
							str1 = "true";
						} else if (nCan == 2) {
							str1 = "unsure-true";
						}
						if (!str1.isEmpty()) {
							eMapItemPathNorth.setAttribute("can-move", str1);
						}
						eMapItemPaths.appendChild(eMapItemPathNorth);
					}
					// Юг
					nType = me->south_type;
					nCan = me->can_south;
					if (nType != 1 || nCan != 0) {
						QDomElement eMapItemPathSouth = xmlDoc.createElement("south");
						QString str1 = "";
						if (nType == 2) {
							str1 = "variable";
						} else if (nType == 0) {
							str1 = "not-set";
						}
						if (!str1.isEmpty()) {
							eMapItemPathSouth.setAttribute("type", str1);
						}
						str1 = "";
						if (nCan == 0) {
							str1 = "false";
						} else if (nCan == 1) {
							str1 = "true";
						} else if (nCan == 2) {
							str1 = "unsure-true";
						}
						if (!str1.isEmpty()) {
							eMapItemPathSouth.setAttribute("can-move", str1);
						}
						eMapItemPaths.appendChild(eMapItemPathSouth);
					}
					// Запад
					nType = me->west_type;
					nCan = me->can_west;
					if (nType != 1 || nCan != 0) {
						QDomElement eMapItemPathWest = xmlDoc.createElement("west");
						QString str1 = "";
						if (nType == 2) {
							str1 = "variable";
						} else if (nType == 0) {
							str1 = "not-set";
						}
						if (!str1.isEmpty()) {
							eMapItemPathWest.setAttribute("type", str1);
						}
						str1 = "";
						if (nCan == 0) {
							str1 = "false";
						} else if (nCan == 1) {
							str1 = "true";
						} else if (nCan == 2) {
							str1 = "unsure-true";
						}
						if (!str1.isEmpty()) {
							eMapItemPathWest.setAttribute("can-move", str1);
						}
						eMapItemPaths.appendChild(eMapItemPathWest);
					}
					// Восток
					nType = me->east_type;
					nCan = me->can_east;
					if (nType != 1 || nCan != 0) {
						QDomElement eMapItemPathEast = xmlDoc.createElement("east");
						QString str1 = "";
						if (nType == 2) {
							str1 = "variable";
						} else if (nType == 0) {
							str1 = "not-set";
						}
						if (!str1.isEmpty()) {
							eMapItemPathEast.setAttribute("type", str1);
						}
						str1 = "";
						if (nCan == 0) {
							str1 = "false";
						} else if (nCan == 1) {
							str1 = "true";
						} else if (nCan == 2) {
							str1 = "unsure-true";
						}
						if (!str1.isEmpty()) {
							eMapItemPathEast.setAttribute("can-move", str1);
						}
						eMapItemPaths.appendChild(eMapItemPathEast);
					}
				}
				eMapItems.appendChild(eMapItem);
			}
		}
	}
	return eMap;
}

bool GameMap::makeMapFromDomNode(struct maps_list* mapHeaderPtr, QDomNode mapNode)
{
	QString mapName = mapNode.toElement().attribute("name").trimmed();
	if (mapName.isEmpty())
		return false;
	QVector<struct GameMap::map_element>* mapPtr = mapHeaderPtr->map;
	if (mapPtr) {
		if (!mapPtr->isEmpty())
			mapPtr->remove(0, mapPtr->size());
		//mapPtr->clear(); //Достали варнинги у QT (!!! Проверить на новых версиях!!!)
	} else {
		mapPtr = new QVector<struct map_element>;
	}
	mapHeaderPtr->status = InMemory;
	mapHeaderPtr->name = mapName;
	mapHeaderPtr->old_name = "";
	qint32 nMinX = QINT32_MIN;
	qint32 nMaxX = QINT32_MIN;
	qint32 nMinY = QINT32_MIN;
	qint32 nMaxY = QINT32_MIN;
	QDomElement eMapItems = mapNode.firstChildElement();
	while (!eMapItems.isNull()) {
		if (eMapItems.tagName() == "map-items") {
			QDomElement eMapItem = eMapItems.firstChildElement();
			while (!eMapItem.isNull()) {
				if (eMapItem.tagName() == "map-item") {
					QString str1 = eMapItem.attribute("pos-x");
					bool bFlag;
					int nPosX = str1.toInt(&bFlag);
					if (bFlag) {
						str1 = eMapItem.attribute("pos-y");
						int nPosY = str1.toInt(&bFlag);
						if (bFlag) {
							struct map_element map_el;
							map_el.status = 1;
							map_el.type = GameMap::TypeNormal;
							map_el.x = nPosX;
							map_el.y = nPosY;
							if (nMinX == QINT32_MIN || nPosX < nMinX) {
								nMinX = nPosX;
							}
							if (nMaxX == QINT32_MIN || nPosX > nMaxX) {
								nMaxX = nPosX;
							}
							if (nMinY == QINT32_MIN || nPosY < nMinY) {
								nMinY = nPosY;
							}
							if (nMaxY == QINT32_MIN || nPosY > nMaxY) {
								nMaxY = nPosY;
							}
							map_el.can_north = 0; map_el.can_south = 0; map_el.can_west = 0; map_el.can_east = 0;
							map_el.north_type = 1; map_el.south_type = 1; map_el.west_type = 1; map_el.east_type = 1;
							map_el.past_pers_pos = -1;
							map_el.enemies_min = -1;
							map_el.enemies_max = 0;
							map_el.mark.enabled = false;
							if (eMapItem.attribute("marked") == "true") { // Атрибут устарел!!!
								// Блок оставлен для совместимости со старым форматом
								// Через пару версий необходимо убрать. Правильный код будет ниже.
								// Изменено в версии 0.1.16
								map_el.mark.enabled = true;
								map_el.mark.title = QString();
								map_el.mark.color = QColor(Qt::blue);
							}
							QDomElement eMapItemChild = eMapItem.firstChildElement();
							while (!eMapItemChild.isNull()) {
								QString sTagName = eMapItemChild.tagName();
								if (sTagName == "paths") {
									QDomElement eMapItemPaths = eMapItemChild.firstChildElement();
									while (!eMapItemPaths.isNull()) {
										QString sTagName = eMapItemPaths.tagName();
										if (sTagName == "north") {
											QString sAttrib = eMapItemPaths.attribute("can-move");
											if (!sAttrib.isEmpty()) {
												if (sAttrib == "true") {
													map_el.can_north = 1;
												} else if (sAttrib == "unsure-true") {
													map_el.can_north = 2;
												}
											}
											sAttrib = eMapItemPaths.attribute("type");
											if (!sAttrib.isEmpty()) {
												if (sAttrib == "variable") {
													map_el.north_type = 2;
												} else if (sAttrib == "not-set") {
													map_el.north_type = 0;
												}
											}
										} else if (sTagName == "south") {
											QString sAttrib = eMapItemPaths.attribute("can-move");
											if (!sAttrib.isEmpty()) {
												if (sAttrib == "true") {
													map_el.can_south = 1;
												} else if (sAttrib == "unsure-true") {
													map_el.can_south = 2;
												}
											}
											sAttrib = eMapItemPaths.attribute("type");
											if (!sAttrib.isEmpty()) {
												if (sAttrib == "variable") {
													map_el.south_type = 2;
												} else if (sAttrib == "not-set") {
													map_el.south_type = 0;
												}
											}
										} else if (sTagName == "west") {
											QString sAttrib = eMapItemPaths.attribute("can-move");
											if (!sAttrib.isEmpty()) {
												if (sAttrib == "true") {
													map_el.can_west = 1;
												} else if (sAttrib == "unsure-true") {
													map_el.can_west = 2;
												}
											}
											sAttrib = eMapItemPaths.attribute("type");
											if (!sAttrib.isEmpty()) {
												if (sAttrib == "variable") {
													map_el.west_type = 2;
												} else if (sAttrib == "not-set") {
													map_el.west_type = 0;
												}
											}
										} else if (sTagName == "east") {
											QString sAttrib = eMapItemPaths.attribute("can-move");
											if (!sAttrib.isEmpty()) {
												if (sAttrib == "true") {
													map_el.can_east = 1;
												} else if (sAttrib == "unsure-true") {
													map_el.can_east = 2;
												}
											}
											sAttrib = eMapItemPaths.attribute("type");
											if (!sAttrib.isEmpty()) {
												if (sAttrib == "variable") {
													map_el.east_type = 2;
												} else if (sAttrib == "not-set") {
													map_el.east_type = 0;
												}
											}
										}
										eMapItemPaths = eMapItemPaths.nextSiblingElement();
									}
								} else if (sTagName == "enemies") {
									QString str1 = eMapItemChild.attribute("enemies-min");
									map_el.enemies_min = str1.toInt();
									str1 = eMapItemChild.attribute("enemies-max");
									map_el.enemies_max = str1.toInt();
									if (map_el.enemies_min < 0 || map_el.enemies_min > map_el.enemies_max) {
										map_el.enemies_min = -1;
										map_el.enemies_min = 0;
									}
									if (map_el.enemies_min > 0) {
										QDomNode eEnemy = eMapItemChild.firstChild();
										while (!eEnemy.isNull()) {
											if (eEnemy.toElement().tagName() == "enemy") {
												QString str1 = eEnemy.toElement().attribute("name");
												if (!str1.isEmpty()) {
													if (map_el.enemies_list.indexOf(str1) == -1) {
														map_el.enemies_list.push_back(str1);
													}
												}
											}
											eEnemy = eEnemy.nextSibling();
										}
										map_el.enemies_list.sort();
									}
								} else if (sTagName == "portal") {
									map_el.type = GameMap::TypePortal;
								} else if (sTagName == "secret") {
									map_el.type = GameMap::TypeSecret;
								} else if (sTagName == "mark") {
									map_el.mark.enabled = true;
									map_el.mark.title = eMapItemChild.attribute("title");
									map_el.mark.color = QColor(eMapItemChild.attribute("color"));
								}
								eMapItemChild = eMapItemChild.nextSiblingElement();
							}



							mapPtr->push_back(map_el);
						}
					}
				}
				eMapItem = eMapItem.nextSiblingElement();
			}
		}
		eMapItems = eMapItems.nextSiblingElement();
	}
	mapHeaderPtr->min_x = nMinX;
	mapHeaderPtr->max_x = nMaxX;
	mapHeaderPtr->min_y = nMinY;
	mapHeaderPtr->max_y = nMaxY;
	mapHeaderPtr->map = mapPtr;
	return true;
}

void GameMap::selectMap(qint32 x, qint32 y)
{
	/**
	* Выбор и загрузка подходящей карты для указанной точки
	**/
	// Сначала проверяем текущую карту
	if (mapCurrIndex != -1) {
		int nMinX = mapsList[mapCurrIndex].min_x;
		int nMaxX = mapsList[mapCurrIndex].max_x;
		if (nMinX != QINT32_MIN && nMaxX != QINT32_MIN) {
			int nMinY = mapsList[mapCurrIndex].min_y;
			int nMaxY = mapsList[mapCurrIndex].max_y;
			if (nMinY != QINT32_MIN && nMaxY != QINT32_MIN) {
				if (nMinX <= x && nMaxX >= x && nMinY <= y && nMaxY >= y) {
					// Текущая карта нам вполне подходит
					return;
				}
			}
		}
	}
	// Сканируем имеющиеся карты
	int mapDefIndex = -1;
	int mapNearIndex = -1;
	int mapGoodIndex = -1;
	int mapsCnt = mapsList.size();
	for (int i = 0; i < mapsCnt; i++) {
		if (mapsList[i].status != None) {
			if (mapDefIndex == -1 && mapsList[i].name == "default") {
				mapDefIndex = i;
			}
			if (mapsList[i].min_x <= x && mapsList[i].max_x >= x && mapsList[i].min_y <= y && mapsList[i].max_y >= y) {
				mapGoodIndex = i;
				break;
			} else {
				if (mapsList[i].min_x - 10 <= x && mapsList[i].max_x + 10 >= x && mapsList[i].min_y - 10 <= y && mapsList[i].max_y + 10 >= y) {
					if (mapNearIndex == -1) {
						mapNearIndex = i;
					}
				}
			}
		}
	}
	// Пытаемся подгрузить одну из найденных карт
	int selMap = -1;
	for (int i = 1; i <= 3; i++) {
		if (i == 1) {
			selMap = mapGoodIndex;
		} else if (i == 2) {
			selMap = mapNearIndex;
		} else {
			selMap = mapDefIndex;
			if (mapDefIndex == -1) {
				// Создаем пустую карту default
				QString mn = "default";
				selMap = createMap(&mn);
				break;
			}
		}
		if (selMap != -1)
			break;
		selMap = -1;
	}
	if (selMap == -1) {
		// Смены карты не произошло
		return;
	}
	// Переключаем карту
	switchMap(selMap);
}

int GameMap::getIndexByCoordinate(qreal x, qreal y)
{
	/**
	* Возвращает индекс элемента карты по координатам графической сцены для текущей карты
	* Возвращает -1 в случае отсутствия
	**/
	if (null_element_x == QINT32_MIN) {
		return -1;
	}
	if (x < 0) x -= (qreal)MAP_ELEMENT_SIZE;
	if (y >= 0) y += (qreal)MAP_ELEMENT_SIZE;
	int mapX = x / (qreal)MAP_ELEMENT_SIZE;
	mapX += null_element_x;
	int mapY = y / (qreal)MAP_ELEMENT_SIZE;
	mapY = null_element_y - mapY + 1;
	int i = getMapElementIndex(-1, mapX, mapY);
	return i;
}

void GameMap::moveMapElement(int souMapIndex, int desMapIndex, int elementIndex)
{
	/**
	* Переносит элемент карты из одной карты в другую
	**/
	if (souMapIndex < 0 || souMapIndex >= mapsList.size())
		return;
	if (desMapIndex < 0 || desMapIndex >= mapsList.size())
		return;
	if (elementIndex < 0)
		return;
	MapStatus status = mapsList[souMapIndex].status;
	if (status == None)
		return;
	// Загружаем карту источник, если не загружена
	if (status == HeaderOnly) {
		if (!loadMap(souMapIndex))
			return;
	}
	// Проверяем корректность индекса элемента
	if (elementIndex >= mapsList[souMapIndex].map->size())
		return;
	struct map_element mapEl = (*mapsList[souMapIndex].map)[elementIndex];
	if (mapEl.status == 0)
		return;
	// Ищем такой же элемент в карте получателе
	int elX = mapEl.x;
	int elY = mapEl.y;
	int desElIndex = getMapElementIndex(desMapIndex, elX, elY);
	if (desElIndex != -1) {
		// Заменяем элемент
		(*mapsList[desMapIndex].map)[desElIndex] = mapEl;
	} else {
		// Добавляем элемент
		mapsList[desMapIndex].map->push_back(mapEl);
		if (mapsList[desMapIndex].min_x == QINT32_MIN || mapsList[desMapIndex].min_x > elX)
			mapsList[desMapIndex].min_x = elX;
		if (mapsList[desMapIndex].max_x == QINT32_MIN || mapsList[desMapIndex].max_x < elX)
			mapsList[desMapIndex].max_x = elX;
		if (mapsList[desMapIndex].min_y == QINT32_MIN || mapsList[desMapIndex].min_y > elY)
			mapsList[desMapIndex].min_y = elY;
		if (mapsList[desMapIndex].max_y == QINT32_MIN || mapsList[desMapIndex].max_y < elY)
			mapsList[desMapIndex].max_y = elY;
	}
	// Удаляем элемент из источника
	removeMapElement(souMapIndex, elementIndex);
	// Отмечаем карты как модифицированные
	if (!mapsList[souMapIndex].modified) {
		mapsList[souMapIndex].modified = true;
		modifiedMapsCount++;
		initSaveTimer();
	}
	if (!mapsList[desMapIndex].modified) {
		mapsList[desMapIndex].modified = true;
		modifiedMapsCount++;
		initSaveTimer();
	}
	QDateTime curr_time = QDateTime::currentDateTime();
	mapsList[souMapIndex].last_access = curr_time;
	mapsList[desMapIndex].last_access = curr_time;
	// Проверяем необходимость перерисовки карты
	if (souMapIndex == mapCurrIndex || desMapIndex == mapCurrIndex)
		redrawMap();
}

/**
 * Удаляет элемент карты
 */
void GameMap::removeMapElement(int mapIndex, int elementIndex)
{
	struct map_element mapEl = (*mapsList[mapIndex].map)[elementIndex];
	int elX = mapEl.x;
	int elY = mapEl.y;
	// Отмечаем элемент как не используемый
	(*mapsList[mapIndex].map)[elementIndex].status = 0;
	if (mapsList[mapIndex].min_x >= elX || mapsList[mapIndex].max_x <= elX || mapsList[mapIndex].min_y >= elY || mapsList[mapIndex].max_y <= elY) {
		// Пересчитываем границы карты источника
		QVector<map_element>* mapPtr = mapsList[mapIndex].map;
		int cnt = mapPtr->size();
		int minX = QINT32_MIN;
		int maxX = QINT32_MIN;
		int minY = QINT32_MIN;
		int maxY = QINT32_MIN;
		for (int i = 0; i < cnt; i++) {
			if ((*mapPtr)[i].status != 0) {
				if (minX == QINT32_MIN || (*mapPtr)[i].x < minX)
					minX = (*mapPtr)[i].x;
				if (maxX == QINT32_MIN || (*mapPtr)[i].x > maxX)
					maxX = (*mapPtr)[i].x;
				if (minY == QINT32_MIN || (*mapPtr)[i].y < minY)
					minY = (*mapPtr)[i].y;
				if (maxY == QINT32_MIN || (*mapPtr)[i].y > maxY)
					maxY = (*mapPtr)[i].y;
			}
		}
		mapsList[mapIndex].min_x = minX;
		mapsList[mapIndex].max_x = maxX;
		mapsList[mapIndex].min_y = minY;
		mapsList[mapIndex].max_y = maxY;
	}
	// Очищаем кэши индекса карт, т.к. вероятно, что наш перемещенный элемент сидит в кэше
	if (mapIndex == mapCurrIndex) {
		lastX = QINT32_MIN;
		lastY = QINT32_MIN;
	}
	// Меняем время доступа
	mapsList[mapIndex].last_access = QDateTime::currentDateTime();
	// Отмечаем карту как модифицированную
	if (!mapsList[mapIndex].modified) {
		mapsList[mapIndex].modified = true;
		modifiedMapsCount++;
		initSaveTimer();
	}
	// Проверяем необходимость перерисовки карты
	if (mapIndex == mapCurrIndex)
		redrawMap();
}

/**
 * Возвращает информацию о метке на элементе карты
 */
GameMap::MapElementMark GameMap::getMapElementMark(int elementIndex) const
{
	if (mapCurrArrayPtr == NULL || elementIndex < 0 || elementIndex >= mapCurrArrayPtr->size() || (*mapCurrArrayPtr)[elementIndex].status != 1) {
		MapElementMark mark;
		mark.enabled = false;
		mark.color = QColor(Qt::blue);
		return mark;
	}
	return (*mapCurrArrayPtr)[elementIndex].mark;
}

/**
 * Устанавливает отметку элемента карты
 */
void GameMap::setMapElementMark(int elementIndex, const QString &title, const QColor &c)
{
	if (mapCurrArrayPtr) {
		if (elementIndex >= 0 && elementIndex < mapCurrArrayPtr->size()) {
			map_element *me = &(*mapCurrArrayPtr)[elementIndex];
			if (me->status == 1 && (!me->mark.enabled || me->mark.title != title || me->mark.color != c)) {
				me->mark.enabled = true;
				me->mark.title = title;
				me->mark.color = c;
				mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
				if (!mapsList[mapCurrIndex].modified) {
					mapsList[mapCurrIndex].modified = true;
					modifiedMapsCount++;
					initSaveTimer();
				}
				drawMapElement(elementIndex, true);
				setTooltipForMapElement(elementIndex, makeTooltipForMapElement(elementIndex));
			}
		}
	}
}

/**
 * Снимает отметку элемента карты
 */
void GameMap::removeMapElementMark(int elementIndex)
{
	if (mapCurrArrayPtr) {
		if (elementIndex >= 0 && elementIndex < mapCurrArrayPtr->size()) {
			map_element *me = &(*mapCurrArrayPtr)[elementIndex];
			if (me->status == 1 && me->mark.enabled) {
				me->mark.enabled = false;
				mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
				if (!mapsList[mapCurrIndex].modified) {
					mapsList[mapCurrIndex].modified = true;
					modifiedMapsCount++;
					initSaveTimer();
				}
				drawMapElement(elementIndex, true);
				setTooltipForMapElement(elementIndex, makeTooltipForMapElement(elementIndex));
			}
		}
	}
}

void GameMap::setOtherPersPos(QVector<maps_other_pers>* other_pers_pos)
{
	/**
	* Устанавливает на карте позиции игроков из массива
	**/
	if (mapCurrIndex == -1)
		return;
	// Очистка карты и старых координат игроков
	clearOtherPersPos();
	// Начинаем прорисовку
	if (other_pers_pos && persPosX != QINT32_MIN && persPosY != QINT32_MIN) {
		int cnt = other_pers_pos->size();
		for (int i = 0; i < cnt; i++) {
			int x = (*other_pers_pos)[i].offset_x;
			int y = (*other_pers_pos)[i].offset_y;
			int cnt2 = otherPers.size();
			int j;
			for (j = 0; j < cnt2; j++) {
				if (otherPers[j].x == x && otherPers[j].y == y)
					break;
			}
			if (j >= cnt2) {
				struct other_pers op;
				op.x = x;
				op.y = y;
				otherPers.push_back(op);
				j = otherPers.size() - 1;
			}
			otherPers[j].names.push_back((*other_pers_pos)[i].name);
			drawOtherPersPos(j);
		}
		setSceneRect(getMapRect());
	}
	mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
}

void GameMap::redrawMap()
{
	/**
	* Стирает карту с графической сцены и перерисовывает заново
	**/
	// Очистка сцены
	clear();
	update();
	null_element_x = QINT32_MIN;
	null_element_y = QINT32_MIN;
	null_pers_pos_x = QINT32_MIN;
	null_pers_pos_y = QINT32_MIN;
	persGraphicItem = NULL;
	// Прорисовываем загруженную карту
	if (mapCurrIndex != -1) {
		int cnt = mapCurrArrayPtr->size();
		for (int i = 0; i < cnt; i++) {
			if ((*mapCurrArrayPtr)[i].status != 0) {
				drawMapElement(i, false);
				drawMapElementPathNorth(i, false);
				drawMapElementPathSouth(i, false);
				drawMapElementPathWest(i, false);
				drawMapElementPathEast(i, false);
			}
		}
		drawMapName();
		// Устанавливаем позицию персонажа
		setPersPos(persPosX, persPosY);
		// --
		mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
	}
}

void GameMap::addMapElement(qint32 x, qint32 y)
{
	/**
	* Добавляет пустой элемент карты в массив и дает команду на прорисовку
	* Если такой элемент присутствует, никаких действий не производится
	**/
	// Подбираем карту, подходящую под наш элемент
	selectMap(x, y);
	// Проверяем, есть ли такой элемент в карте
	int i = getMapElementIndex(-1, x, y);
	if (i == -1) {
		// Добавляем элемент
		struct map_element map_el;
		map_el.status = 1;
		map_el.type = GameMap::TypeNormal;
		map_el.x = x; map_el.y = y;
		map_el.can_north = 0; map_el.can_south = 0; map_el.can_west = 0; map_el.can_east = 0;
		map_el.north_type = 0; map_el.south_type = 0; map_el.west_type = 0; map_el.east_type = 0;
		map_el.past_pers_pos = -1;
		map_el.enemies_min = -1;
		map_el.enemies_max = 0;
		map_el.mark.enabled = false;
		mapCurrArrayPtr->push_back(map_el);
		// Прописываем размеры карты
		if (mapsList[mapCurrIndex].map->size() == 0) {
			mapsList[mapCurrIndex].min_x = x;
			mapsList[mapCurrIndex].max_x = x;
			mapsList[mapCurrIndex].min_y = y;
			mapsList[mapCurrIndex].max_y = y;
		} else if (mapsList[mapCurrIndex].min_x != QINT32_MIN && mapsList[mapCurrIndex].max_x != QINT32_MIN && mapsList[mapCurrIndex].min_y != QINT32_MIN && mapsList[mapCurrIndex].max_y != QINT32_MIN) {
			if (mapsList[mapCurrIndex].min_x > x) {
				mapsList[mapCurrIndex].min_x = x;
			}
			if (mapsList[mapCurrIndex].max_x < x) {
				mapsList[mapCurrIndex].max_x = x;
			}
			if (mapsList[mapCurrIndex].min_y > y) {
				mapsList[mapCurrIndex].min_y = y;
			}
			if (mapsList[mapCurrIndex].max_y < y) {
				mapsList[mapCurrIndex].max_y = y;
			}
		}
		// Отмечаем как модифицированную
		if (!mapsList[mapCurrIndex].modified) {
			mapsList[mapCurrIndex].modified = true;
			modifiedMapsCount++;
			initSaveTimer();
		}
		// Рисуем элемент карты
		drawMapElement(mapCurrArrayPtr->size()-1, false);
		setSceneRect(getMapRect());
	}
	mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
}

void GameMap::drawMapElement(int el_index, bool modif)
{
	if ((*mapCurrArrayPtr)[el_index].status == 0)
		return;
	// Расчитываем координаты сцены
	int x = (*mapCurrArrayPtr)[el_index].x;
	int y = (*mapCurrArrayPtr)[el_index].y;
	if (null_element_x == QINT32_MIN) {
		// Проверяем начальную точку отсчета
		null_element_x = x;
		null_element_y = y;
	}
	x -= null_element_x;
	y = null_element_y - y;
	qreal x_ = (qreal)x * (qreal)MAP_ELEMENT_SIZE;
	qreal y_ = (qreal)y * (qreal)MAP_ELEMENT_SIZE;
	if (modif) {
		QRect itemRect = QRect(x_, y_, (qreal)MAP_ELEMENT_SIZE, (qreal)MAP_ELEMENT_SIZE);
		QList<QGraphicsItem*> gItems = items(itemRect, Qt::IntersectsItemShape);
		int cnt = gItems.size();
		for (int i = 0; i < cnt; i++) {
			int dataVal = gItems[i]->data(0).toInt();
			if (dataVal == 1 || dataVal == 9) {
				removeItem(gItems[i]);
			}
		}
	}
	qreal ellipseSize = MAP_ELEMENT_SIZE / 2;
	x_ += ellipseSize / 2;
	y_ += ellipseSize / 2;
	QBrush brush = QBrush();
	brush.setStyle(Qt::SolidPattern);
	if ((*mapCurrArrayPtr)[el_index].enemies_max == 0) {
		if ((*mapCurrArrayPtr)[el_index].type == GameMap::TypePortal || (*mapCurrArrayPtr)[el_index].type == GameMap::TypeSecret) {
			brush.setColor(Qt::green);
		} else {
			brush.setColor(Qt::gray);
		}
	} else {
		brush.setColor(Qt::red);
	}
	if ((*mapCurrArrayPtr)[el_index].mark.enabled) {
		QPainterPath markPath = QPainterPath();
		markPath.moveTo(x_ + ellipseSize * 0.75, y_ - 2);
		markPath.lineTo(x_ + ellipseSize * 0.75, y_ - 2 + ellipseSize * 0.5);
		markPath.lineTo(x_ + ellipseSize * 1.25, y_ - 2);
		QColor c = (*mapCurrArrayPtr)[el_index].mark.color;
		if (!c.isValid())
			c = QColor(Qt::blue);
		QGraphicsPathItem* gMarkElement = addPath(markPath, QPen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

		//QBrush markBrush = QBrush();
		//markBrush.setStyle(Qt::SolidPatern);
		//markBrush.setColor(Qt::blue);
		//QGraphicsEllipseItem* gMarkElement = addLine(x1, y1, x2, x2, QPen(Qt::black, 1.0f, Qt::SolidLine));
		gMarkElement->setZValue(8.0);
		gMarkElement->setData(0, 9);
	}
	QGraphicsEllipseItem* gElement = addEllipse(x_, y_, ellipseSize, ellipseSize, QPen(Qt::black, 1.0f, Qt::SolidLine), brush);
	gElement->setToolTip(makeTooltipForMapElement(el_index));
	gElement->setZValue(3.0);
	gElement->setData(0, 1);
}

/**
 * Формирование строчки для всплывающей подсказки элемента карты
 */
QString GameMap::makeTooltipForMapElement(int el_index)
{
	// Проверять el_index не будем - предполагается что вызов метода происходит уже после проверки.
	// mapCurrArrayPtr не проверяем по той же причине
	QString s_res = QString();
	const map_element *me = &mapCurrArrayPtr->at(el_index);
	if (me->mark.enabled && !me->mark.title.isEmpty()) {
		s_res.append(QString::fromUtf8("Метка: %1").arg(me->mark.title));
	}
	int min_enem = me->enemies_min;
	int max_enem = me->enemies_max;
	if (min_enem > 0 || max_enem != 0) {
		if (!s_res.isEmpty())
			s_res.append("\n");
		s_res.append(QString::fromUtf8("Противники: "));
		if (min_enem == max_enem) {
			s_res.append(QString::fromUtf8("%1").arg(min_enem));
		} else {
			s_res.append(QString::fromUtf8("%1-%2").arg(min_enem).arg(max_enem));
		}
		if ((*mapCurrArrayPtr)[el_index].enemies_list.size() > 0) {
			s_res.append("; ").append(me->enemies_list.join(", "));
		}
	}
	int type = me->type;
	if (type == GameMap::TypePortal || type == GameMap::TypeSecret) {
		if (!s_res.isEmpty())
			s_res.append("\n");
		s_res.append(QString::fromUtf8("Другие элементы: "));
		if (type == GameMap::TypePortal) {
			s_res.append(QString::fromUtf8("портал"));
		} else if (type == GameMap::TypeSecret) {
			s_res.append(QString::fromUtf8("тайник"));
		}
	}
	return s_res;
}

/**
 * Установка всплавающей подсказки
 */
void GameMap::setTooltipForMapElement(int el_index, QString tooltip_text)
{
	QRectF rect = getSceneCoordinates((*mapCurrArrayPtr)[el_index].x, (*mapCurrArrayPtr)[el_index].y);
	if (!rect.isNull()) {
		QList<QGraphicsItem*> gItems = items(rect, Qt::IntersectsItemShape);
		for (int i = 0; i < gItems.size(); i++) {
			QGraphicsItem* item = gItems.at(i);
			if (item->data(0).toInt() == 9) {
				item->setToolTip(tooltip_text);
				break;
			}
		}
	}
}

void GameMap::drawMapName()
{
	if (mapCurrIndex == -1 || mapsList[mapCurrIndex].status == None)
		return;
	QFont nameFont = QFont();
	nameFont.setBold(true);
	nameFont.setItalic(true);
	nameFont.setPixelSize(MAP_ELEMENT_SIZE);
	// Удаляем название карты, если существует
	QList<QGraphicsItem*> gItems = items();
	int cnt = gItems.size();
	for (int i = 0; i < cnt; i++) {
		if (gItems[i]->data(0).toString() == "map_name") {
			removeItem(gItems[i]);
			break;
		}
	}
	// Создаем элемент с именем карты
	QGraphicsTextItem* mapNameItem = addText(mapsList[mapCurrIndex].name, nameFont);
	if (mapNameItem) {
		mapNameItem->setOpacity(0.5);
		mapNameItem->setZValue(10.0);
		mapNameItem->setDefaultTextColor(QColor(70,70,255,255));
		mapNameItem->setData(0, "map_name");
		// Устанавливаем положение надписи
		if (null_element_x == QINT32_MIN) {
			return;
		}
		int min_x = mapsList[mapCurrIndex].min_x;
		qreal x = (min_x - null_element_x) * MAP_ELEMENT_SIZE;
		qreal y = (null_element_y - mapsList[mapCurrIndex].max_y) * MAP_ELEMENT_SIZE;
		//qreal width_delta = mapNameItem->textWidth() - ((mapsList[mapCurrIndex].max_x - min_x) * MAP_ELEMENT_SIZE);
		//if (width_delta > 0.0f) { // Ширина текста больше ширины карты
			// Выравниваем текст по центру
		//	qDebug() << x;
		//	x = x - width_delta / 2.0f;
		//	qDebug() << x;
		//}
		mapNameItem->setPos(x, y);
		if (mapNameItem->collidingItems().size() > 0) {
			// Текст перекрывает элементы карты, поднимаем его выше
			y = y - (qreal)MAP_ELEMENT_SIZE;
			//mapNameItem->setPos(x, y - MAP_ELEMENT_SIZE);
			mapNameItem->setPos(x, y);
		}
	}

	/*if (null_pers_pos_x == QINT32_MIN) {
		null_pers_pos_x = pers_x;
		null_pers_pos_y = pers_y;
	}
	persPosX = pers_x;
	persPosY = pers_y;
	// Высчитываем координаты положения персонажа
	qreal ellipseSize = MAP_ELEMENT_SIZE / 4;
	if (!persGraphicItem) {
		// Создаем элемент положения персонажа
		int x = pers_x - null_element_x;
		int y = null_element_y - pers_y;
		qreal x_ = (qreal)x * (qreal)MAP_ELEMENT_SIZE;
		qreal y_ = (qreal)y * (qreal)MAP_ELEMENT_SIZE;
		x_ += MAP_ELEMENT_SIZE * 3 / 8;
		y_ += MAP_ELEMENT_SIZE * 3 / 8;
		persGraphicItem = addEllipse(x_, y_, ellipseSize, ellipseSize, QPen(Qt::black, 1, Qt::SolidLine), QBrush(Qt::yellow, Qt::SolidPattern));
		persGraphicItem->setZValue(9.0);
	} else {
		// Перемещаем существующий элемент на новую позицию
		int x = pers_x - null_pers_pos_x;
		int y = null_pers_pos_y - pers_y;
		qreal x_ = (qreal)x * (qreal)MAP_ELEMENT_SIZE;
		qreal y_ = (qreal)y * (qreal)MAP_ELEMENT_SIZE;
		persGraphicItem->setPos(x_, y_);
	}
	int oldPersPosIndex = persPosIndex;
	persPosIndex = pers_index;*/

}

void GameMap::setPersPos(int pers_x, int pers_y)
{
	if (mapCurrIndex == -1) {
		return;
	}
	int pers_index = getMapElementIndex(-1, pers_x, pers_y);
	if (pers_index == -1) {
		return;
	}
	if (null_element_x == QINT32_MIN) {
		return;
	}
	if (null_pers_pos_x == QINT32_MIN) {
		null_pers_pos_x = pers_x;
		null_pers_pos_y = pers_y;
	}
	persPosX = pers_x;
	persPosY = pers_y;
	// Высчитываем координаты положения персонажа
	qreal ellipseSize = MAP_ELEMENT_SIZE / 4;
	if (!persGraphicItem) {
		// Создаем элемент положения персонажа
		int x = pers_x - null_element_x;
		int y = null_element_y - pers_y;
		qreal x_ = (qreal)x * (qreal)MAP_ELEMENT_SIZE;
		qreal y_ = (qreal)y * (qreal)MAP_ELEMENT_SIZE;
		x_ += MAP_ELEMENT_SIZE * 3 / 8;
		y_ += MAP_ELEMENT_SIZE * 3 / 8;
		persGraphicItem = addEllipse(x_, y_, ellipseSize, ellipseSize, QPen(Qt::black, 1, Qt::SolidLine), QBrush(Qt::yellow, Qt::SolidPattern));
		persGraphicItem->setZValue(9.0);
	} else {
		// Перемещаем существующий элемент на новую позицию
		int x = pers_x - null_pers_pos_x;
		int y = null_pers_pos_y - pers_y;
		qreal x_ = (qreal)x * (qreal)MAP_ELEMENT_SIZE;
		qreal y_ = (qreal)y * (qreal)MAP_ELEMENT_SIZE;
		persGraphicItem->setPos(x_, y_);
	}
	int oldPersPosIndex = persPosIndex;
	persPosIndex = pers_index;
	// Перерисовываем временные маршруты, если таковые существуют
	if (oldPersPosIndex != -1) {
		if ((*mapCurrArrayPtr)[oldPersPosIndex].north_type == 2) {
			drawMapElementPathNorth(oldPersPosIndex, ((*mapCurrArrayPtr)[oldPersPosIndex].can_north != 0));
		}
		if ((*mapCurrArrayPtr)[oldPersPosIndex].south_type == 2) {
			drawMapElementPathSouth(oldPersPosIndex, ((*mapCurrArrayPtr)[oldPersPosIndex].can_south != 0));
		}
		if ((*mapCurrArrayPtr)[oldPersPosIndex].west_type == 2) {
			drawMapElementPathWest(oldPersPosIndex, ((*mapCurrArrayPtr)[oldPersPosIndex].can_west != 0));
		}
		if ((*mapCurrArrayPtr)[oldPersPosIndex].east_type == 2) {
			drawMapElementPathEast(oldPersPosIndex, ((*mapCurrArrayPtr)[oldPersPosIndex].can_east != 0));
		}
	}
	if ((*mapCurrArrayPtr)[pers_index].north_type == 2) {
		drawMapElementPathNorth(pers_index, ((*mapCurrArrayPtr)[pers_index].can_north != 0));
	}
	if ((*mapCurrArrayPtr)[pers_index].south_type == 2) {
		drawMapElementPathSouth(pers_index, ((*mapCurrArrayPtr)[pers_index].can_south != 0));
	}
	if ((*mapCurrArrayPtr)[pers_index].west_type == 2) {
		drawMapElementPathWest(pers_index, ((*mapCurrArrayPtr)[pers_index].can_west != 0));
	}
	if ((*mapCurrArrayPtr)[pers_index].east_type == 2) {
		drawMapElementPathEast(pers_index, ((*mapCurrArrayPtr)[pers_index].can_east != 0));
	}
}

QRectF GameMap::getSceneCoordinates(int pers_x, int pers_y)
{
	/**
	* Преобразует координаты игры в координаты графической сцены
	**/
	QRectF resRect;
	if (null_element_x == QINT32_MIN) {
		return resRect;
	}
	resRect.setX(((qreal)pers_x - (qreal)null_element_x) * (qreal)MAP_ELEMENT_SIZE);
	resRect.setY(((qreal)null_element_y - (qreal)pers_y) * (qreal)MAP_ELEMENT_SIZE);
	resRect.setWidth((qreal)MAP_ELEMENT_SIZE);
	resRect.setHeight((qreal)MAP_ELEMENT_SIZE);
	return resRect;
}

QGraphicsItem* GameMap::getPersItem()
{
	return persGraphicItem;
}

int GameMap::getMapElementIndex(int mapIndex, int pers_x, int pers_y)
{
	/**
	* Ищет элемент карты с координатами игры в массиве элементов для карты
	* Если индекс карты -1, то ищет в текущей карте
	* Если не найдено, то возвращает -1
	**/
	if (mapIndex == -1) {
		mapIndex = mapCurrIndex;
	}
	// Проверки
	if (mapIndex < 0 || mapIndex >= mapsList.size() || mapsList[mapIndex].status == None)
		return -1;
	if (pers_x == QINT32_MIN || pers_y == QINT32_MIN) {
		return -1;
	}
	// Сначала сравним с последним запросом только для текущей карты (для скорости)
	if (mapIndex == mapCurrIndex && lastX == pers_x && lastY == pers_y) {
		return lastIndex;
	}
	// В кэше не нашли, ищем в массиве тупым перебором
	// Подгружаем карту, если необходимо
	if (mapsList[mapIndex].status == HeaderOnly) {
		if (!loadMap(mapIndex))
			return -1;
	}
	// Перебираем все элементы
	QVector<map_element>* mapArrayPtr = mapsList[mapIndex].map;
	int cnt = mapArrayPtr->size();
	for (int i = 0; i < cnt; i++) {
		if ((*mapArrayPtr)[i].status > 0 && (*mapArrayPtr)[i].x == pers_x && (*mapArrayPtr)[i].y == pers_y) {
			if (mapIndex == mapCurrIndex) {
				// Кэш только для текущей карты
				lastX = pers_x;
				lastY = pers_y;
				lastIndex = i;
			}
			return i;
		}
	}
	return -1;
}

void GameMap::setMapElementPaths(int pers_x, int pers_y, int paths)
{
	/**
	* Сохраняет и прорисовывает пути в элементе карты
	**/
	int i = getMapElementIndex(-1, pers_x, pers_y);
	if (i != -1) {
		// Прописываем пути в карту
		int unsure = paths & 3; // В наличии пути не уверены
		int south = 0;
		int west = 0;
		int east = 0;
		int north = 0;
		if ((unsure & 2) == 0) {
			// Юг
			south = (paths & 12) >> 2;
			if (south != 0) {
				if ((south == 3) || ((unsure & 1) == 0)) {
					south = 1;
				} else {
					south = 2;
				}
			}
			// Запад
			west = (paths & 48) >> 4;
			if (west != 0) {
				if ((west == 3) || ((unsure & 1) == 0)) {
					west = 1;
				} else {
					west = 2;
				}
			}
			// Восток
			east = (paths & 192) >> 6;
			if (east != 0) {
				if ((east == 3) || ((unsure & 1) == 0)) {
					east = 1;
				} else {
					east = 2;
				}
			}
			// Север
			north = (paths & 768) >> 8;
			if (north != 0) {
				if ((north == 3) || ((unsure & 1) == 0)) {
					north = 1;
				} else {
					north = 2;
				}
			}
		}
		// Можно прорисовывать карту
		bool modif_ = false;
		if ((*mapCurrArrayPtr)[i].can_north != north) {
			bool modif = false;
			if ((*mapCurrArrayPtr)[i].can_north) {
				modif = true;
			}
			if ((*mapCurrArrayPtr)[i].north_type == 0) {
				(*mapCurrArrayPtr)[i].north_type = 1;
			} else {
				(*mapCurrArrayPtr)[i].north_type = 2; // Непостоянный маршрут
			}
			modif_ = true;
			(*mapCurrArrayPtr)[i].can_north = north;
			drawMapElementPathNorth(i, modif);
		}
		if ((*mapCurrArrayPtr)[i].north_type == 0) {
			(*mapCurrArrayPtr)[i].north_type = 1;
		}
		if ((*mapCurrArrayPtr)[i].can_south != south) {
			bool modif = false;
			if ((*mapCurrArrayPtr)[i].can_south) {
				modif = true;
			}
			if ((*mapCurrArrayPtr)[i].south_type == 0) {
				(*mapCurrArrayPtr)[i].south_type = 1;
			} else {
				(*mapCurrArrayPtr)[i].south_type = 2; // Непостоянный маршрут
			}
			modif_ = true;
			(*mapCurrArrayPtr)[i].can_south = south;
			drawMapElementPathSouth(i, modif);
		}
		if ((*mapCurrArrayPtr)[i].south_type == 0) {
			(*mapCurrArrayPtr)[i].south_type = 1;
		}
		if ((*mapCurrArrayPtr)[i].can_west != west) {
			bool modif = false;
			if ((*mapCurrArrayPtr)[i].can_west) {
				modif = true;
			}
			if ((*mapCurrArrayPtr)[i].west_type == 0) {
				(*mapCurrArrayPtr)[i].west_type = 1;
			} else {
				(*mapCurrArrayPtr)[i].west_type = 2; // Непостоянный маршрут
			}
			modif_ = true;
			(*mapCurrArrayPtr)[i].can_west = west;
			drawMapElementPathWest(i, modif);
		}
		if ((*mapCurrArrayPtr)[i].west_type == 0) {
			(*mapCurrArrayPtr)[i].west_type = 1;
		}
		if ((*mapCurrArrayPtr)[i].can_east != east) {
			bool modif = false;
			if ((*mapCurrArrayPtr)[i].can_east) {
				modif = true;
			}
			if ((*mapCurrArrayPtr)[i].east_type == 0) {
				(*mapCurrArrayPtr)[i].east_type = 1;
			} else {
				(*mapCurrArrayPtr)[i].east_type = 2; // Непостоянный маршрут
			}
			modif_ = true;
			(*mapCurrArrayPtr)[i].can_east = east;
			drawMapElementPathEast(i, modif);
		}
		if ((*mapCurrArrayPtr)[i].east_type == 0) {
			(*mapCurrArrayPtr)[i].east_type = 1;
		}
		if (modif_) {
			if (!mapsList[mapCurrIndex].modified) {
				mapsList[mapCurrIndex].modified = true;
				modifiedMapsCount++;
				initSaveTimer();
			}
		}
		mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
	}
}

void GameMap::drawMapElementPathNorth(int element_index, bool modif)
{
	/**
	* Непосредственно прорисовывает путь движения на карте для выбранного элемента (север)
	**/
	Q_UNUSED(modif);
	qreal x1 = ((qreal)(*mapCurrArrayPtr)[element_index].x - (qreal)null_element_x) * (qreal)MAP_ELEMENT_SIZE;
	qreal y1 = ((qreal)null_element_y - (qreal)(*mapCurrArrayPtr)[element_index].y) * (qreal)MAP_ELEMENT_SIZE;
	QRect itemRect = QRect(x1+(qreal)MAP_ELEMENT_SIZE/4.0, y1+(qreal)MAP_ELEMENT_SIZE/4.0, (qreal)MAP_ELEMENT_SIZE/2.0, (qreal)MAP_ELEMENT_SIZE/2.0);
	QList<QGraphicsItem*> gItems = items(itemRect, Qt::IntersectsItemShape);
	int cnt = gItems.size();
	for (int i = 0; i < cnt; i++) {
		if (gItems[i]->data(0).toInt() == 8) {
			removeItem(gItems[i]);
			break;
		}
	}
	x1 += (qreal)MAP_ELEMENT_SIZE / 2.0;
	y1 += (qreal)MAP_ELEMENT_SIZE / 10.0; // учитываем утолщение кончика
	int type = (*mapCurrArrayPtr)[element_index].north_type;
	if ((*mapCurrArrayPtr)[element_index].can_north || type == 2) {
		qreal x2 = x1;
		qreal y2 = y1 + (qreal)MAP_ELEMENT_SIZE / 4.0; // С запасом - утолщение на хвосте
		QPen pen = QPen();
		setPathPen(element_index, type, (*mapCurrArrayPtr)[element_index].can_north, &pen);
		addLine(x1, y1, x2, y2, pen)->setData(0, 8);
	}
}

void GameMap::drawMapElementPathSouth(int element_index, bool modif)
{
	/**
	* Непосредственно прорисовывает путь движения на карте для выбранного элемента (юг)
	**/
	Q_UNUSED(modif);
	qreal x1 = ((qreal)(*mapCurrArrayPtr)[element_index].x - (qreal)null_element_x) * (qreal)MAP_ELEMENT_SIZE;
	qreal y1 = ((qreal)null_element_y - (qreal)(*mapCurrArrayPtr)[element_index].y) * (qreal)MAP_ELEMENT_SIZE;
	QRect itemRect = QRect(x1+(qreal)MAP_ELEMENT_SIZE/4.0, y1+(qreal)MAP_ELEMENT_SIZE/4.0, (qreal)MAP_ELEMENT_SIZE/2.0, (qreal)MAP_ELEMENT_SIZE/2.0);
	QList<QGraphicsItem*> gItems = items(itemRect, Qt::IntersectsItemShape);
	int cnt = gItems.size();
	for (int i = 0; i < cnt; i++) {
		if (gItems[i]->data(0).toInt() == 2) {
			removeItem(gItems[i]);
			break;
		}
	}
	x1 += (qreal)MAP_ELEMENT_SIZE / 2.0;
	y1 += (qreal)MAP_ELEMENT_SIZE * 0.9; // 9/10
	int type = (*mapCurrArrayPtr)[element_index].south_type;
	if ((*mapCurrArrayPtr)[element_index].can_south || type == 2) {
		qreal x2 = x1;
		qreal y2 = y1 - (qreal)MAP_ELEMENT_SIZE / 4.0; // Это с запасом
		QPen pen = QPen();
		setPathPen(element_index, type, (*mapCurrArrayPtr)[element_index].can_south, &pen);
		addLine(x1, y1, x2, y2, pen)->setData(0, 2);
	}
}

void GameMap::drawMapElementPathWest(int element_index, bool modif)
{
	/**
	* Непосредственно прорисовывает путь движения на карте для выбранного элемента (запад)
	**/
	Q_UNUSED(modif);
	qreal x1 = ((qreal)(*mapCurrArrayPtr)[element_index].x - (qreal)null_element_x) * (qreal)MAP_ELEMENT_SIZE;
	qreal y1 = ((qreal)null_element_y - (qreal)(*mapCurrArrayPtr)[element_index].y) * (qreal)MAP_ELEMENT_SIZE;
	QRect itemRect = QRect(x1+(qreal)MAP_ELEMENT_SIZE/4.0, y1+(qreal)MAP_ELEMENT_SIZE/4.0, (qreal)MAP_ELEMENT_SIZE/2.0, (qreal)MAP_ELEMENT_SIZE/2.0);
	QList<QGraphicsItem*> gItems = items(itemRect, Qt::IntersectsItemShape);
	int cnt = gItems.size();
	for (int i = 0; i < cnt; i++) {
		if (gItems[i]->data(0).toInt() == 4) {
			removeItem(gItems[i]);
			break;
		}
	}
	x1 += (qreal)MAP_ELEMENT_SIZE / 10.0;
	y1 += (qreal)MAP_ELEMENT_SIZE / 2.0;
	int type = (*mapCurrArrayPtr)[element_index].west_type;
	if ((*mapCurrArrayPtr)[element_index].can_west || type == 2) {
		qreal x2 = x1 + (qreal)MAP_ELEMENT_SIZE / 4.0;
		qreal y2 = y1;
		QPen pen = QPen();
		setPathPen(element_index, type, (*mapCurrArrayPtr)[element_index].can_west, &pen);
		addLine(x1, y1, x2, y2, pen)->setData(0, 4);
	}
}

void GameMap::drawMapElementPathEast(int element_index, bool modif)
{
	/**
	* Непосредственно прорисовывает путь движения на карте для выбранного элемента (восток)
	**/
	Q_UNUSED(modif);
	qreal x1 = ((qreal)(*mapCurrArrayPtr)[element_index].x - (qreal)null_element_x) * (qreal)MAP_ELEMENT_SIZE;
	qreal y1 = ((qreal)null_element_y - (qreal)(*mapCurrArrayPtr)[element_index].y) * (qreal)MAP_ELEMENT_SIZE;
	QRect itemRect = QRect(x1+(qreal)MAP_ELEMENT_SIZE/4.0, y1+(qreal)MAP_ELEMENT_SIZE/4.0, (qreal)MAP_ELEMENT_SIZE/2.0, (qreal)MAP_ELEMENT_SIZE/2.0);
	QList<QGraphicsItem*> gItems = items(itemRect, Qt::IntersectsItemShape);
	int cnt = gItems.size();
	for (int i = 0; i < cnt; i++) {
		if (gItems[i]->data(0).toInt() == 6) {
			removeItem(gItems[i]);
			break;
		}
	}
	x1 += (qreal)MAP_ELEMENT_SIZE * 0.9;
	y1 += (qreal)MAP_ELEMENT_SIZE / 2.0;
	int type = (*mapCurrArrayPtr)[element_index].east_type;
	if ((*mapCurrArrayPtr)[element_index].can_east || type == 2) {
		qreal x2 = x1 - (qreal)MAP_ELEMENT_SIZE / 4.0;
		qreal y2 = y1;
		QPen pen = QPen();
		setPathPen(element_index, type, (*mapCurrArrayPtr)[element_index].can_east, &pen);
		addLine(x1, y1, x2, y2, pen)->setData(0, 6);
	}
}

void GameMap::setPathPen(int map_index, int path_type, int can_move, QPen* pen)
{
	/**
	* Настраивает перо, в зависимости от типа пути и позиции персонажа
	**/
	Q_UNUSED(map_index);
	pen->setColor(Qt::gray);
	pen->setStyle(Qt::SolidLine);
	if (can_move != 0) {
		if (path_type == 2) {
			pen->setWidth((qreal)MAP_ELEMENT_SIZE / 8.0);
		} else {
			pen->setWidth((qreal)MAP_ELEMENT_SIZE / 4.0);
		}
	} else if (path_type == 2) {
		pen->setWidth((qreal)MAP_ELEMENT_SIZE / 16.0);
	}
	/*if (path_type == 2 && (can_move == 0 || persPosIndex == -1 || persPosIndex != map_index)) {
		pen->setWidth((qreal)MAP_ELEMENT_SIZE / 8.0);
	} else {
		pen->setWidth((qreal)MAP_ELEMENT_SIZE / 4.0);
	}*/
}

void GameMap::setMapElementEnemies(qint32 x, qint32 y, int count_min, int count_max)
{
	/**
	* Устанавливает количество врагов для элемента карты
	**/
	if (count_min > count_max) {
		return;
	}
	int idx = getMapElementIndex(-1, x, y);
	if (idx == -1) {
		return;
	}
	bool modifFlag = false;
	if ((*mapCurrArrayPtr)[idx].enemies_min == -1 || (*mapCurrArrayPtr)[idx].enemies_min > count_min) {
		(*mapCurrArrayPtr)[idx].enemies_min = count_min;
		modifFlag = true;
	}
	if ((*mapCurrArrayPtr)[idx].enemies_max < count_max) {
		(*mapCurrArrayPtr)[idx].enemies_max = count_max;
		modifFlag = true;
	}
	if (modifFlag) {
		if (!mapsList[mapCurrIndex].modified) {
			mapsList[mapCurrIndex].modified = true;
			modifiedMapsCount++;
			initSaveTimer();
		}
		drawMapElement(idx, true);
		setTooltipForMapElement(idx, makeTooltipForMapElement(idx));
	}
}

/**
 * Устанавливает список врагов для элемента карты
 */
void GameMap::setMapElementEnemiesList(qint32 x, qint32 y, QStringList enList)
{
	int idx = getMapElementIndex(-1, x, y);
	if (idx == -1)
		return;
	bool modifFlag = false;
	for (int enIdx = 0; enIdx < enList.size(); enIdx++) {
		if (mapCurrArrayPtr->at(idx).enemies_list.indexOf(enList.at(enIdx)) == -1) {
			(*mapCurrArrayPtr)[idx].enemies_list.push_back(enList.at(enIdx));
			modifFlag = true;
		}
	}
	(*mapCurrArrayPtr)[idx].enemies_list.sort();
	if (modifFlag) {
		if (!mapsList[mapCurrIndex].modified) {
			mapsList[mapCurrIndex].modified = true;
			modifiedMapsCount++;
			initSaveTimer();
		}
		drawMapElement(idx, true);
		setTooltipForMapElement(idx, makeTooltipForMapElement(idx));
	}
	mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
}

void GameMap::setMapElementType(qint32 pers_x, qint32 pers_y, qint32 type)
{
	/**
	* Устанавливает тип элемента карты
	**/
	int i = getMapElementIndex(-1, pers_x, pers_y);
	if (i != -1) {
		if ((*mapCurrArrayPtr)[i].type != type) {
			(*mapCurrArrayPtr)[i].type = type;
			if (!mapsList[mapCurrIndex].modified) {
				mapsList[mapCurrIndex].modified = true;
				modifiedMapsCount++;
				initSaveTimer();
			}
			drawMapElement(i, true);
			setTooltipForMapElement(i, makeTooltipForMapElement(i));
		}
		mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
	}
}

void GameMap::mapsInfo(struct maps_info* mapsInfoPtr)
{
	/**
	* Заполняет структуру общей информацией о картах
	**/
	int recCnt = mapsList.size();
	int mapsCnt = 0;
	int mapsLoaded = 0;
	for (int i = 0; i < recCnt; i++) {
		if (mapsList[i].status != None) {
			mapsCnt++;
			if (mapsList[i].status == InMemory) {
				mapsLoaded++;
			}
		}
	}
	mapsInfoPtr->maps_count = mapsCnt;
	mapsInfoPtr->maps_loaded = mapsLoaded;
	mapsInfoPtr->curr_map_index = mapCurrIndex;
	if (mapCurrIndex != -1) {
		mapsInfoPtr->curr_map_name = mapsList[mapCurrIndex].name;
	} else {
		mapsInfoPtr->curr_map_name = "";
	}
}

void GameMap::getMapsList(QVector<maps_list2>* maps_ls)
{
	/**
	* Возвращает информацию об имеющихся картах
	**/
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		MapStatus mapStatus = mapsList[i].status;
		if (mapStatus != None) {
			struct maps_list2 mp_ls;
			mp_ls.index = i;
			mp_ls.name = mapsList[i].name;
			mp_ls.loaded = (mapStatus != HeaderOnly);
			maps_ls->push_back(mp_ls);
		}
	}
}

bool GameMap::switchMap(int mapIndex)
{
	/**
	* Переключает текущую карту
	**/
	if (mapCurrIndex != -1)
		clearOtherPersPos();
	if (mapIndex < 0 || mapIndex >= mapsList.size() || mapsList[mapIndex].status == None)
		return false;
	if (mapIndex == mapCurrIndex)
		return true;
	// Загружаем карту, если не загружена
	if (mapsList[mapIndex].status == HeaderOnly) {
		if (!loadMap(mapIndex))
			return false;
	}
	// Очищаем кэши индекса карт
	lastX = QINT32_MIN;
	lastY = QINT32_MIN;
	// Прописываем индекс и указатель на текущую карту
	mapCurrIndex = mapIndex;
	mapCurrArrayPtr = mapsList[mapIndex].map;
	// Перерисовываем карту
	redrawMap();
	// Подгоняем размер сцены
	setSceneRect(getMapRect());
	return true;
}

QRectF GameMap::getMapRect()
{
	/**
	* Обсчитывает размер текущей карты
	*/
	QRectF mapSz = QRectF();

	QList<QGraphicsItem*> gItems = items();
	int cnt = gItems.size();
	for (int i = 0; i < cnt; i++) {
		mapSz = mapSz.united(gItems[i]->mapRectToScene(gItems[i]->boundingRect()));
	}
	mapSz.setX(mapSz.x() - (qreal)MAP_ELEMENT_SIZE * 0.5f);
	mapSz.setY(mapSz.y() - (qreal)MAP_ELEMENT_SIZE * 0.5f);
	mapSz.setWidth(mapSz.width() + (qreal)MAP_ELEMENT_SIZE * 1.0f);
	mapSz.setHeight(mapSz.height() + (qreal)MAP_ELEMENT_SIZE * 1.0f);

	return mapSz;
}

bool GameMap::unloadMap(int mapIndex)
{
	/**
	* Выгружает выбранную карту из памяти не сохраняя
	* TODO !!! Пересчитать минимальные и максимальные координаты карты !!!
	**/
	if (mapIndex < 0 || mapIndex >= mapsList.size() || mapsList[mapIndex].status == None)
		return false;
	// Освобождаем память
	mapsList[mapIndex].status = HeaderOnly;
	QVector<map_element>* mapPtr = mapsList[mapIndex].map;
	if (mapPtr)
		delete mapPtr;
	mapsList[mapIndex].map = 0;
	// Если выгружаемая карта текущая
	if (mapIndex == mapCurrIndex) {
		// Очищаем кэши индекса карт
		lastX = QINT32_MIN;
		lastY = QINT32_MIN;
		// Прописываем индекс и указатель на текущую карту
		mapCurrIndex = -1;
		// Перерисовываем карту
		redrawMap();
		// Подгоняем размер сцены
		setSceneRect(getMapRect());
	}
	return true;
}

bool GameMap::clearMap(int mapIndex)
{
	/**
	* Удаляет с карты все элементы, в том числе из памяти
	*/
	if (mapIndex < 0 || mapIndex >= mapsList.size() || mapsList[mapIndex].status == None)
		return false;
	QVector<map_element>* mapPtr = mapsList[mapIndex].map;
	mapPtr->clear();
	clearOtherPersPos();
	if (mapIndex == mapCurrIndex) {
		// Очищаем кэши индекса карт
		lastX = QINT32_MIN;
		lastY = QINT32_MIN;
		// Перерисовываем карту
		redrawMap();
		// Подгоняем размер сцены
		setSceneRect(getMapRect());
	}
	mapsList[mapIndex].min_x = QINT32_MIN;
	mapsList[mapIndex].max_x = QINT32_MIN;
	mapsList[mapIndex].min_y = QINT32_MIN;
	mapsList[mapIndex].max_y = QINT32_MIN;
	if (!mapsList[mapIndex].modified) {
		mapsList[mapIndex].modified = true;
		modifiedMapsCount++;
		initSaveTimer();
	}
	mapsList[mapIndex].last_access = QDateTime::currentDateTime();
	return true;
}

/**
 * Задает новое имя карты
 */
int GameMap::renameMap(int mapIndex, QString mapNewName)
{
	int mapsCount = mapsList.size();
	if (mapIndex < 0 || mapIndex >= mapsCount || mapsList[mapIndex].status == None)
		return 1;
	QString newName = mapNewName.trimmed();
	// Проверяем наличие карты с таким же именем
	for (int i = 0; i < mapsCount; i++) {
		if (mapsList[mapIndex].status != None) {
			if (mapsList[mapIndex].status == HeaderOnly) {
				if (!loadMap(mapIndex))
					return 3;
			}
			if (mapsList[mapIndex].name == newName)
				return 2;
		}
	}
	// Сохраняем старое имя карты
	if (mapsList[mapIndex].old_name.isEmpty())
		mapsList[mapIndex].old_name = mapsList[mapIndex].name;
	// Прописываем новое имя карты
	mapsList[mapIndex].name = newName;
	if (mapIndex == mapCurrIndex) {
		// Перерисовываем имя карты
		drawMapName();
		setSceneRect(getMapRect());
	}
	//--
	if (!mapsList[mapIndex].modified) {
		mapsList[mapIndex].modified = true;
		modifiedMapsCount++;
		initSaveTimer();
	}
	mapsList[mapIndex].last_access = QDateTime::currentDateTime();
	return 0;
}

void GameMap::clearOtherPersPos()
{
	/**
	* Очищает метки положения других игроков на карте
	**/
	// Очистка элементов карты
	QList<QGraphicsItem*> gItems = items();
	while (!gItems.isEmpty()) {
		QGraphicsItem* item = gItems.takeFirst();
		if (item->data(0).toInt() == 7) {
			removeItem(item);
		}
	}
	// Очистка списка положения игроков
	otherPers.clear();

}

void GameMap::drawOtherPersPos(int other_index)
{
	/**
	* Отмечает местоположение другого персонажа карте
	**/
	if (mapCurrIndex == -1)
		return;
	if (null_element_x == QINT32_MIN || null_element_y == QINT32_MIN || persPosX == QINT32_MIN || persPosY == QINT32_MIN)
		return;
	if (other_index < 0 || other_index >= otherPers.size())
		return;
	int x = persPosX - null_element_x + otherPers.at(other_index).x;
	int y = null_element_y - persPosY - otherPers.at(other_index).y;
	qreal x_ = (qreal)x * (qreal)MAP_ELEMENT_SIZE;
	qreal y_ = (qreal)y * (qreal)MAP_ELEMENT_SIZE;
	x_ += (qreal)MAP_ELEMENT_SIZE * 0.3;
	// Создаем элемент положения персонажа
	QGraphicsLineItem* otherPersItem = addLine(QLineF(x_, y_, x_, y_ + (qreal)MAP_ELEMENT_SIZE * 0.5), QPen(Qt::green, 2, Qt::SolidLine));
	otherPersItem->setZValue(8.0);
	otherPersItem->setData(0, 7);
	// Отображаем надпись над маркером
	if (!otherPers.at(other_index).names.isEmpty()) {
		QFont nameFont = QFont();
		nameFont.setBold(true);
		nameFont.setItalic(true);
		nameFont.setPixelSize(MAP_ELEMENT_SIZE / 2);
		QGraphicsTextItem* otherPersNames = addText(otherPers.at(other_index).names.join("\n"), nameFont);
		if (otherPersNames) {
			otherPersNames->setOpacity(0.7);
			otherPersNames->setZValue(9.0);
			otherPersNames->setData(0, 7);
			otherPersNames->setDefaultTextColor(QColor(32,128,32,255));
			// Устанавливаем положение надписи
			otherPersNames->setPos(x_ + (qreal)MAP_ELEMENT_SIZE / 8.0, y_ - (qreal)MAP_ELEMENT_SIZE / 2.0);
			//if (mapNameItem->collidingItems().size() > 0) {
				// Текст перекрывает элементы карты, поднимаем его выше
			//	y = y - (qreal)MAP_ELEMENT_SIZE;
				//mapNameItem->setPos(x, y - MAP_ELEMENT_SIZE);
			//	mapNameItem->setPos(x, y);
			//}
		}
	}
}

void GameMap::initSaveTimer()
{
	if (modifiedMapsCount && autoSaveInterval) {
		if (saveTimer == NULL) {
			saveTimer = new QTimer(this);
			saveTimer->setSingleShot(false);
			connect(saveTimer, SIGNAL(timeout()), this, SLOT(doAutoSave()));
			saveTimer->start(autoSaveInterval * 60000);
		} else {
			if (!saveTimer->isActive())
				saveTimer->start(autoSaveInterval * 60000);
		}
	} else {
		if (saveTimer != NULL) {
			disconnect(saveTimer, SIGNAL(timeout()), this, SLOT(doAutoSave()));
			delete saveTimer;
			saveTimer = NULL;
		}
	}
}

void GameMap::doAutoSave()
{
	if (modifiedMapsCount) {
		saveMap();
	}
}

void GameMap::setMapsParams(ParamId param_id, int param)
{
	saveMode = param;
	if (param_id == AutoSaveMode) {
		if (param == 2) {
			autoSaveInterval = 5; // 5min
		} else {
			autoSaveInterval = 0;
		}
		initSaveTimer();
	}
}

/**
 * Инициирует таймер автовыгрузки карт.
 * Карта выгружаются только в случае, если она не активна и сохранена.
 */
void GameMap::initUnloadTimer(bool update_interval)
{
	if (autoUnloadInterval) {
		// Анализ заголовка карт, для выяснения необходимости запуска таймера и выгрузки карт
		int saved_count = 0;
		QDateTime oldest_access;
		QDateTime curr_time = QDateTime::currentDateTime().addSecs(1); // Поправка на "ветер"
		int maps_cnt = mapsList.size();
		for (int i = 0; i < maps_cnt; i++) {
			if (mapsList[i].status == InMemory && !mapsList[i].modified && i != mapCurrIndex) {
				QDateTime map_access = mapsList[i].last_access;
				if (map_access.secsTo(curr_time) >= autoUnloadInterval * 60) {
					unloadMap(i);
					continue;
				}
				if (oldest_access.isNull() || oldest_access > mapsList[i].last_access)
					oldest_access = mapsList[i].last_access;
				saved_count++;
			}
		}
		if (saved_count > 0 && !oldest_access.isNull()) {
			// Таймер нужен
			if (unloadTimer != NULL) {
				if (!update_interval)
					return; // Используем ленивый режим
				if (unloadTimer->isActive())
					unloadTimer->stop();
			} else {
				unloadTimer = new QTimer(this);
				unloadTimer->setSingleShot(true);
				connect(unloadTimer, SIGNAL(timeout()), this, SLOT(doAutoUnload()));
			}
			unloadTimer->start(autoUnloadInterval * 60000);
			return;
		}
	}
	unloadTimer->deleteLater(); // Удалиться позже
	unloadTimer = NULL; // А указатель очистим сейчас
}

void GameMap::doAutoUnload()
{
	//
}
