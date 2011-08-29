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
		};

	public:
		static GameMap *instance();
		static void reset();
		void init(QString);
		bool loadMapsList();
		int  createMap(QString* map_name);
		bool saveMap();
		int  exportMaps(QStringList, int, QString);
		int  importMaps(QString);
		bool removeMap(int);
		bool mergeMaps(int, int);
		int  getIndexByCoordinate(qreal, qreal);
		void moveMapElement(int, int, int);
		void removeMapElement(int, int);
		void addMapElement(qint32 x, qint32 y);
		QGraphicsScene* getGraphicsScene();
		void setPersPos(qint32 pers_x, qint32 pers_y);
		QRectF getSceneCoordinates(qint32 pers_x, qint32 pers_y);
		QGraphicsItem* getPersItem();
		void setMapElementPaths(qint32 pers_x, qint32 pers_y, int paths);
		void setMapElementEnemies(qint32 x, qint32 y, int count_min, int count_max);
		void setMapElementEnemiesList(qint32 x, qint32 y, QStringList enList);
		void setMapElementType(qint32 pers_x, qint32 pers_y, qint32 type);
		void mapsInfo(struct maps_info* mapsInfoPtr);
		void getMapsList(QVector<maps_list2>* maps_ls);
		bool switchMap(int);
		QRectF getMapRect();
		bool unloadMap(int);
		bool clearMap(int);
		int  renameMap(int, QString);
		MapElementMark getMapElementMark(int) const;
		void setMapElementMark(int, const QString &, const QColor &);
		void removeMapElementMark(int);
		void setOtherPersPos(QVector<GameMap::maps_other_pers>*);
		int  getMapsSettingParam(ParamId) const;
		void setMapsParam(ParamId, int);
		QDomElement exportMapsSettingsToDomElement(QDomDocument &xmlDoc) const;

	private:
		enum MapStatus {
			None,
			HeaderOnly,
			InMemory,
			NewMap
		};
		struct map_element {
			int     status;
			int     type;
			qint32  x;
			qint32  y;
			int     can_north;
			int     can_south;
			int     can_west;
			int     can_east;
			int     north_type;
			int     south_type;
			int     west_type;
			int     east_type;
			int     past_pers_pos;
			int     enemies_min;
			int     enemies_max;
			QStringList enemies_list;
			MapElementMark mark;
		};
		struct maps_list {
			MapStatus  status;
			QString    name;
			QString    old_name; // возможность идентификации при сохранении после переименования
			qint32     min_x;
			qint32     max_x;
			qint32     min_y;
			qint32     max_y;
			QVector<struct GameMap::map_element>* map;
			bool       modified;
			QDateTime  last_access;
		};
		struct other_pers {
			qint32          x;
			qint32          y;
			QStringList     names;
		};
		static GameMap *instace_;
		QString currAccJid;
		QVector<maps_list> mapsList;
		QList<struct other_pers> otherPers;
		qint32 null_element_x;
		qint32 null_element_y;
		qint32 null_pers_pos_x;
		qint32 null_pers_pos_y;
		qint32 lastX;
		qint32 lastY;
		qint32 persPosX;
		qint32 persPosY;
		int lastIndex;
		int persPosIndex;
		int mapCurrIndex;
		QVector<GameMap::map_element>* mapCurrArrayPtr;
		QGraphicsEllipseItem* persGraphicItem;
		int modifiedMapsCount;
		int saveMode;
		QTimer *saveTimer;
		int autoSaveInterval;   // В минутах
		QTimer *unloadTimer;
		int autoUnloadInterval; // В минутах

	private:
		GameMap(QObject *parent = 0);
		~GameMap();
		bool loadMap(int map_index);
		QDomNode makeMapXmlElement(QDomDocument xmlDoc, struct maps_list* map_head);
		bool makeMapFromDomNode(struct maps_list*, QDomNode);
		void selectMap(qint32 x, qint32 y);
		void redrawMap();
		void drawMapElement(int, bool);
		QString makeTooltipForMapElement(int);
		void setTooltipForMapElement(int, QString);
		void drawMapName();
		int  getMapElementIndex(int, qint32, qint32);
		void drawMapElementPathNorth(int element_index, bool modif);
		void drawMapElementPathSouth(int element_index, bool modif);
		void drawMapElementPathWest(int element_index, bool modif);
		void drawMapElementPathEast(int element_index, bool modif);
		void setPathPen(int map_index, int path_type, int can_move, QPen* pen);
		void clearOtherPersPos();
		void drawOtherPersPos(int);
		void initSaveTimer();
		void initUnloadTimer(bool);
		void loadMapsSettings(const QDomElement &xml);

	private slots:
		void doAutoSave();
		void doAutoUnload();

};

#endif
