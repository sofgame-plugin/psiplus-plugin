/*
 * mapscene.cpp - Sof Game Psi plugin
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

#include <QtCore>

#include "mapscene.h"

#define ZValuePath          0.0
#define ZValueLocation      3.0
#define ZValueMark          7.0
#define ZValueOtherPersLine 8.0
#define ZValueOtherPersText 8.5
#define ZValuePersPos       9.0
#define ZValueMapName       10.0

// ------------- MapScene ----------------------------
MapScene::MapScene(QObject *parent) :
	QGraphicsScene(parent)
{
	init();
}

void MapScene::init()
{
	nullElementPos.reset();
	persGraphicItem = NULL;
	persPosColor = QColor(Qt::yellow);
}

void MapScene::clear()
{
	QGraphicsScene::clear();
	init();
}

/**
 * Обсчитывает координаты сцены включающие все видимые элементы сцены
 */
QRectF MapScene::getMapSceneRect(qreal margin) const
{
	QRectF rect = QRectF();
	foreach (QGraphicsItem *item, items()) {
		if (item->isVisible())
			rect = rect.united(item->mapRectToScene(item->boundingRect()));
	}
	if (margin != 0.0f) {
		qreal mg1 = margin * -MAP_ELEMENT_SIZE;
		qreal mg2 = margin * MAP_ELEMENT_SIZE;
		rect.adjust(mg1, mg1, mg2, mg2);
	}
	return rect;
}

/**
 * Преобразует координаты сцены в координаты сервера игры
 */
MapPos MapScene::sceneToMapCoordinates(const QPointF &p) const
{
	MapPos mapPos;
	if (nullElementPos.isValid()) {
		qreal x = p.x();
		if (x < 0)
			x -= MAP_ELEMENT_SIZE;
		qreal y = p.y();
		if (y >= 0)
			y += MAP_ELEMENT_SIZE;
		int mapX = x / MAP_ELEMENT_SIZE;
		mapX += nullElementPos.x();
		int mapY = y / MAP_ELEMENT_SIZE;
		mapY = nullElementPos.y() - mapY + 1;
		mapPos.setPos(mapX, mapY);
	}
	return mapPos;
}

/**
 * Преобразует координаты игры в координаты графической сцены
 */
QRectF MapScene::mapToSceneCoordinates(const MapPos &p) const
{
	QRectF resRect;
	if (nullElementPos.isValid()) {
		resRect.setX(((qreal)p.x() - (qreal)nullElementPos.x()) * MAP_ELEMENT_SIZE);
		resRect.setY(((qreal)nullElementPos.y() - (qreal)p.y()) * MAP_ELEMENT_SIZE);
		resRect.setWidth(MAP_ELEMENT_SIZE);
		resRect.setHeight(MAP_ELEMENT_SIZE);
	}
	return resRect;
}

/**
 * Удаляет все элементы карты указанного типа в указанном прямоугольнике
 */
void MapScene::removeSceneElement(const QRectF &rect, MapScene::ElementType type)
{
	QList<QGraphicsItem *> list;
	if (rect.isValid()) {
		list = items(rect, Qt::IntersectsItemShape);
	} else {
		list = items();
	}
	foreach (QGraphicsItem *gItem, list) {
		const int dataVal = gItem->data(0).toInt();
		if (dataVal == type) {
			removeItem(gItem);
			delete gItem;
		}
	}
}

/**
 * Отрисовывает название карты в верхней части сцены
 */
