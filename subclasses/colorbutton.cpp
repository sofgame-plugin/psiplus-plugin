/*
 * colorbutton.cpp - Sof Game Psi plugin
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

#include <QVariant>
#include <QColorDialog>
#include <QPainter>


#include "colorbutton.h"

ColorButton::ColorButton(QWidget *parent) :
	QToolButton(parent),
	color(QColor(Qt::black))
{
	setToolButtonStyle(Qt::ToolButtonIconOnly);
	setAutoRaise(true);
	setColorIcon();
	connect(this, SIGNAL(clicked()), this, SLOT(chooseColor()));
}

void ColorButton::chooseColor()
{
	QColor c = QColorDialog::getColor(color, this);
	if (c.isValid() && c != color) {
		color = c;
		setColorIcon();
	}
}

void ColorButton::setColor(const QColor &c)
{
	if (c.isValid() && c!= color) {
		color = c;
		setColorIcon();
	}
}

QColor ColorButton::getColor() const
{
	return color;
}

void ColorButton::setColorIcon()
{
	QPixmap pix(iconSize());
	QPainter p(&pix);
	p.fillRect(0, 0, pix.width(), pix.height(), QBrush(color));
	p.setPen(QColor(0, 0, 0));
	p.drawRect(0, 0, pix.width(), pix.height());
	p.end();
	setIcon(pix);
}
