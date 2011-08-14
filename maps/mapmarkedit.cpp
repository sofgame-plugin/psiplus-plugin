/*
 * mapmarkedit.cpp - Sof Game Psi plugin
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

#include "mapmarkedit.h"
#include "ui_mapmarkedit.h"

MapMarkEdit::MapMarkEdit(bool enabled, const QString &title, const QColor &color, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MapMarkEdit),
	res_(ResNone)
{
	ui->setupUi(this);
	if (enabled) {
		ui->lbMarkStatus->setText(QString::fromUtf8("Метка установлена"));
	} else {
		ui->lbMarkStatus->setText(QString::fromUtf8("Метка отсутствует"));
		ui->btnRemove->setEnabled(false);
	}
	ui->leMarkText->setText(title);
	if (color.isValid())
		ui->btnMarkColor->setColor(color);
	else
		ui->btnMarkColor->setColor(QColor(Qt::blue));
}

MapMarkEdit::~MapMarkEdit()
{
	delete ui;
}

void MapMarkEdit::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MapMarkEdit::save()
{
	res_ = ResSave;
	close();
}

void MapMarkEdit::remove()
{
	res_ = ResRemove;
	close();
}

int MapMarkEdit::getResult() const
{
	return res_;
}

QString MapMarkEdit::getMarkTitle() const
{
	return ui->leMarkText->text();
}

QColor MapMarkEdit::getMarkColor() const
{
	return ui->btnMarkColor->getColor();
}
