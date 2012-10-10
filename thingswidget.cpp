/*
 * thingswidget.cpp - Sof Game Psi plugin
 * Copyright (C) 2012  Aleksey Andreev
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "thingswidget.h"
#include "utils.h"
#include "pers.h"

ThingsWidget::ThingsWidget(QWidget *parent) :
	QWidget(parent)
{
	setupUi(this);
	tabBar_ = new QTabBar(this);
	QLayout *lt = layout();
	lt->removeWidget(thingsTable);
	lt->removeItem(thingsSummaryLayout);
	lt->removeItem(moneySummaryLayout);
	lt->addWidget(tabBar_);
	lt->addWidget(thingsTable);
	lt->addItem(thingsSummaryLayout);
	lt->addItem(moneySummaryLayout);
	connect(tabBar_, SIGNAL(currentChanged(int)), this, SLOT(activateTab(int)));
	connect(thingsTable, SIGNAL(changeSummary()), this, SLOT(updateSummary()));
}

void ThingsWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		retranslateUi(this);
		break;
	default:
		break;
	}
}

void ThingsWidget::activateTab(int i)
{
	thingsTable->setFilter(tabBar_->tabData(i).toInt());
}

void ThingsWidget::updateTabs()
{
	disconnect(tabBar_, SIGNAL(currentChanged(int)), this, SLOT(activateTab(int)));

	int currTab = tabBar_->currentIndex();
	while (tabBar_->count() > 0)
		tabBar_->removeTab(0);

	tabBar_->setTabData(tabBar_->addTab(QString::fromUtf8("Все вещи")), -1);
	int fltrIdx = 0;
	const ThingFiltersList &thfList = Pers::instance()->thingsFiltersList();
	for (int i = 0, cnt = thfList.size(); i < cnt; ++i)
	{
		ThingFilter const *thf = thfList.at(i);
		if (thf->isActive())
			tabBar_->setTabData(tabBar_->addTab(thf->name()), fltrIdx);
		++fltrIdx;
	}
	if (currTab < 0 || currTab >= tabBar_->count())
		currTab = 0;

	tabBar_->setCurrentIndex(currTab);
	activateTab(currTab);

	connect(tabBar_, SIGNAL(currentChanged(int)), this, SLOT(activateTab(int)));
}

void ThingsWidget::updateSummary()
{
	labelThingsCountAll->setText(numToStr(thingsTable->summaryCount(), "'"));

	QString str1 = numToStr(thingsTable->summaryPriceAll(), "'");
	if (thingsTable->summaryNoPriceCount() != 0)
		str1.append("+");
	labelThingsPriceAll->setText(str1);
}
