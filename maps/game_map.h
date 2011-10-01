/*
 * game_map.h - Sof Game Psi plugin
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

#ifndef GAME_MAP_H
#define GAME_MAP_H

#include <QtCore>
#include <QGraphicsScene>
#include <QDomDocument>

#include "common.h"
#include "maprect.h"

QT_BEGIN_NAMESPACE
	class QGraphicsEllipseItem;
	class QDomNode;
QT_END_NAMESPACE


#define MAP_ELEMENT_SIZE  25

class GameMap: public QGraphicsScene
{
  Q_OBJECT

	public:
		enum ParamId {
			AutoSaveMode,
			AutoUnloadPeriod
		};
		enum MapElementType {
			TypeNormal,
			TypePortal,
			TypeSecret
		};
		struct maps_info {
			int      maps_count;
			int      maps_loaded;
			int      curr_map_index;
			QString  curr_map_name;
		};
		struct maps_other_pers {
			int      offset_x;
			int      offset_y;
			QString  name;
		};
		struct maps_list2 {
			int      index;
			QString  name;
			bool     loaded;
		};
		struct MapElementMark {
			bool	enabled;
			QString title;
			QColor  color;
			MapElementMark() : enabled(false) {};
		};

	public:
		static GameMap *instance();
		static void reset();
		void init(const QString &);
		bool loadMapsList();
		int  createMap(const QString &map_name);
		bool saveMap();
		int  exportMaps(const QStringList &, int, const QString &);
		int  importMaps(const QString &);
		bool removeMap(int);
		bool mergeMaps(int, int);
		int  getIndexByCoordinate(qreal, qreal);
		void moveMapElement(int, int, int);
		void removeMapElement(int, int);
		QGraphicsScene* getGraphicsScene();
		QRectF getSceneCoordinates(int pers_x, int pers_y) const;
		QGraphicsItem* getPersItem() const;
		void setMapElementPaths(int pers_x, int pers_y, int paths);
		void setMapElementEnemies(int x, int y, int count_min, int count_max);
		void setMapElementEnemiesList(int x, int y, const QStringList &enList);
		void setMapElementType(int pers_x, int pers_y, MapElementType type);
		void mapsInfo(struct maps_info* mapsInfoPtr) const;
		void getMapsList(QVector<maps_list2>* maps_ls) const;
		bool switchMap(int);
		QRectF getMapRect() const;
		bool unloadMap(int);
		bool clearMap(int);
		int  renameMap(int, const QString &);
		MapElementMark getMapElementMark(int) const;
		void setMapElementMark(int, const QString &, const QColor &);
		void removeMapElementMark(int);
		void setOtherPersPos(QVector<GameMap::maps_other_pers>*);
		int  getMapsSettingParam(ParamId) const;
		void setMapsParam(ParamId, int);
		const QColor &getPersPosColor() const {return persPosColor;};
		void setPersPosColor(const QColor &);
		int  getUnloadInterval() const {return autoUnloadInterval;};
		void setUnloadInterval(int minutes);
		QDomElement exportMapsSettingsToDomElement(QDomDocument &xmlDoc) const;

	private:
		enum MapStatus {
			None,
			HeaderOnly,
			InMemory,
			NewMap
		};
		struct MapElement {
			int            status;
			MapElementType type;
			QPoint         pos;
			int            can_north;
			int            can_south;
			int            can_west;
			int            can_east;
			int            north_type;
			int            south_type;
			int            west_type;
			int            east_type;
			int            enemies_min;
			int            enemies_max;
			QStringList    enemies_list;
			MapElementMark mark;
			MapElement() {}; // Для добавления в вектор
			MapElement(MapElementType type_, const QPoint &pos_) :
				status(1),
				type(type_),
				pos(pos_),
				can_north(0), can_south(0), can_west(0), can_east(0),
				north_type(0), south_type(0), west_type(0), east_type(0),
				enemies_min(-1), enemies_max(0)
			{};
		};
		struct MapInfo {
			MapStatus  status;
			QString    name;
			QString    old_name; // возможность идентификации при сохранении после переименования
			MapRect    rect;
			QVector<struct GameMap::MapElement>* map;
			bool       modified;
			QDateTime  last_access;
			MapInfo() {}; // Для добавления в вектор
			MapInfo(MapStatus status_, const QString &name_) :
				status(status_), name(name_),
				map(NULL),
				modified(false)
			{};
		};
		struct OtherPers {
			QPoint      pos;
			QStringList names;
		};
		static GameMap *instace_;
		QString currAccJid;
		QVector<MapInfo> mapsList;
		QList<OtherPers> otherPers;
		int null_element_x;
		int null_element_y;
		int null_pers_pos_x;
		int null_pers_pos_y;
		QPoint lastPos;
		QPoint persPos;
		int lastIndex;
		int persPosIndex;
		int mapCurrIndex;
		QVector<GameMap::MapElement>* mapCurrArrayPtr;
		QGraphicsEllipseItem* persGraphicItem;
		int modifiedMapsCount;
		int saveMode;
		QTimer *saveTimer;
		int autoSaveInterval;   // В минутах
		QTimer *unloadTimer;
		int autoUnloadInterval; // В минутах
		QColor persPosColor;

	private:
		GameMap(QObject *parent = 0);
		~GameMap();
		bool loadMap(int map_index);
		void addMapElement(const QPoint &pos);
		void setPersPos(const QPoint &pos);
		QDomNode makeMapXmlElement(QDomDocument xmlDoc, const MapInfo &map_head) const;
		bool makeMapFromDomElement(MapInfo &, const QDomElement &);
		void selectMap(const QPoint &pos);
		void redrawMap();
		void drawMapElement(int, bool);
		QString makeTooltipForMapElement(int) const;
		void setTooltipForMapElement(int, const QString &);
		void drawMapName();
		int  getMapElementIndex(int mapIndex, const QPoint &pos);
		void drawMapElementPathNorth(int element_index, bool modif);
		void drawMapElementPathSouth(int element_index, bool modif);
		void drawMapElementPathWest(int element_index, bool modif);
		void drawMapElementPathEast(int element_index, bool modif);
		void setPathPen(int map_index, int path_type, int can_move, QPen* pen);
		void clearOtherPersPos();
		void drawOtherPersPos(int);
		void initSaveTimer();
		void initUnloadTimer(bool update_interval);
		void loadMapsSettings(const QDomElement &xml);

	private slots:
		void doAutoSave();
		void doAutoUnload();
		void persParamChanged(int, int, int);

};

#endif
