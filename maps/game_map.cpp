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
#include <QPainter>
#include <QImage>
#include <QTextDocument>


#include "game_map.h"
#include "utils.h"
#include "settings.h"
#include "pers.h"

GameMap::GameMap(QObject *parent) : QObject(parent),
	mapCurrIndex(-1),
	mapCurrArrayPtr(NULL),
	modifiedMapsCount(false),
	saveMode(1),
	saveTimer(NULL),
	autoSaveInterval(0),
	unloadTimer(NULL),
	autoUnloadInterval(0),
	unloading(false)
{
	mapScene_ = new MapScene(this);
	connect(Pers::instance(), SIGNAL(persParamChanged(int, int, int)), this, SLOT(persParamChanged(int, int, int)));
}

GameMap::~GameMap()
{
	unloading = true;
	// Сохраняем карты
	if (saveMode > 0)
		saveMap();
	// Освобождаем память занятую под загруженные карты
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		if (mapsList.at(i).status == InMemory) {
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

void GameMap::init(const QString &acc_jid)
{
	if (!currAccJid.isEmpty() && saveMode > 0) {
		saveMap();
	}
	// Очищаем картинку
	mapScene_->clear();
	// Сохраняем новый джид
	currAccJid = acc_jid;
	// Инициализация переменных
	persPos.reset();
	mapCache = MapCache();
	mapCurrIndex = -1;
	mapCurrArrayPtr = NULL;
	otherPers.clear();
	autoUnloadInterval = 0;
	// Загружаем настройки модуля карт
	loadMapsSettings(Settings::instance()->getMapsData());
	// Загружаем список карт
	loadMapsList();
}

QGraphicsScene* GameMap::getGraphicsScene()
{
	return mapScene_;
}

bool GameMap::loadMapsList()
{
	/**
	* Загружает список карт с файла
	**/
	// Очищаем массив со старыми картами
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		if (mapsList.at(i).status == InMemory) {
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
				MapInfo map_head(HeaderOnly, sName);// Карта существует но не загружена
				bool fOk;
				int minX = eMap.attribute("min-x").toInt(&fOk);
				if (fOk) {
					int minY = eMap.attribute("min-y").toInt(&fOk);
					if (fOk) {
						int maxX = eMap.attribute("max-x").toInt(&fOk);
						if (fOk) {
							int maxY = eMap.attribute("max-y").toInt(&fOk);
							if (fOk) {
								map_head.rect = MapRect(minX, maxX, minY, maxY);
							}
						}
					}
				}
				mapsList.push_back(map_head);
			}
		}
		eMapNode = eMapNode.nextSibling();
	}
	modifiedMapsCount = 0;
	initSaveTimer();
	return true;
}

/**
 * Создает пустую карту с именем map_name
 * Возвращает индекс новой созданной карты или -1 в случае ошибки
 */
int GameMap::createMap(const QString &map_name)
{
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		if (mapsList.at(i).name == map_name) {
			// Такое имя карты уже есть
			return -1;
		}
	}
	MapInfo mapHeader(NewMap, map_name); // Отмечаем как новую
	mapHeader.map = new QVector<struct GameMap::MapElement>;
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
	QString sName = mapsList.at(map_index).old_name;
	if (sName.isEmpty())
		sName = mapsList.at(map_index).name;
	bool oldVersion = false;
	QString sVer = eRoot.attribute("version");
	if (sVer == "0.1") {
		oldVersion = true; // Старый формат карты
	} else if (sVer != "0.2") {
		return false; // Неизвестный формат карт
	}
	QDomElement eMap;
	if (oldVersion) {
		eMap = eRoot.firstChildElement();
	} else {
		if (currAccJid.isEmpty())
			return false;
		QDomElement eAcc = eRoot.firstChildElement();
		while (!eAcc.isNull()) {
			if (eAcc.tagName() == "account") {
				if (eAcc.attribute("jid") == currAccJid) {
					eMap = eAcc.firstChildElement();
					break;
				}
			}
			eAcc = eAcc.nextSiblingElement();
		}
	}
	while (!eMap.isNull()) {
		if (eMap.tagName() == "map") {
			if (eMap.attribute("name") == sName) {
				break;
			}
		}
		eMap = eMap.nextSiblingElement();
	}
	if (eMap.isNull()) {
		return false;
	}
	// Нашли нужную нам карту, теперь грузим ее в память
	makeMapFromDomElement(mapsList[map_index], eMap);
	// Отмечаем карту как загруженную
	mapsList[map_index].status = InMemory;
	// Другие флаги
	mapsList[map_index].modified = false;
	mapsList[map_index].last_access = QDateTime::currentDateTime();
	// Запускаем таймер автовыгрузки, если необходимо
	initUnloadTimer(false);
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
						QString sName = mapsList.at(i).old_name;
						if (sName.isEmpty())
							sName = mapsList.at(i).name;
						if (domMapName == sName) {
							MapStatus nStatus = mapsList.at(i).status;
							if (nStatus == InMemory) {
								// Эта карта загружена в память, формируем XML элемент
								QDomNode eNewMap = makeMapXmlElement(xmlDoc, mapsList.at(i));
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
		const MapStatus nStatus = mapsList.at(i).status;
		if (nStatus == NewMap) {
			// Новая карта
			QDomNode eNewMap = makeMapXmlElement(xmlDoc, mapsList.at(i));
			if (!eNewMap.isNull()) {
				eAcc.appendChild(eNewMap);
			}
			mapsList[i].status = InMemory;
		}
		if (!mapsList.at(i).old_name.isEmpty())
			mapsList[i].old_name = "";
		mapsList[i].modified = false;
	}
	xmlDoc.appendChild(eRoot);
	// Сохраняем в файл
	if (!savePluginXml(&xmlDoc, "sofgame_maps.xml")) {
		return false;
	}
	//--
	if (!unloading) {
		modifiedMapsCount = 0;
		initSaveTimer();
		initUnloadTimer(false);
	}
	return true;
}

