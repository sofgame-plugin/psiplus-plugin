/*
 * aurainfo.cpp - Sof Game Psi plugin
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

#include <QHBoxLayout>
#include <QPixmap>
#include <QIcon>

#include "aurainfo.h"
#include "pers.h"

AuraInfo::AuraInfo(QWidget *parent) : QWidget(parent)
{
	QHBoxLayout *hLay = new QHBoxLayout(this);
	hLay->setMargin(0);
	hLay->setSpacing(0);
	lbShield1 = new QLabel(parent);
	hLay->addWidget(lbShield1, 0);
	lbShield2 = new QLabel(parent);
	hLay->addWidget(lbShield2, 0);
	lbSword1 = new QLabel(parent, 0);
	hLay->addWidget(lbSword1);
	lbSword2 = new QLabel(parent, 0);
	hLay->addWidget(lbSword2, 0);
	lbPill = new QLabel(parent);
	hLay->addWidget(lbPill, 0);
	setShield(QString(), QString());
	setSword(QString(), QString());
	setPill(QString());
}

void AuraInfo::setShield(const QString &shield1, const QString &shield2)
{
	bool ena = (!shield1.isEmpty() && shield1 == Pers::instance()->citizenship());
	lbShield1->setPixmap(shieldPixmap(ena));
	QString tooltip = QString::fromUtf8("Поле предков запад");
	if (!shield1.isEmpty())
		tooltip.append("- " + shield1);
	lbShield1->setToolTip(tooltip);
	ena = (!shield2.isEmpty() && shield2 == Pers::instance()->citizenship());
	lbShield2->setPixmap(shieldPixmap(ena));
	tooltip = QString::fromUtf8("Поле предков восток");
	if (!shield2.isEmpty())
		tooltip.append("- " + shield2);
	lbShield2->setToolTip(tooltip);
}

void AuraInfo::setSword(const QString &sword1, const QString &sword2)
{
	bool ena = (!sword1.isEmpty() && sword1 == Pers::instance()->citizenship());
	lbSword1->setPixmap(swordPixmap(ena));
	QString tooltip = QString::fromUtf8("Холм героев запад");
	if (!sword1.isEmpty())
		tooltip.append("- " + sword1);
	lbSword1->setToolTip(tooltip);
	ena = (!sword2.isEmpty() && sword2 == Pers::instance()->citizenship());
	lbSword2->setPixmap(swordPixmap(ena));
	tooltip = QString::fromUtf8("Холм героев восток");
	if (!sword2.isEmpty())
		tooltip.append("- " + sword2);
	lbSword2->setToolTip(tooltip);
}

void AuraInfo::setPill(const QString &pill)
{
	bool ena = (!pill.isEmpty() && pill == Pers::instance()->citizenship());
	lbPill->setPixmap(pillPixmap(ena));
	QString tooltip = QString::fromUtf8("Призрачный фонтан");
	if (!pill.isEmpty())
		tooltip.append("- " + pill);
	lbPill->setToolTip(tooltip);
}

QPixmap AuraInfo::shieldPixmap(bool enable)
{
	QIcon ico(":aura/shield");
	QPixmap pix = ico.pixmap(QSize(24, 24), (enable) ? QIcon::Normal : QIcon::Disabled, QIcon::Off);
	return pix;
}

QPixmap AuraInfo::swordPixmap(bool enable)
{
	QIcon ico(":aura/sword");
	QPixmap pix = ico.pixmap(QSize(24, 24), (enable) ? QIcon::Normal : QIcon::Disabled, QIcon::Off);
	return pix;
}

QPixmap AuraInfo::pillPixmap(bool enable)
{
	QIcon ico(":aura/pill");
	QPixmap pix = ico.pixmap(QSize(24, 24), (enable) ? QIcon::Normal : QIcon::Disabled, QIcon::Off);
	return pix;
}