void MapScene::drawMapName(const QString &name)
{
	// Удаляем название карты, если существует
	removeSceneElement(QRectF(), ElementMapName);
	if (!name.isEmpty() && nullElementPos.isValid()) {
		// Создаем элемент с именем карты
		QFont font;
		font.setBold(true);
		font.setItalic(true);
		font.setPixelSize((int)MAP_ELEMENT_SIZE);
		QGraphicsTextItem *item = addText(name, font);
		if (item != NULL) {
			item->setOpacity(0.5f);
			item->setDefaultTextColor(QColor(70, 70, 255, 255));
			item->setZValue(ZValueMapName);
			item->setData(0, ElementMapName);
			QRectF mapRect = getMapSceneRect(0.0f);
			qreal x = mapRect.x();
			qreal y = mapRect.y() - MAP_ELEMENT_SIZE;
			item->setPos(x, y);
			if (item->collidingItems(Qt::IntersectsItemShape).size() > 0) {
				// Текст перекрывает элементы карты, поднимаем его выше
				item->setPos(x, y - MAP_ELEMENT_SIZE);
			}
		}
	}
}

void MapScene::setPersPosColor(const QColor &color)
{
	if (color != persPosColor) {
		persPosColor = color;
		if (persGraphicItem != NULL) {
			persGraphicItem->setBrush(QBrush(color, Qt::SolidPattern));
		}
	}
}

void MapScene::drawPersPos(const MapPos &pos)
{
	if (!nullElementPos.isValid())
		return;
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (itemRect.isValid()) {
		if (persGraphicItem == NULL) {
			// Создаем новый элемент положения персонажа
			qreal ellipseSize = qFloor(itemRect.width() / 4.0f);
			qreal x1 = qFloor((itemRect.width() - ellipseSize) / 2.0f);
			qreal y1 = qFloor((itemRect.height() - ellipseSize) / 2.0f);
			persGraphicItem = addEllipse(x1, y1, ellipseSize, ellipseSize, QPen(Qt::black, 1.0f, Qt::SolidLine), QBrush(persPosColor, Qt::SolidPattern));
			persGraphicItem->setZValue(ZValuePersPos);
			persGraphicItem->setData(0, ElementPersPos);
		}
		// Перемещаем уже существующий элемент
		persGraphicItem->setPos(itemRect.x(), itemRect.y());
	}
}

/**
 * Отмечает местоположение другого персонажа карте
 */
void MapScene::drawOtherPersPos(const MapPos &pos, const QStringList &list)
{
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (!itemRect.isValid())
		return;
	removeSceneElement(itemRect, ElementOtherPers);
	if (!list.isEmpty()) {
		qreal x1 = itemRect.x() + itemRect.width() * 0.3f;
		qreal y1 = itemRect.y();
		qreal y2 = y1 + itemRect.height() * 0.5f;
		// Создаем элемент положения персонажа
		QGraphicsLineItem *lineItem = addLine(x1, y1, x1, y2, QPen(Qt::green, 2.0f, Qt::SolidLine));
		lineItem->setZValue(ZValueOtherPersLine);
		lineItem->setData(0, ElementOtherPers);
		// Отображаем надпись над маркером
		QFont font;
		font.setBold(true);
		font.setItalic(true);
		font.setPixelSize(itemRect.height() / 2.0f);
		QGraphicsTextItem *textItem = addText(list.join("\n"), font);
		textItem->setOpacity(0.7f);
		textItem->setZValue(ZValueOtherPersText);
		textItem->setData(0, ElementOtherPers);
		textItem->setDefaultTextColor(QColor(32, 128, 32, 255));
		// Устанавливаем положение надписи
		x1 += itemRect.width() / 8.0f;
		y1 -= itemRect.height() / 2.0f;
		textItem->setPos(x1, y1);
	}
}

/**
 * Удаляет все элементы положения других игроков со сцены
 */
void MapScene::removePersPosElements()
{
	removeSceneElement(QRectF(), ElementOtherPers);
}

/**
 * Настраивает перо, в зависимости от типа пути
 */
QPen MapScene::getPathPen(int path_type, bool can_move) const
{
	QPen pen(Qt::gray);
	pen.setStyle(Qt::SolidLine);
	if (can_move != 0) {
		if (path_type == 2) {
			pen.setWidth(MAP_ELEMENT_SIZE / 8.0f);
		} else {
			pen.setWidth(MAP_ELEMENT_SIZE / 4.0f);
		}
	} else if (path_type == 2) {
		pen.setWidth(MAP_ELEMENT_SIZE / 16.0f);
	}
	return pen;
}