/**
 * Экспорт карты с списком индексов maps_list, тип exp_type, имя файла exp_file
 */
int GameMap::exportMaps(const QStringList &maps_list, int exp_type, const QString &exp_file)
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
				QDomNode mapElement = makeMapXmlElement(xmlDoc, mapsList.at(i));
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
	} else 	if (exp_type == 2) { // PNG файл
		if (maps_list.size() != 1)
			return 4;
		QString str1 = maps_list.at(0);
		if (str1 == "*")
			return 4;
		const int mapIndex = str1.toInt();
		if (mapIndex < 0 || mapIndex >= mapsList.size())
			return 2;
		MapStatus status = mapsList.at(mapIndex).status;
		if (status == HeaderOnly || status == InMemory || status == NewMap) {
			if (status == HeaderOnly) {
				if (!loadMap(mapIndex))
					return -1;
			}
			// Создаем новую сцену и заполняем ее
			MapScene *imgScene = new MapScene();
			paintMap(imgScene, mapIndex);
			QRectF rect = imgScene->getMapSceneRect(1.0f);
			imgScene->setSceneRect(rect);
			int width = rect.width();
			int height = rect.height();
			QImage image(width * 2, height * 2, QImage::Format_ARGB32);
			image.fill(QColor(Qt::white).rgb());
			QPainter *painter = new QPainter(&image);
			painter->setRenderHint(QPainter::Antialiasing);
			imgScene->render(painter);
			delete painter;
			delete imgScene;
			mapsList[mapIndex].last_access = QDateTime::currentDateTime();
			if (!image.save(exp_file, "PNG", -1)) {
				return 3;
			}
			return 0;
		}
		return 2;
	}
	return 1;
}

/**
 * Импорт карты из XML файла
 */
int GameMap::importMaps(const QString &imp_file)
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
	QDomElement eRootChild = eRoot.firstChildElement();
	int mapsCnt = mapsList.size();
	int expMapsCnt = 0;
	while (!eRootChild.isNull()) {
		if (eRootChild.tagName() == "map") {
			// Получаем карту из DOM ноды
			MapInfo ml(NewMap, QString()); // Отмечаем карту как новую
			if (makeMapFromDomElement(ml, eRootChild)) {
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
				// Карта модифицирована
				ml.modified = true;
				modifiedMapsCount++;
				ml.last_access = QDateTime::currentDateTime();
				initSaveTimer();
				// Добавляем карту в список карт
				mapsList.push_back(ml);
			}
		}
		eRootChild = eRootChild.nextSiblingElement();
	}
	return 0;
}

/**
 * Функция удаляет карту.
 */
