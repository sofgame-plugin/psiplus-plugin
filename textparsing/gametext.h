/*
 * gametext.h - Sof Game Psi plugin
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

#ifndef GAMETEXT_H
#define GAMETEXT_H

#include <QString>
#include <QList>
#include <QPair>

class GameText
{
public:
	GameText();
	GameText(const QString &text, bool html);
	bool isEmpty() const {return textArray.isEmpty();}
	bool isFirst() const {return (pos == 0);}
	bool isEnd() const;
	void setEnd() {pos = textArray.size();}
	const QString &currentLine() const;
	const QString &nextLine();
	const QString &getLine(int i) const;
	void prior();
	void next();
	int  currentPos() const {return pos;}
	void append(const QString &text, bool html);
	void removeLine();
	void replace(const QString &text, bool html);
	void replace(int i, const QString &text, bool html);
	void savePos() {savedPos = pos;}
	void restorePos() {pos = savedPos;}
	QString toHtml() const;

private:
	QList< QPair<bool, QString> > textArray;
	int pos;
	int savedPos;
};

#endif // GAMETEXT_H