/**
 * Возвращает указатель на элемент карты для указанной позиции или NULL если не найден
 */
MapSceneItem *MapScene::getMapItem(const MapPos &pos) const
{
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (itemRect.isValid())
	{
		foreach (QGraphicsItem *gItem, items(itemRect, Qt::IntersectsItemShape))
		{
			if (gItem->data(0).toInt() == MapScene::ElementLocationRect)
				return (MapSceneItem *)gItem;
		}
	}
	return NULL;
}

void MapScene::setMapItem(MapSceneItem *item, bool replace)
{
	const MapPos &pos = item->mapPos();
	if (!nullElementPos.isValid()) {
		// Нет начальной точки отсчета
		nullElementPos = pos;
	}
	// Расчитываем координаты сцены
	QRectF itemRect = mapToSceneCoordinates(pos);
	// Удаление старого объекта
	if (replace)
	{
		MapSceneItem *oldItem = getMapItem(item->mapPos());
		if (oldItem)
			delete oldItem;
	}
	// Добавление на сцену
	addItem(item);
	item->setPos(itemRect.topLeft());
}

// ------------- MapSceneItem ------------------------

MapSceneItem::MapSceneItem(const MapPos &pos) : QGraphicsRectItem(NULL)
	, northLine(NULL)
	, southLine(NULL)
	, westLine(NULL)
	, eastLine(NULL)
	, markItem(NULL)
{
	mPos = pos;
	setRect(0.0f, 0.0f, MAP_ELEMENT_SIZE, MAP_ELEMENT_SIZE);

	qreal ellipseSize = qFloor(MAP_ELEMENT_SIZE * 0.5f);
	qreal margin = qFloor((MAP_ELEMENT_SIZE - ellipseSize) / 2.0f);
	centralCircle = new QGraphicsEllipseItem(0.0f, 0.0f, ellipseSize, ellipseSize, this);
	centralCircle->setPen(QPen(Qt::black, 1.0f, Qt::SolidLine));
	QBrush br(Qt::SolidPattern);
	br.setColor(Qt::gray);
	centralCircle->setBrush(br);
	centralCircle->setZValue(ZValueLocation);
	centralCircle->setData(0, MapScene::ElementLocation);
	centralCircle->setPos(margin, margin);
}

void MapSceneItem::setExtraInfo(MapScene::MapElementFeature feature, bool enemies)
{
	QBrush br = centralCircle->brush();
	if (enemies) {
		// На этом участке карты есть враги
		br.setColor(Qt::red);
	} else {
		if (feature.testFlag(MapScene::LocationPortal) || feature.testFlag(MapScene::LocationSecret)) {
			// Есть секреты или порталы
			br.setColor(Qt::green);
		} else {
			// Нет ничего
			br.setColor(Qt::gray);
		}
	}
	centralCircle->setBrush(br);
}

QPen MapSceneItem::getPathPen(int path_type, bool can_move)
{
	QPen p(Qt::gray);
	p.setStyle(Qt::SolidLine);
	if (can_move != 0) {
		if (path_type == 2) {
			p.setWidth(MAP_ELEMENT_SIZE / 8.0f);
		} else {
			p.setWidth(MAP_ELEMENT_SIZE / 4.0f);
		}
	} else if (path_type == 2) {
		p.setWidth(MAP_ELEMENT_SIZE / 16.0f);
	}
	return p;

}

/**
 * Непосредственно прорисовывает путь движения на карте для выбранного элемента (север)
 */