bool GameMap::removeMap(int map_index)
{
	if (map_index >= 0 && map_index < mapsList.size()) {
		if (mapsList.at(map_index).status == InMemory) {
			unloadMap(map_index);
		}
		mapsList[map_index].status = None;
		// Если выгружаемая карта текущая
		if (map_index == mapCurrIndex) {
			// Очищаем кэши индекса карт
			mapCache = MapCache();
			// Прописываем индекс и указатель на текущую карту
			mapCurrIndex = -1;
			// Перерисовываем карту
			redrawMap();
			// Подгоняем размер сцены
			mapScene_->setSceneRect(mapScene_->getMapSceneRect(0.5f));
		}
		//--
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
	MapInfo *ml1 = &mapsList[map1_index];
	MapInfo *ml2 = &mapsList[map2_index];
	if (ml1->status == HeaderOnly)
		loadMap(map1_index);
	if (ml2->status == HeaderOnly)
		loadMap(map2_index);
	// Проверяем готовность карт
	if ((ml1->status != InMemory && ml1->status != NewMap) || (ml2->status != InMemory  && ml2->status != NewMap))
		return false;
	// Начинаем сканирование и объединение карт
	MapRect newRect = ml1->rect;
	QVector<MapElement>* mel1 = ml1->map;
	QVector<MapElement>* mel2 = ml2->map;
	int cnt = mel2->size();
	for (int i = 0; i < cnt; i++) {
		const struct MapElement* me2 = &mel2->at(i);
		if (me2->status == 1) {
			const MapPos &pos = me2->pos;
			newRect.addPoint(pos);
			bool fNew = true;
			MapElement *me1 = NULL;
			int me1_index = getMapElementIndex(map1_index, pos);
			if (me1_index != -1) {
				me1 = &(*mel1)[me1_index];
				if (me1->status == 1)
					fNew = false;
			}
			if (fNew) {
				// Нет такого элемента. Добавляем новый
				mel1->append(*me2);
			} else {
				// Есть такой элемент в базовой карте
				if (me1->feature == MapScene::MapElementFeature())
					me1->feature = me2->feature;
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
	ml1->rect = newRect;
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
		mapScene_->setSceneRect(mapScene_->getMapSceneRect(0.5f));
	}
	return true;
}

QDomNode GameMap::makeMapXmlElement(QDomDocument xmlDoc, const MapInfo &map_head) const
{
	// Создаем элемент с нашей картой
	QDomElement eMap = xmlDoc.createElement("map");
	eMap.setAttribute("name", map_head.name);
	const MapRect &mapRect = map_head.rect;
	if (mapRect.isValid()) {
		eMap.setAttribute("min-x", mapRect.left());
		eMap.setAttribute("max-x", mapRect.right());
		eMap.setAttribute("min-y", mapRect.bottom());
		eMap.setAttribute("max-y", mapRect.top());
	}
	// Создаем временный указатель
	QVector<struct GameMap::MapElement>* mapPtr = map_head.map;
	if (mapPtr) {
		int map_size = mapPtr->size();
		// Выгружаем элементы массива
		QDomElement eMapItems = xmlDoc.createElement("map-items");
		eMap.appendChild(eMapItems);
		for (int mapIdx = 0; mapIdx < map_size; mapIdx++) {
			MapElement *me = &(*mapPtr)[mapIdx];
			if (me->status == 1) { // Только если элемент карты в памяти
				QDomElement eMapItem = xmlDoc.createElement("map-item");
				// Координаты карты
				eMapItem.setAttribute("pos-x", QString::number(me->pos.x()));
				eMapItem.setAttribute("pos-y", QString::number(me->pos.y()));
				// Отметка на карте
				if (me->mark.enabled) {
					QDomElement eMapItemMark = xmlDoc.createElement("mark");
					eMapItem.appendChild(eMapItemMark);
					if (!me->mark.title.isEmpty())
						eMapItemMark.setAttribute("title", me->mark.title);
					eMapItemMark.setAttribute("color", me->mark.color.name());
				}
				// Порталы, секреты и др.
				if (me->feature.testFlag(MapScene::LocationPortal)) {
					QDomElement eMapItemPortal = xmlDoc.createElement("portal");
					eMapItem.appendChild(eMapItemPortal);
				}
				if (me->feature.testFlag(MapScene::LocationSecret)) {
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

bool GameMap::makeMapFromDomElement(MapInfo &mapHeader, const QDomElement &mapElement)
{
	QString mapName = mapElement.attribute("name").trimmed();
	if (mapName.isEmpty())
		return false;
	QVector<struct GameMap::MapElement>* mapPtr = mapHeader.map;
	if (mapPtr) {
		if (!mapPtr->isEmpty())
			mapPtr->remove(0, mapPtr->size());
		//mapPtr->clear(); //Достали варнинги у QT (!!! Проверить на новых версиях!!!)
	} else {
		mapPtr = new QVector<struct MapElement>;
	}
	mapHeader.status = InMemory;
	mapHeader.name = mapName;
	mapHeader.old_name = QString();
	MapRect mapRect;
	QDomElement eMapItems = mapElement.firstChildElement();
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
							MapPos pos(nPosX, nPosY);
							MapElement map_el(pos);
							map_el.north_type = 1; map_el.south_type = 1;
							map_el.west_type = 1; map_el.east_type = 1;
							mapRect.addPoint(pos);
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
									map_el.feature |= MapScene::LocationPortal;
								} else if (sTagName == "secret") {
									map_el.feature |= MapScene::LocationSecret;
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
	mapHeader.rect = mapRect;
	mapHeader.map = mapPtr;
	return true;
}

void GameMap::selectMap(const MapPos &pos)
{
	/**
	* Выбор и загрузка подходящей карты для указанной точки
	**/
	// Сначала проверяем текущую карту
	if (mapCurrIndex != -1) {
		if (mapsList.at(mapCurrIndex).rect.contains(pos)) {
			// Текущая карта нам вполне подходит
			return;
		}
	}
	// Сканируем имеющиеся карты
	int mapDefIndex = -1;
	int mapNearIndex = -1;
	int mapNearDist = 0;
	int mapGoodIndex = -1;
	int mapsCnt = mapsList.size();
	for (int i = 0; i < mapsCnt; i++) {
		const MapInfo *mh = &mapsList.at(i);
		if (mh->status != None) {
			if (mapDefIndex == -1 && mh->name == "default") {
				mapDefIndex = i;
				continue;
			}
			MapRect mapRect = mh->rect;
			int dist = mapRect.distance(pos);
			if (dist == 0) {
				mapGoodIndex = i;
				break;
			} else if (dist > 0 && dist <= 10) {
				if (mapNearIndex == -1 || mapNearDist > dist) {
					mapNearIndex = i;
					mapNearDist = dist;
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
				selMap = createMap("default");
				break;
			}
		}
		if (selMap != -1) {
			break;
		}
	}
	if (selMap == -1) {
		// Смены карты не произошло
		return;
	}
	// Переключаем карту
	switchMap(selMap);
}

/**
 * Возвращает индекс элемента карты по координатам графической сцены для текущей карты
 * Возвращает -1 в случае отсутствия
 */
int GameMap::getIndexByCoordinate(const QPointF &p)
{
	MapPos pos = mapScene_->sceneToMapCoordinates(p);
	int i = getMapElementIndex(-1, pos);
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
	const MapStatus status = mapsList.at(souMapIndex).status;
	if (status == None)
		return;
	// Загружаем карту источник, если не загружена
	if (status == HeaderOnly) {
		if (!loadMap(souMapIndex))
			return;
	}
	// Проверяем корректность индекса элемента
	if (elementIndex >= mapsList.at(souMapIndex).map->size())
		return;
	struct MapElement mapEl = (*mapsList[souMapIndex].map)[elementIndex];
	if (mapEl.status == 0)
		return;
	// Ищем такой же элемент в карте получателе
	const MapPos &pos = mapEl.pos;
	int desElIndex = getMapElementIndex(desMapIndex, pos);
	if (desElIndex != -1) {
		// Заменяем элемент
		(*mapsList[desMapIndex].map)[desElIndex] = mapEl;
	} else {
		// Добавляем элемент
		mapsList[desMapIndex].map->push_back(mapEl);
		mapsList[desMapIndex].rect.addPoint(pos);
	}
	// Удаляем элемент из источника
	removeMapElement(souMapIndex, elementIndex);
	// Отмечаем карты как модифицированные
	if (!mapsList.at(souMapIndex).modified) {
		mapsList[souMapIndex].modified = true;
		modifiedMapsCount++;
		initSaveTimer();
	}
	if (!mapsList.at(desMapIndex).modified) {
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
	struct MapElement *mapEl = &(*mapsList[mapIndex].map)[elementIndex];
	// Отмечаем элемент как не используемый
	mapEl->status = 0;
	const int x = mapEl->pos.x();
	const int y = mapEl->pos.y();
	MapRect mapRect = mapsList.at(mapIndex).rect;
	if (mapRect.left() == x || mapRect.right() == x || mapRect.top() == y || mapRect.bottom() == y) {
		// Пересчитываем границы карты источника, т.к. удаляемая точка находится на границе карты
		QVector<MapElement>* mapPtr = mapsList[mapIndex].map;
		int cnt = mapPtr->size();
		mapRect = MapRect();
		for (int i = 0; i < cnt; i++) {
			if (mapPtr->at(i).status != 0) {
				mapRect.addPoint(mapPtr->at(i).pos);
			}
		}
		mapsList[mapIndex].rect = mapRect;
	}
	// Очищаем кэши индекса карт, т.к. вероятно, что наш перемещенный элемент сидит в кэше
	if (mapIndex == mapCurrIndex) {
		mapCache = MapCache();
	}
	// Меняем время доступа
	mapsList[mapIndex].last_access = QDateTime::currentDateTime();
	// Отмечаем карту как модифицированную
	if (!mapsList.at(mapIndex).modified) {
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
	if (mapCurrArrayPtr == NULL || elementIndex < 0 || elementIndex >= mapCurrArrayPtr->size() || mapCurrArrayPtr->at(elementIndex).status != 1) {
		MapElementMark mark;
		mark.enabled = false;
		mark.color = QColor(Qt::blue);
		return mark;
	}
	return mapCurrArrayPtr->at(elementIndex).mark;
}

/**
 * Устанавливает отметку элемента карты
 */
void GameMap::setMapElementMark(int elementIndex, const QString &title, const QColor &c)
{
	if (mapCurrArrayPtr) {
		if (elementIndex >= 0 && elementIndex < mapCurrArrayPtr->size()) {
			MapElement *me = &(*mapCurrArrayPtr)[elementIndex];
			if (me->status == 1 && (!me->mark.enabled || me->mark.title != title || me->mark.color != c)) {
				me->mark.enabled = true;
				me->mark.title = title;
				me->mark.color = c;
				mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
				if (!mapsList.at(mapCurrIndex).modified) {
					mapsList[mapCurrIndex].modified = true;
					modifiedMapsCount++;
					initSaveTimer();
				}
				mapScene_->drawMark(me->pos, true, c);
				mapScene_->setTooltip(me->pos, makeTooltipForMapElement(elementIndex));
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
			MapElement *me = &(*mapCurrArrayPtr)[elementIndex];
			if (me->status == 1 && me->mark.enabled) {
				me->mark.enabled = false;
				mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
				if (!mapsList.at(mapCurrIndex).modified) {
					mapsList[mapCurrIndex].modified = true;
					modifiedMapsCount++;
					initSaveTimer();
				}
				mapScene_->drawMark(me->pos, false, QColor());
				mapScene_->setTooltip(me->pos, makeTooltipForMapElement(elementIndex));
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
	if (other_pers_pos != NULL && persPos.isValid()) {
		for (int i = 0, cnt1 = other_pers_pos->size(); i < cnt1; i++) {
			const maps_other_pers &othPersExt = other_pers_pos->at(i);
			MapPos othPos(persPos.x() + othPersExt.offset_x, persPos.y() + othPersExt.offset_y);
			int idx = -1;
			for (int j = 0, cnt2 = otherPers.size(); j < cnt2; j++) {
				if (otherPers.at(j).pos == othPos) {
					idx = j;
					break;
				}
			}
			if (idx == -1) {
				idx = otherPers.size();
				OtherPers op;
				op.pos = othPos;
				otherPers.append(op);
			}
			otherPers[idx].names.append(other_pers_pos->at(i).name);
		}
		foreach (const OtherPers &oth, otherPers) {
			mapScene_->drawOtherPersPos(oth.pos, oth.names);
		}
		mapScene_->setSceneRect(mapScene_->getMapSceneRect(0.5f));
	}
	mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
}

/**
 * Рисует указанную карту на указанной сцене
 * Карта должна быть уже загружена в память
 */
void GameMap::paintMap(MapScene *scene, int mapIndex)
{
	const QVector<GameMap::MapElement> *mapArrayPtr = mapsList.at(mapIndex).map;
	for (int i = 0, cnt = mapArrayPtr->size(); i < cnt; i++) {
		const MapElement *me = &mapArrayPtr->at(i);
		if (me->status != 0) {
			const MapPos &pos = me->pos;
			scene->drawMapElement(pos, me->feature, (me->enemies_max > 0), false);
			scene->drawMapElementPathNorth(pos, me->north_type, (me->can_north != 0));
			scene->drawMapElementPathSouth(pos, me->south_type, (me->can_south != 0));
			scene->drawMapElementPathWest(pos, me->west_type, (me->can_west != 0));
			scene->drawMapElementPathEast(pos, me->east_type, (me->can_east != 0));
			if (me->mark.enabled) {
				scene->drawMark(pos, true, me->mark.color);
			}
		}
	}
	scene->drawMapName(mapsList.at(mapIndex).name);
}

/**
 * Стирает карту с графической сцены и перерисовывает заново
 */
void GameMap::redrawMap()
{
	// Очистка сцены
	mapScene_->clear();
	mapScene_->update();
	// Прорисовываем загруженную карту
	if (mapCurrIndex != -1) {
		paintMap(mapScene_, mapCurrIndex);
		// Устанавливаем позицию персонажа
		setPersPos(persPos);
		// Тултипы
		for (int i = 0, cnt = mapCurrArrayPtr->size(); i < cnt; i++) {
			const MapElement &me = mapCurrArrayPtr->at(i);
			if (me.status != 0) {
				mapScene_->setTooltip(me.pos, makeTooltipForMapElement(i));
			}
		}
		// --
		mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
	}
}

/**
 * Добавляет пустой элемент карты в массив и дает команду на прорисовку
 * Если такой элемент присутствует, никаких действий не производится
 */
void GameMap::addMapElement(const MapPos &pos)
{
	// Подбираем карту, подходящую под наш элемент
	selectMap(pos);
	// Проверяем, есть ли такой элемент в карте
	int i = getMapElementIndex(-1, pos);
	if (i == -1) {
		// Добавляем элемент
		MapElement me(pos);
		mapCurrArrayPtr->push_back(me);
		i = mapCurrArrayPtr->size() - 1;
		// Прописываем размеры карты
		mapsList[mapCurrIndex].rect.addPoint(pos);
		// Отмечаем как модифицированную
		if (!mapsList.at(mapCurrIndex).modified) {
			mapsList[mapCurrIndex].modified = true;
			modifiedMapsCount++;
			initSaveTimer();
		}
		// Рисуем элемент карты
		mapScene_->drawMapElement(pos, me.feature, false, false);
		// Подсказки
		mapScene_->setTooltip(pos, makeTooltipForMapElement(i));
		mapScene_->setSceneRect(mapScene_->getMapSceneRect(0.5f));
	}
	mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
}

/**
 * Формирование строчки для всплывающей подсказки элемента карты
 */
QString GameMap::makeTooltipForMapElement(int el_index) const
{
	// Проверять el_index не будем - предполагается что вызов метода происходит уже после проверки.
	// mapCurrArrayPtr не проверяем по той же причине
	QString s_res;
	const MapElement *me = &mapCurrArrayPtr->at(el_index);
	if (me->mark.enabled) {
		s_res.append(QString("<tr><td><div class=\"layer1\">%1</div></td><td><div class=\"layer2\">%2</div></td></tr>")
			.arg(QString::fromUtf8("Метка:"))
			.arg((me->mark.title.isEmpty()) ? QString::fromUtf8("<em>нет&nbsp;описания</em>") : Qt::escape(me->mark.title)));
	}
	int min_enem = me->enemies_min;
	int max_enem = me->enemies_max;
	if (min_enem > 0 || max_enem != 0) {
		QString enemStr;
		if (min_enem == max_enem) {
			enemStr = QString::fromUtf8("%1").arg(min_enem);
		} else {
			enemStr = QString::fromUtf8("от&nbsp;%1&nbsp;до&nbsp;%2").arg(min_enem).arg(max_enem);
		}
		s_res.append(QString("<tr><td><div class=\"layer1\">%1</div></td><td><div class=\"layer2\">%2</div></td></tr>")
			.arg(QString::fromUtf8("Кол-во противников:"))
			.arg(enemStr));
		if (me->enemies_list.size() > 0) {
			QStringList enemList;
			foreach (const QString &enemStr, me->enemies_list) {
				enemList.append(Qt::escape(enemStr).replace(" ", "&nbsp;", Qt::CaseInsensitive));
			}
			s_res.append(QString("<tr><td><div class=\"layer1\">%1</div></td><td><div class=\"layer2\">%2</div></td></tr>")
				.arg(QString::fromUtf8("Противники:"))
				.arg(enemList.join(", "))); //escape сделано ранее
		}
	}
	MapScene::MapElementFeature feature = me->feature;
	if (feature.testFlag(MapScene::LocationPortal) || feature.testFlag(MapScene::LocationSecret)) {
		QString featureStr;
		if (feature.testFlag(MapScene::LocationPortal)) {
			featureStr = QString::fromUtf8("портал");
		}
		if (feature.testFlag(MapScene::LocationSecret)) {
			if (feature.testFlag(MapScene::LocationPortal))
				featureStr.append(", ");
			featureStr.append(QString::fromUtf8("тайник"));
		}
		s_res.append(QString("<tr><td><div class=\"layer1\">%1</div></td><td><div class=\"layer2\">%2</div></td></tr>")
			.arg(QString::fromUtf8("Другие элементы:"))
			.arg(Qt::escape(featureStr)));
	}

	if (!s_res.isEmpty()) {
		s_res = "<qt><style type='text/css'>"
			".layer1 {white-space:pre;}"
			".layer2 {white-space:normal;margin-left:10px;}"
			"</style>"
			"<table>" + s_res + "</table></qt>";
	}
	return s_res;
}

void GameMap::setUnloadInterval(int minutes)
{
	if (autoUnloadInterval != minutes) {
		autoUnloadInterval = minutes;
		if (autoUnloadInterval < 0)
			autoUnloadInterval = 0;
		if (autoUnloadInterval != 0) {
			initUnloadTimer(true);
			return;
		}
		if (unloadTimer != NULL) {
			if (unloadTimer->isActive())
				unloadTimer->stop();
			delete unloadTimer;
			unloadTimer = NULL;
		}
	}
}

void GameMap::setPersPos(const MapPos &pos)
{
	if (mapCurrIndex == -1) {
		return;
	}
	int pers_index = getMapElementIndex(-1, pos);
	if (pers_index == -1) {
		return;
	}
	persPos = pos;
	mapScene_->drawPersPos(pos);
	int oldPersPosIndex = mapCache.persPosIndex;
	mapCache.persPosIndex = pers_index;
	// Перерисовываем временные маршруты, если таковые существуют
	if (oldPersPosIndex != -1) {
		const MapElement *me = &mapCurrArrayPtr->at(oldPersPosIndex);
		if (me->north_type == 2) {
			mapScene_->drawMapElementPathNorth(me->pos, me->north_type, (me->can_north != 0));
		}
		if (me->south_type == 2) {
			mapScene_->drawMapElementPathSouth(me->pos, me->south_type, (me->can_south != 0));
		}
		if (me->west_type == 2) {
			mapScene_->drawMapElementPathWest(me->pos, me->west_type, (me->can_west != 0));
		}
		if (me->east_type == 2) {
			mapScene_->drawMapElementPathEast(me->pos, me->east_type, (me->can_east != 0));
		}
	}
	const MapElement *me = &mapCurrArrayPtr->at(pers_index);
	if (me->north_type == 2) {
		mapScene_->drawMapElementPathNorth(me->pos, me->north_type, (me->can_north != 0));
	}
	if (me->south_type == 2) {
		mapScene_->drawMapElementPathSouth(me->pos, me->south_type, (me->can_south != 0));
	}
	if (me->west_type == 2) {
		mapScene_->drawMapElementPathWest(me->pos, me->west_type, (me->can_west != 0));
	}
	if (me->east_type == 2) {
		mapScene_->drawMapElementPathEast(me->pos, me->east_type, (me->can_east != 0));
	}
}

/**
 * Ищет элемент карты с координатами игры в массиве элементов для карты
 * Если индекс карты -1, то ищет в текущей карте
 * Если не найдено, то возвращает -1
 */
int GameMap::getMapElementIndex(int mapIndex, const MapPos &pos)
{
	if (mapIndex == -1) {
		mapIndex = mapCurrIndex;
	}
	// Проверки
	if (mapIndex < 0 || mapIndex >= mapsList.size() || mapsList.at(mapIndex).status == None)
		return -1;
	if (!pos.isValid())
		return -1;
	// Сначала сравним с последним запросом только для текущей карты (для скорости)
	if (mapIndex == mapCurrIndex && mapCache.lastPos == pos) {
		return mapCache.lastIndex;
	}
	// В кэше не нашли, ищем в массиве тупым перебором
	// Подгружаем карту, если необходимо
	if (mapsList.at(mapIndex).status == HeaderOnly) {
		if (!loadMap(mapIndex))
			return -1;
	}
	// Перебираем все элементы
	const QVector<MapElement>* mapArrayPtr = mapsList.at(mapIndex).map;
	for (int i = 0, cnt = mapArrayPtr->size(); i < cnt; i++) {
		const MapElement *me = &mapArrayPtr->at(i);
		if (me->status > 0 && me->pos == pos) {
			if (mapIndex == mapCurrIndex) {
				// Кэш только для текущей карты
				mapCache.lastPos = pos;
				mapCache.lastIndex = i;
			}
			return i;
		}
	}
	return -1;
}

void GameMap::setMapElementPaths(const MapPos &pos, int paths)
{
	/**
	* Сохраняет и прорисовывает пути в элементе карты
	**/
	const int i = getMapElementIndex(-1, pos);
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
		MapElement *me = &(*mapCurrArrayPtr)[i];
		if (me->can_north != north) {
			if (me->north_type == 0) {
				me->north_type = 1;
			} else {
				me->north_type = 2; // Непостоянный маршрут
			}
			modif_ = true;
			me->can_north = north;
			mapScene_->drawMapElementPathNorth(me->pos, me->north_type, (me->can_north != 0));
		}
		if (me->north_type == 0) {
			me->north_type = 1;
		}
		if (me->can_south != south) {
			if (me->south_type == 0) {
				me->south_type = 1;
			} else {
				me->south_type = 2; // Непостоянный маршрут
			}
			modif_ = true;
			me->can_south = south;
			mapScene_->drawMapElementPathSouth(me->pos, me->south_type, (me->can_south != 0));
		}
		if (me->south_type == 0) {
			me->south_type = 1;
		}
		if (me->can_west != west) {
			if (me->west_type == 0) {
				me->west_type = 1;
			} else {
				me->west_type = 2; // Непостоянный маршрут
			}
			modif_ = true;
			me->can_west = west;
			mapScene_->drawMapElementPathWest(me->pos, me->west_type, (me->can_west != 0));
		}
		if (me->west_type == 0) {
			me->west_type = 1;
		}
		if (me->can_east != east) {
			if (me->east_type == 0) {
				me->east_type = 1;
			} else {
				me->east_type = 2; // Непостоянный маршрут
			}
			modif_ = true;
			me->can_east = east;
			mapScene_->drawMapElementPathEast(me->pos, me->east_type, (me->can_east != 0));
		}
		if (me->east_type == 0) {
			me->east_type = 1;
		}
		if (modif_) {
			if (!mapsList.at(mapCurrIndex).modified) {
				mapsList[mapCurrIndex].modified = true;
				modifiedMapsCount++;
				initSaveTimer();
			}
		}
		mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
	}
}

void GameMap::setMapElementEnemies(const MapPos &pos, int count_min, int count_max)
{
	/**
	* Устанавливает количество врагов для элемента карты
	**/
	if (count_min > count_max) {
		return;
	}
	const int idx = getMapElementIndex(-1, pos);
	if (idx == -1) {
		return;
	}
	bool modifFlag = false;
	MapElement &me = (*mapCurrArrayPtr)[idx];
	if (me.enemies_min == -1 || me.enemies_min > count_min) {
		me.enemies_min = count_min;
		modifFlag = true;
	}
	if (me.enemies_max < count_max) {
		me.enemies_max = count_max;
		modifFlag = true;
	}
	if (modifFlag) {
		if (!mapsList.at(mapCurrIndex).modified) {
			mapsList[mapCurrIndex].modified = true;
			modifiedMapsCount++;
			initSaveTimer();
		}
		mapScene_->drawMapElement(pos, me.feature, (me.enemies_max > 0), true);
		mapScene_->setTooltip(pos, makeTooltipForMapElement(idx));
	}
}

/**
 * Устанавливает список врагов для элемента карты
 */
void GameMap::setMapElementEnemiesList(const MapPos &pos, const QStringList &enList)
{
	int idx = getMapElementIndex(-1, pos);
	if (idx == -1)
		return;
	bool modifFlag = false;
	MapElement &me = (*mapCurrArrayPtr)[idx];
	for (int enIdx = 0; enIdx < enList.size(); enIdx++) {
		if (me.enemies_list.indexOf(enList.at(enIdx)) == -1) {
			me.enemies_list.push_back(enList.at(enIdx));
			modifFlag = true;
		}
	}
	me.enemies_list.sort();
	if (modifFlag) {
		if (!mapsList.at(mapCurrIndex).modified) {
			mapsList[mapCurrIndex].modified = true;
			modifiedMapsCount++;
			initSaveTimer();
		}
		mapScene_->drawMapElement(pos, me.feature, (me.enemies_max > 0), true);
		mapScene_->setTooltip(pos, makeTooltipForMapElement(idx));
	}
	mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
}

void GameMap::setMapElementType(const MapPos &pos, const MapScene::MapElementFeature &feature)
{
	/**
	* Устанавливает тип элемента карты
	**/
	const int i = getMapElementIndex(-1, pos);
	if (i != -1) {
		MapElement &me = (*mapCurrArrayPtr)[i];
		if (me.feature != feature) {
			me.feature = feature;
			if (!mapsList.at(mapCurrIndex).modified) {
				mapsList[mapCurrIndex].modified = true;
				modifiedMapsCount++;
				initSaveTimer();
			}
			mapScene_->drawMapElement(pos, feature, (me.enemies_max > 0), true);
			mapScene_->setTooltip(pos, makeTooltipForMapElement(i));
		}
		mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
	}
}

void GameMap::mapsInfo(struct maps_info* mapsInfoPtr) const
{
	/**
	* Заполняет структуру общей информацией о картах
	**/
	int recCnt = mapsList.size();
	int mapsCnt = 0;
	int mapsLoaded = 0;
	for (int i = 0; i < recCnt; i++) {
		const MapStatus status = mapsList.at(i).status;
		if (status != None) {
			mapsCnt++;
			if (status == InMemory) {
				mapsLoaded++;
			}
		}
	}
	mapsInfoPtr->maps_count = mapsCnt;
	mapsInfoPtr->maps_loaded = mapsLoaded;
	mapsInfoPtr->curr_map_index = mapCurrIndex;
	if (mapCurrIndex != -1) {
		mapsInfoPtr->curr_map_name = mapsList.at(mapCurrIndex).name;
	} else {
		mapsInfoPtr->curr_map_name = "";
	}
}

void GameMap::getMapsList(QVector<maps_list2>* maps_ls) const
{
	/**
	* Возвращает информацию об имеющихся картах
	**/
	int cnt = mapsList.size();
	for (int i = 0; i < cnt; i++) {
		MapStatus mapStatus = mapsList.at(i).status;
		if (mapStatus != None) {
			struct maps_list2 mp_ls;
			mp_ls.index = i;
			mp_ls.name = mapsList.at(i).name;
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
	bool forUnload = false;
	if (mapCurrIndex != -1) {
		clearOtherPersPos();
		if (!mapsList.at(mapCurrIndex).modified)
			forUnload = true;
		mapsList[mapCurrIndex].last_access = QDateTime::currentDateTime();
	}
	if (mapIndex < 0 || mapIndex >= mapsList.size() || mapsList.at(mapIndex).status == None)
		return false;
	if (mapIndex == mapCurrIndex)
		return true;
	// Загружаем карту, если не загружена
	if (mapsList.at(mapIndex).status == HeaderOnly) {
		if (!loadMap(mapIndex))
			return false;
	}
	// Очищаем кэши индекса карт
	mapCache = MapCache();
	// Прописываем индекс и указатель на текущую карту
	mapCurrIndex = mapIndex;
	mapCurrArrayPtr = mapsList[mapIndex].map;
	// Перерисовываем карту
	redrawMap();
	// Подгоняем размер сцены
	mapScene_->setSceneRect(mapScene_->getMapSceneRect(0.5f));
	// Обновляем метку последнего доступа
	mapsList[mapIndex].last_access = QDateTime::currentDateTime();
	// Инициируем таймер автовыгрузки, если необходимо
	if (forUnload)
		initUnloadTimer(false);
	return true;
}

bool GameMap::unloadMap(int mapIndex)
{
	/**
	* Выгружает выбранную карту из памяти не сохраняя
	* TODO !!! Пересчитать минимальные и максимальные координаты карты !!!
	**/
	if (mapIndex < 0 || mapIndex >= mapsList.size() || mapsList.at(mapIndex).status == None)
		return false;
	// Освобождаем память
	mapsList[mapIndex].status = HeaderOnly;
	QVector<MapElement>* mapPtr = mapsList[mapIndex].map;
	mapsList[mapIndex].map = NULL;
	if (mapPtr)
		delete mapPtr;
	// Если выгружаемая карта текущая
	if (mapIndex == mapCurrIndex) {
		// Очищаем кэши индекса карт
		mapCache = MapCache();
		// Прописываем индекс и указатель на текущую карту
		mapCurrIndex = -1;
		// Перерисовываем карту
		redrawMap();
		// Подгоняем размер сцены
		mapScene_->setSceneRect(mapScene_->getMapSceneRect(0.5f));
	}
	return true;
}

bool GameMap::clearMap(int mapIndex)
{
	/**
	* Удаляет с карты все элементы, в том числе из памяти
	*/
	if (mapIndex < 0 || mapIndex >= mapsList.size() || mapsList.at(mapIndex).status == None)
		return false;
	QVector<MapElement>* mapPtr = mapsList[mapIndex].map;
	mapPtr->clear();
	clearOtherPersPos();
	if (mapIndex == mapCurrIndex) {
		// Очищаем кэши индекса карт
		mapCache = MapCache();
		// Перерисовываем карту
		redrawMap();
		// Подгоняем размер сцены
		mapScene_->setSceneRect(mapScene_->getMapSceneRect(0.5f));
	}
	mapsList[mapIndex].rect = MapRect();
	if (!mapsList.at(mapIndex).modified) {
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
int GameMap::renameMap(int mapIndex, const QString &mapNewName)
{
	int mapsCount = mapsList.size();
	if (mapIndex < 0 || mapIndex >= mapsCount || mapsList.at(mapIndex).status == None)
		return 1;
	QString newName = mapNewName.trimmed();
	// Проверяем наличие карты с таким же именем
	for (int i = 0; i < mapsCount; i++) {
		if (mapsList.at(i).status == None || mapsList.at(mapIndex).name == newName) {
			return 2;
		}
	}
	if (mapsList.at(mapIndex).status == HeaderOnly && !loadMap(mapIndex))
		return 3;
	// Сохраняем старое имя карты
	if (mapsList.at(mapIndex).old_name.isEmpty())
		mapsList[mapIndex].old_name = mapsList.at(mapIndex).name;
	// Прописываем новое имя карты
	mapsList[mapIndex].name = newName;
	if (mapIndex == mapCurrIndex) {
		// Перерисовываем имя карты
		mapScene_->drawMapName(newName);
		mapScene_->setSceneRect(mapScene_->getMapSceneRect(0.5f));
	}
	//--
	if (!mapsList.at(mapIndex).modified) {
		mapsList[mapIndex].modified = true;
		modifiedMapsCount++;
		initSaveTimer();
	}
	mapsList[mapIndex].last_access = QDateTime::currentDateTime();
	return 0;
}

/**
 * Очищает все метки положения других игроков на карте
 */
void GameMap::clearOtherPersPos()
{
	// Очистка элементов карты
	mapScene_->removePersPosElements();
	// Очистка списка положения игроков
	otherPers.clear();

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

int GameMap::getMapsSettingParam(ParamId id) const
{
	if (id == AutoSaveMode) {
		return saveMode;
	}
	return -1;
}

void GameMap::setMapsParam(ParamId param_id, int param)
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

void GameMap::loadMapsSettings(const QDomElement &xml)
{
	QDomElement eChild = xml.firstChildElement("maps-save-mode");
	int saveMode_ = 1;
	if (!eChild.isNull()) {
		QString sMode = eChild.attribute("value").toLower();
		int i = Settings::persSaveModeStrings.indexOf(sMode);
		if (i != -1) {
			saveMode_ = i;
		}
	}
	setMapsParam(AutoSaveMode, saveMode_);
	eChild = xml.firstChildElement("maps-unload-interval");
	int interval = 0;
	if (!eChild.isNull()) {
		interval = eChild.attribute("value").toInt();
	}
	setUnloadInterval(interval);
	QDomElement ePersPos = xml.firstChildElement("pers-position");
	if (!ePersPos.isNull()) {
		QDomElement ePersPosColor = ePersPos.firstChildElement("color");
		if (!ePersPosColor.isNull()) {
			QColor c = QColor(ePersPosColor.attribute("value"));
			if (c.isValid()) {
				setPersPosColor(c);
			}
		}
	}
}

QDomElement GameMap::exportMapsSettingsToDomElement(QDomDocument &xmlDoc) const
{
	QDomElement eMaps = xmlDoc.createElement("maps");
	if (saveMode >= 0 && saveMode < Settings::persSaveModeStrings.size()) {
		QDomElement eMapSaveMode = xmlDoc.createElement("maps-save-mode");
		eMapSaveMode.setAttribute("value", Settings::persSaveModeStrings.at(saveMode));
		eMaps.appendChild(eMapSaveMode);
	}
	QDomElement eUnloadInterval = xmlDoc.createElement("maps-unload-interval");
	eUnloadInterval.setAttribute("value", autoUnloadInterval);
	eMaps.appendChild(eUnloadInterval);
	QDomElement ePersPos = xmlDoc.createElement("pers-position");
	eMaps.appendChild(ePersPos);
	QDomElement ePersPosColor = xmlDoc.createElement("color");
	ePersPosColor.setAttribute("value", getPersPosColor().name());
	ePersPos.appendChild(ePersPosColor);
	return eMaps;
}

/**
 * Инициирует таймер автовыгрузки карт.
 * Карта выгружаются только в случае, если она не активна и сохранена.
 */
void GameMap::initUnloadTimer(bool update_interval)
{
	if (autoUnloadInterval != 0) {
		// Анализ заголовка карт, для выяснения необходимости запуска таймера и выгрузки карт
		int saved_count = 0;
		QDateTime oldest_access;
		QDateTime curr_time = QDateTime::currentDateTime().addSecs(1); // Поправка на "ветер"
		int maps_cnt = mapsList.size();
		for (int i = 0; i < maps_cnt; i++) {
			// Карта должна быть загруженной, сохраненной и не текущей
			if (mapsList.at(i).status == InMemory && !mapsList.at(i).modified && i != mapCurrIndex) {
				QDateTime map_access = mapsList.at(i).last_access;
				if (map_access.secsTo(curr_time) >= autoUnloadInterval * 60) {
					unloadMap(i);
					continue;
				}
				if (oldest_access.isNull() || oldest_access > map_access)
					oldest_access = map_access;
				saved_count++;
			}
		}
		if (saved_count == 0 || oldest_access.isNull()) {
			unloadTimer->deleteLater(); // Удалиться позже
			unloadTimer = NULL; // А указатель очистим сейчас
			return;
		}
		// Таймер нужен
		if (unloadTimer != NULL) {
			if (unloadTimer->isActive()) {
				if (!update_interval)
					return; // Используем ленивый режим
				unloadTimer->stop();
			}
		} else {
			unloadTimer = new QTimer(this);
			unloadTimer->setSingleShot(true);
			connect(unloadTimer, SIGNAL(timeout()), this, SLOT(doAutoUnload()));
		}
		unloadTimer->start((autoUnloadInterval * 60 - (oldest_access.secsTo(curr_time) - 1)) * 1000);
	}
}

void GameMap::doAutoUnload()
{
	initUnloadTimer(true);
}

void GameMap::persParamChanged(int paramId, int, int)
{
	if (paramId == Pers::ParamCoordinates) {
		MapPos p = Pers::instance()->getCoordinates();
		// Добавляем элемент в карту
		addMapElement(p);
		// Устанавливаем новую позицию персонажа
		setPersPos(p);
	}
}

const QColor &GameMap::getPersPosColor() const
{
	return mapScene_->getPersPosColor();
}

void GameMap::setPersPosColor(const QColor &color)
{
	mapScene_->setPersPosColor(color);
}

QRectF GameMap::gameToSceneCoordinates(const MapPos &pos) const
{
	return mapScene_->mapToSceneCoordinates(pos);
}
