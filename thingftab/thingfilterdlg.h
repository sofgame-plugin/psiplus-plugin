/*
 * thingfilterdlg.h - Sof Game Psi plugin
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

#ifndef THINGFILTEREDIT_H
#define THINGFILTEREDIT_H

#include "ui_thingfilterdlg.h"

class ThingFilterEditDialog : public QDialog, public Ui::ThingFilterEdit
{
	Q_OBJECT
public:
	ThingFilterEditDialog(QWidget* parent, bool*, QString*);
	~ThingFilterEditDialog();

protected:
	bool* activePtr;
	QString* namePtr;

protected slots:
	void okBtnClick();

};

#endif // THINGFILTEREDIT_H
