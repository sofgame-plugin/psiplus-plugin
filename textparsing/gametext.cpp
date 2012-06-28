/*
 * gametext.cpp - Sof Game Psi plugin
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

#include <QTextDocument>

#include "gametext.h"
#include "utils.h"

GameText::GameText() :
	pos(0),
	savedPos(0)
{
}

GameText::GameText(const QString &text, bool html) :
	pos(0),
	savedPos(0)
{
	int start = 0;
	const int len = text.length();
	while (true) {
		int i = text.indexOf("\n", start, Qt::CaseSensitive);
		if (i != -1) {
			textArray.append(QPair<bool, QString>(html, text.mid(start, i - start)));
			start = i + 1;
			if (start == len) {
				textArray.append(QPair<bool, QString>(html, QString()));
				break;
			}
		} else {
			textArray.append(QPair<bool, QString>(html, text.mid(start)));
			break;
		}
	}
}

/**
 * Возвращает true если указатель массива находится за его пределами.
 * Т.е. нет больше текста для анализа
 */
bool GameText::isEnd() const
{
	return (pos >= textArray.size());
}

/**
 * Возвращает текущую строчку массива с текстом
 */
const QString &GameText::currentLine() const
{
	if (isEnd())
		return emptyString;
	return textArray.at(pos).second;
}

/**
 * Возвращает следующую строчку массива с текстом и передвигает указатель
 */
const QString &GameText::nextLine()
{
	if (isEnd())
		return emptyString;
	++pos;
	return currentLine();
}

/**
 * Возвращает указанную строку текста
 */
const QString &GameText::getLine(int i) const
{
	if (i < 0 || i >= textArray.size())
		return emptyString;
	return textArray.at(i).second;
}

/**
 * Перемещение указателя на предыдущую строчку
 */
void GameText::prior()
{
	if (pos > 0)
		--pos;
}

/**
 * Перемещение указателя на следующую строчку
 */
void GameText::next()
{
	++pos;
}

/**
 * Добавляет новую строку с текстом в конец массива
 * Положение указателя не меняется.
 * Может измениться свойство isEnd()
 */
void GameText::append(const QString &text, bool html)
{
	textArray.append(QPair<bool, QString>(html, text));
}

/**
 * Удаляет текущую строчку из массива
 * Текущей становится следующая строка
 */
void GameText::removeLine()
{
	if (!isEnd())
		textArray.removeAt(pos);
}

/**
 * Заменяет текущую строку с текстом на указанную.
 * Если html == true, то в дальнейшем текст обрабатывается как html
 */
void GameText::replace(const QString &text, bool html)
{
	if (!isEnd())
		replace(pos, text, html);
}

/**
 * Заменяет указанную строку с текстом значением из параметра text.
 * Если html == true, то в дальнейшем текст обрабатывается как html
 */
void GameText::replace(int i, const QString &text, bool html)
{
	textArray.replace(i, QPair<bool, QString>(html, text));
}

/**
 * Формирует текст для отображения с учетом атрибутов каждой строки
 */
QString GameText::toHtml() const
{
	QString resStr;
	for (int i = 0, cnt = textArray.size(); i < cnt; ++i) {
		if (i > 0)
			resStr.append("<br />");
		const QPair<bool, QString> &pair = textArray.at(i);
		if (!pair.first) {
			resStr.append(Qt::escape(pair.second));
		} else {
			resStr.append(pair.second);
		}
	}
	return resStr;
}