void MapSceneItem::setPathNorth(int type, bool avaible)
{
	if (northLine)
	{
		delete northLine;
		northLine = NULL;
	}

	if (avaible || type == 2)
	{
		qreal w =  MAP_ELEMENT_SIZE;
		qreal x1 = w / 2.0f;
		qreal y1 = w / 10.f; // учитываем утолщение кончика
		qreal y2 = y1 + w / 4.0f;  // С запасом - утолщение на хвосте
		northLine = new QGraphicsLineItem(x1, y1, x1, y2, this);
		northLine->setPen(getPathPen(type, avaible));
		northLine->setData(0, MapScene::ElementPathNorth);
		northLine->setZValue(ZValuePath);
	}
}

/**
 * Непосредственно прорисовывает путь движения на карте для выбранного элемента (юг)
 */
void MapSceneItem::setPathSouth(int type, bool avaible)
{
	if (southLine)
	{
		delete southLine;
		southLine = NULL;
	}

	if (avaible || type == 2)
	{
		qreal w = MAP_ELEMENT_SIZE;
		qreal x1 = w / 2.0f;
		qreal y1 = w * 0.9f;
		qreal y2 = y1 - w / 4.0f; // Это с запасом
		southLine = new QGraphicsLineItem(x1, y1, x1, y2, this);
		southLine->setPen(getPathPen(type, avaible));
		southLine->setData(0, MapScene::ElementPathSouth);
		southLine->setZValue(ZValuePath);
	}
}

/**
* Непосредственно прорисовывает путь движения на карте для выбранного элемента (запад)
*/
void MapSceneItem::setPathWest(int type, bool avaible)
{
	if (westLine)
	{
		delete westLine;
		westLine = NULL;
	}

	if (avaible || type == 2)
	{
		qreal w = MAP_ELEMENT_SIZE;
		qreal x1 = w / 10.0f;
		qreal y1 = w / 2.0f;
		qreal x2 = x1 + w / 4.0f;
		westLine = new QGraphicsLineItem(x1, y1, x2, y1, this);
		westLine->setPen(getPathPen(type, avaible));
		westLine->setData(0, MapScene::ElementPathWest);
		westLine->setZValue(ZValuePath);
	}
}

/**
* Непосредственно прорисовывает путь движения на карте для выбранного элемента (восток)
*/
void MapSceneItem::setPathEast(int type, bool avaible)
{
	if (eastLine)
	{
		delete eastLine;
		eastLine = NULL;
	}

	if (avaible || type == 2)
	{
		qreal w = MAP_ELEMENT_SIZE;
		qreal x1 = w * 0.9f;
		qreal y1 = w / 2.0f;
		qreal x2 = x1 - w / 4.0f;
		eastLine = new QGraphicsLineItem(x1, y1, x2, y1, this);
		eastLine->setPen(getPathPen(type, avaible));
		eastLine->setData(0, MapScene::ElementPathEast);
		eastLine->setZValue(ZValuePath);
	}
}

/**
 * Отрисовывает отметку на карте указанным цветом.
 * Если enable == false, то метка удаляется
 */
void MapSceneItem::setMark(bool enable, const QColor &color)
{
	if (markItem && !enable)
	{
		delete markItem;
		markItem = NULL;
		return;
	}

	if (!markItem)
	{
		qreal ellipseSize = MAP_ELEMENT_SIZE / 2.0f;
		qreal x = MAP_ELEMENT_SIZE / 4.0f;
		qreal y = MAP_ELEMENT_SIZE / 4.0f;
		QPainterPath markPath = QPainterPath();
		markPath.moveTo(x + ellipseSize * 0.75f, y - 2.0f);
		markPath.lineTo(x + ellipseSize * 0.75f, y - 2.0f + ellipseSize * 0.5f);
		markPath.lineTo(x + ellipseSize * 1.25f, y - 2.0f);
		markItem = new QGraphicsPathItem(markPath, this);
		markItem->setZValue(ZValueMark);
		markItem->setData(0, MapScene::ElementMark);
	}
	QColor c = color;
	if (!c.isValid())
		c = QColor(Qt::blue);
	markItem->setPen(QPen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void MapSceneItem::paint(QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/)
{
}
