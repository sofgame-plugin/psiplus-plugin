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

#include "mapscene.h"

#define ZValueLocation  3.0
#define ZValueMark      7.0
#define ZValueOtherPers 8.0
#define ZValuePersPos   9.0
#define ZValueMapName   10.0

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
QRectF MapScene::getMapSceneRect(double margin) const
{
	QRectF rect = QRectF();
	foreach (QGraphicsItem *item, items()) {
		if (item->isVisible())
			rect = rect.united(item->mapRectToScene(item->boundingRect()));
	}
	if (margin != 0.0f) {
		double mg1 = margin * -MAP_ELEMENT_SIZE;
		double mg2 = margin * MAP_ELEMENT_SIZE;
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
		double x = p.x();
		if (x < 0)
			x -= MAP_ELEMENT_SIZE;
		double y = p.y();
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
		resRect.setX(((double)p.x() - (double)nullElementPos.x()) * MAP_ELEMENT_SIZE);
		resRect.setY(((double)nullElementPos.y() - (double)p.y()) * MAP_ELEMENT_SIZE);
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
 * Отрисовывает основной элемент локации (кружочек)
 * Заливает цветом зависящим от наличия особенностей и врагов
 */
void MapScene::drawMapElement(const MapPos &pos, const MapElementFeature &feature, bool enemies, bool modif)
{
	if (!nullElementPos.isValid()) {
		// Нет начальной точки отсчета
		nullElementPos = pos;
	}
	// Расчитываем координаты сцены
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (modif) {
		// Удаляем старые элементы
		removeSceneElement(itemRect, ElementLocation);
	}
	QBrush brush(Qt::SolidPattern);
	if (enemies) {
		// На этом участке карты есть враги
		brush.setColor(Qt::red);
	} else {
		if (feature.testFlag(LocationPortal) || feature.testFlag(LocationSecret)) {
			// Есть секреты или порталы
			brush.setColor(Qt::green);
		} else {
			// Нет ничего
			brush.setColor(Qt::gray);
		}
	}
	qreal ellipseSize = MAP_ELEMENT_SIZE / 2; // Делим на целое, что бы исключить артефакты
	double x1 = itemRect.x();
	double y1 = itemRect.y();
	x1 += ellipseSize / 2;
	y1 += ellipseSize / 2;
	QGraphicsEllipseItem *gItem = addEllipse(x1, y1, ellipseSize, ellipseSize, QPen(Qt::black, 1.0f, Qt::SolidLine), brush);
	gItem->setZValue(ZValueLocation);
	gItem->setData(0, ElementLocation);
}

/**
 * Отрисовывает отметку на карте указанным цветом.
 * Если enable == false, то метка удаляется
 */
void MapScene::drawMark(const MapPos &pos, bool enable, const QColor &color)
{
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (!itemRect.isValid())
		return;
	removeSceneElement(itemRect, ElementMark);
	if (enable) {
		double ellipseSize = MAP_ELEMENT_SIZE / 2.0f;
		double x = itemRect.x() + itemRect.width() / 4.0f;
		double y = itemRect.y() + itemRect.height() / 4.0f;
		QPainterPath markPath = QPainterPath();
		markPath.moveTo(x + ellipseSize * 0.75f, y - 2.0f);
		markPath.lineTo(x + ellipseSize * 0.75f, y - 2.0f + ellipseSize * 0.5f);
		markPath.lineTo(x + ellipseSize * 1.25f, y - 2.0f);
		QColor c = color;
		if (!c.isValid())
			c = QColor(Qt::blue);
		QGraphicsPathItem *gMarkItem = addPath(markPath, QPen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		gMarkItem->setZValue(ZValueMark);
		gMarkItem->setData(0, ElementMark);
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
			double x = mapRect.x();
			double y = mapRect.y() - MAP_ELEMENT_SIZE;
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
			//persGraphicItem->update();
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
			double x1 = itemRect.width() * 3 / 8 - 1.0f;
			double y1 = itemRect.height() * 3 / 8 - 1.0f;
			double ellipseSize = MAP_ELEMENT_SIZE / 4;
			persGraphicItem = addEllipse(x1, y1, ellipseSize, ellipseSize, QPen(Qt::black, 1.0f, Qt::SolidLine), QBrush(persPosColor, Qt::SolidPattern));
			persGraphicItem->setZValue(ZValuePersPos);
		}
		// Перемещаем уже существующий элемент
		persGraphicItem->setPos(itemRect.x(), itemRect.y());
	}
}

/**
 * Непосредственно прорисовывает путь движения на карте для выбранного элемента (север)
 */
void MapScene::drawMapElementPathNorth(const MapPos &pos, int type, bool avaible)
{
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (!itemRect.isValid())
		return;
	double x1 = itemRect.x();
	double y1 = itemRect.y();
	QRectF rect1(x1 + MAP_ELEMENT_SIZE / 4.0f, y1 + MAP_ELEMENT_SIZE / 4.0f, MAP_ELEMENT_SIZE / 2.0f, MAP_ELEMENT_SIZE / 2.0f);
	removeSceneElement(rect1, ElementPathNorth);
	if (avaible || type == 2) {
		x1 += MAP_ELEMENT_SIZE / 2.0f;
		y1 += MAP_ELEMENT_SIZE / 10.f; // учитываем утолщение кончика
		double y2 = y1 + MAP_ELEMENT_SIZE / 4.0f;  // С запасом - утолщение на хвосте
		QPen pen = getPathPen(type, avaible);
		addLine(x1, y1, x1, y2, pen)->setData(0, ElementPathNorth);
	}
}

/**
 * Непосредственно прорисовывает путь движения на карте для выбранного элемента (юг)
 */
void MapScene::drawMapElementPathSouth(const MapPos &pos, int type, bool avaible)
{
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (!itemRect.isValid())
		return;
	double x1 = itemRect.x();
	double y1 = itemRect.y();
	QRectF rect1(x1 + MAP_ELEMENT_SIZE / 4.0f, y1 + MAP_ELEMENT_SIZE / 4.0f, MAP_ELEMENT_SIZE / 2.0f, MAP_ELEMENT_SIZE / 2.0f);
	removeSceneElement(rect1, ElementPathSouth);
	if (avaible || type == 2) {
		x1 += MAP_ELEMENT_SIZE / 2.0f;
		y1 += MAP_ELEMENT_SIZE * 0.9f;
		double y2 = y1 - MAP_ELEMENT_SIZE / 4.0f; // Это с запасом
		QPen pen = getPathPen(type, avaible);
		addLine(x1, y1, x1, y2, pen)->setData(0, ElementPathSouth);
	}
}

/**
 * Непосредственно прорисовывает путь движения на карте для выбранного элемента (запад)
 */
void MapScene::drawMapElementPathWest(const MapPos &pos, int type, bool avaible)
{
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (!itemRect.isValid())
		return;
	double x1 = itemRect.x();
	double y1 = itemRect.y();
	QRectF rect1(x1 + MAP_ELEMENT_SIZE / 4.0f, y1 + MAP_ELEMENT_SIZE / 4.0f, MAP_ELEMENT_SIZE / 2.0f, MAP_ELEMENT_SIZE / 2.0f);
	removeSceneElement(rect1, ElementPathWest);
	if (avaible || type == 2) {
		x1 += MAP_ELEMENT_SIZE / 10.0f;
		y1 += MAP_ELEMENT_SIZE / 2.0f;
		double x2 = x1 + MAP_ELEMENT_SIZE / 4.0f;
		QPen pen = getPathPen(type, avaible);
		addLine(x1, y1, x2, y1, pen)->setData(0, ElementPathWest);
	}
}

/**
 * Непосредственно прорисовывает путь движения на карте для выбранного элемента (восток)
 */
void MapScene::drawMapElementPathEast(const MapPos &pos, int type, bool avaible)
{
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (!itemRect.isValid())
		return;
	double x1 = itemRect.x();
	double y1 = itemRect.y();
	QRectF rect1(x1 + MAP_ELEMENT_SIZE / 4.0f, y1 + MAP_ELEMENT_SIZE / 4.0f, MAP_ELEMENT_SIZE / 2.0f, MAP_ELEMENT_SIZE / 2.0f);
	removeSceneElement(rect1, ElementPathEast);
	if (avaible || type == 2) {
		x1 += MAP_ELEMENT_SIZE * 0.9f;
		y1 += MAP_ELEMENT_SIZE / 2.0f;
		double x2 = x1 - MAP_ELEMENT_SIZE / 4.0f;
		QPen pen = getPathPen(type, avaible);
		addLine(x1, y1, x2, y1, pen)->setData(0, ElementPathEast);
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
		double x1 = itemRect.x() + itemRect.width() * 0.3f;
		double y1 = itemRect.y();
		double y2 = y1 + itemRect.height() * 0.5f;
		// Создаем элемент положения персонажа
		QGraphicsLineItem *lineItem = addLine(x1, y1, x1, y2, QPen(Qt::green, 2.0f, Qt::SolidLine));
		lineItem->setZValue(ZValueOtherPers);
		lineItem->setData(0, ElementOtherPers);
		// Отображаем надпись над маркером
		QFont font;
		font.setBold(true);
		font.setItalic(true);
		font.setPixelSize(itemRect.height() / 2.0f);
		QGraphicsTextItem *textItem = addText(list.join("\n"), font);
		textItem->setOpacity(0.7f);
		textItem->setZValue(ZValueOtherPers);
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
 * Установка всплавающей подсказки для элемента карты
 */
void MapScene::setTooltip(const MapPos &pos, const QString &tooltipStr)
{
	QRectF itemRect = mapToSceneCoordinates(pos);
	if (itemRect.isValid()) {
		foreach (QGraphicsItem *item, items(itemRect, Qt::IntersectsItemShape)) {
			if (item->data(0).toInt() == ElementLocation) {
				item->setToolTip(tooltipStr);
				break;
			}
		}
	}
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
