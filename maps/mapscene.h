/*
 * mapscene.h - Sof Game Psi plugin
 * Copyright (C) 2011  Aleksey Andreev
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

#ifndef MAPSCENE_H
#define MAPSCENE_H

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QFlags>

#include "maps/mappos.h"

#define MAP_ELEMENT_SIZE  25


class MapScene : public QGraphicsScene
{
Q_OBJECT
public:
	enum LocationStatus {
		LocationPortal = 1,
		LocationSecret = 2
	};
	typedef QFlags<LocationStatus> MapElementFeature;
	MapScene(QObject *parent = 0);
	void clear();
	MapPos sceneToMapCoordinates(const QPointF &p) const;
	QRectF mapToSceneCoordinates(const MapPos &p) const;
	const QColor &getPersPosColor() const {return persPosColor;};
	void setPersPosColor(const QColor &color);
	void drawMapElement(const MapPos &pos, const MapElementFeature &feature, bool enemies, bool modif);
	void drawMapElementPathNorth(const MapPos &pos, int type, bool avaible);
	void drawMapElementPathSouth(const MapPos &pos, int type, bool avaible);
	void drawMapElementPathWest(const MapPos &pos, int type, bool avaible);
	void drawMapElementPathEast(const MapPos &pos, int type, bool avaible);
	void drawMark(const MapPos &pos, bool enable, const QColor &color);
	void drawPersPos(const MapPos &pos);
	void drawOtherPersPos(const MapPos &pos, const QStringList &list);
	void removePersPosElements();
	void setTooltip(const MapPos &pos, const QString &tooltipStr);
	void drawMapName(const QString &name);
	QRectF getMapSceneRect(double margin) const;

protected:

private:
	enum ElementType {
		ElementLocation,
		ElementMark,
		ElementPathNorth,
		ElementPathSouth,
		ElementPathWest,
		ElementPathEast,
		ElementMapName,
		ElementOtherPers
	};
	void init();
	void removeSceneElement(const QRectF &rect, ElementType type);
	QPen getPathPen(int path_type, bool can_move) const;

private:
	MapPos nullElementPos;
	QGraphicsEllipseItem *persGraphicItem;
	QColor persPosColor;

};

#endif // MAPSCENE_H
