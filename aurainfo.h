/*
 * aurainfo.h - Sof Game Psi plugin
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

#ifndef AURAINFO_H
#define AURAINFO_H

#include <QWidget>
#include <QLabel>

class AuraInfo : public QWidget
{
Q_OBJECT

public:
	AuraInfo(QWidget *parent = 0);
	void setShield(const QString &shield1, const QString &shield2);
	void setSword(const QString &sword1, const QString &sword2);
	void setPill(const QString &pill);

private:
	QPixmap shieldPixmap(bool enable);
	QPixmap swordPixmap(bool enable);
	QPixmap pillPixmap(bool enable);

private:
	QLabel *lbShield1;
	QLabel *lbShield2;
	QLabel *lbSword1;
	QLabel *lbSword2;
	QLabel *lbPill;

};

#endif // AURAINFO_H
