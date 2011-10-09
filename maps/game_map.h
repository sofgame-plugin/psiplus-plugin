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
#include <QDomDocument>

#include "common.h"
#include "mapscene.h"
#include "maprect.h"
#include "mappos.h"

QT_BEGIN_NAMESPACE
	class QGraphicsEllipseItem;
	class QDomNode;
QT_END_NAMESPACE


class GameMap: public QObject
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
		int  getIndexByCoordinate(const QPointF &p);
		void moveMapElement(int, int, int);
		void removeMapElement(int, int);
		QGraphicsScene* getGraphicsScene();
		void setMapElementPaths(const MapPos &pos, int paths);
		void setMapElementEnemies(const MapPos &pos, int count_min, int count_max);
		void setMapElementEnemiesList(const MapPos &pos, const QStringList &enList);
		void setMapElementType(const MapPos &pos, const MapScene::MapElementFeature &feature);
		void mapsInfo(struct maps_info* mapsInfoPtr) const;
		void getMapsList(QVector<maps_list2>* maps_ls) const;
		bool switchMap(int);
		bool unloadMap(int);
		bool clearMap(int);
		int  renameMap(int, const QString &);
		MapElementMark getMapElementMark(int) const;
		void setMapElementMark(int, const QString &, const QColor &);
		void removeMapElementMark(int);
		void setOtherPersPos(QVector<GameMap::maps_other_pers>*);
		int  getMapsSettingParam(ParamId) const;
		void setMapsParam(ParamId, int);
		int  getUnloadInterval() const {return autoUnloadInterval;};
		void setUnloadInterval(int minutes);
		QDomElement exportMapsSettingsToDomElement(QDomDocument &xmlDoc) const;
		const QColor &getPersPosColor() const;
		void setPersPosColor(const QColor &color);
		QRectF gameToSceneCoordinates(const MapPos &pos) const;

	private:
		enum MapStatus {
			None,
			HeaderOnly,
			InMemory,
			NewMap
		};
		struct MapElement {
			int               status;
			MapScene::MapElementFeature feature;
			MapPos            pos;
			int               can_north;
			int               can_south;
			int               can_west;
			int               can_east;
			int               north_type;
			int               south_type;
			int               west_type;
			int               east_type;
			int               enemies_min;
			int               enemies_max;
			QStringList       enemies_list;
			MapElementMark    mark;
			MapElement() {}; // Для добавления в вектор
			MapElement(const MapPos &pos_, MapScene::MapElementFeature feature_ = MapScene::MapElementFeature()) :
				status(1),
				feature(feature_),
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
			MapPos      pos;
			QStringList names;
		};
		static GameMap *instace_;
		MapScene *mapScene_;
		QString currAccJid;
		QVector<MapInfo> mapsList;
		QList<OtherPers> otherPers;
		MapPos lastPos;
		MapPos persPos;
		int lastIndex;
		int persPosIndex;
		int mapCurrIndex;
		QVector<GameMap::MapElement>* mapCurrArrayPtr;
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
		void addMapElement(const MapPos &pos);
		void setPersPos(const MapPos &pos);
		QDomNode makeMapXmlElement(QDomDocument xmlDoc, const MapInfo &map_head) const;
		bool makeMapFromDomElement(MapInfo &, const QDomElement &);
		void selectMap(const MapPos &pos);
		void paintMap(MapScene *scene, int mapIndex);
		void redrawMap();
		QString makeTooltipForMapElement(int) const;
		int  getMapElementIndex(int mapIndex, const MapPos &pos);
		void clearOtherPersPos();
		void initSaveTimer();
		void initUnloadTimer(bool update_interval);
		void loadMapsSettings(const QDomElement &xml);

	private slots:
		void doAutoSave();
		void doAutoUnload();
		void persParamChanged(int, int, int);

};

#endif
