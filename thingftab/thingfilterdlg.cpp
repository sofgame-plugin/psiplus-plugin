/*
 * thingfilterdlg.cpp - Sof Game Psi plugin
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

#include <QMessageBox>

#include "thingfilterdlg.h"
//#include "../pers_info.h"


ThingFilterEditDialog::ThingFilterEditDialog(QWidget* parent, bool* enable_, QString* name_) :
	QDialog(parent)
{
	setupUi(this);
	if (*enable_)
		enabled->setCheckState(Qt::Checked);
	name->setText(*name_);
	activePtr = enable_;
	namePtr = name_;
	// Удалять диалог после закрытия
	//setAttribute(Qt::WA_DeleteOnClose);
}

ThingFilterEditDialog::~ThingFilterEditDialog()
{
}


void ThingFilterEditDialog::okBtnClick()
{
	QString name_str = name->text();
	if (name_str.trimmed().isEmpty()) {
		QMessageBox::warning(this, QString::fromUtf8("Сохранение фильтра"), QString::fromUtf8("Не заполнено имя фильтра"), QMessageBox::Ok);
		name->setFocus();
		return;
	}
	// Записываем данные из формы в указатели
	*namePtr = name_str;
	*activePtr = (enabled->checkState() == Qt::Checked) ? true : false;
	accept();
}
