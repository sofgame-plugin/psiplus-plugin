/*
 * aliases.cpp - Sof Game plugin
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

#include "aliases.h"
#include "settings.h"

Aliases::Aliases(QObject *parent):
	QObject(parent)
{
}

Aliases::~Aliases()
{
}

Aliases *Aliases::instance_ = NULL;

Aliases *Aliases::instance()
{
	if (!Aliases::instance_)
		Aliases::instance_ = new Aliases();
	return Aliases::instance_;
}

void Aliases::reset()
{
	if (Aliases::instance_) {
		delete Aliases::instance_;
		Aliases::instance_ = NULL;
	}
}

void Aliases::init()
{
	clear();
	loadFromDomElement(Settings::instance()->getAliasesData());
}

void Aliases::clear()
{
	aliasesList.clear();
}

QString Aliases::command(const QString &str) const
{
	for (int i = 0, cnt = aliasesList.size(); i < cnt; i++) {
		const AliasItem *aitem = &aliasesList.at(i);
		if (aitem->prefix) {
			if (str.startsWith(aitem->name)) {
				return aitem->command + str.mid(aitem->name.length());
			}
		} else {
			if (str == aitem->name) {
				return aitem->command;
			}
		}
	}
	return QString();
}

QString Aliases::aliasName(int i) const
{
	if (i >= 0 && i < aliasesList.size())
		return aliasesList.at(i).name;
	return QString();
}

bool Aliases::aliasPrefix(int i) const
{
	if (i >= 0 && i < aliasesList.size())
		return aliasesList.at(i).prefix;
	return false;
}

QString Aliases::aliasCommand(int i) const
{
	if (i >= 0 && i < aliasesList.size())
		return aliasesList.at(i).command;
	return QString();
}

bool Aliases::appendAlias(const QString &name, bool prefix, const QString &command)
{
	if (name.isEmpty() || command.isEmpty())
		return false; // Это понятно
	if (name.left(1) == "/" || (name.at(0) >= '0' && name.at(0) <= '9')) {
		return false; // алиас не должен начинаться со слеша или цифры
	}
	for (int i = 0, cnt = aliasesList.size(); i < cnt; i++) {
		if (name == aliasesList.at(i).name)
			return false; // Алиас с таким именем уже существует
	}
	AliasItem aitem;
	aitem.name = name;
	aitem.prefix = prefix;
	aitem.command = command;
	aliasesList.append(aitem);
	emit newAlias(aliasesList.size()-1);
	return true;
}

bool Aliases::removeAlias(int i)
{
	if (i >= 0 && i < aliasesList.size()) {
		aliasesList.removeAt(i);
		emit removed(i);
		return true;
	}
	return false;
}

void Aliases::loadFromDomElement(const QDomElement &el)
{
	if (!el.isNull() && el.tagName() == "aliases") {
		QDomElement child = el.firstChildElement("alias");
		while (!child.isNull()) {
			bool prefix = (child.attribute("prefix") == "yes");
			appendAlias(child.attribute("name"), prefix, child.text());
			child = child.nextSiblingElement("alias");
		}
	}
}

QDomElement Aliases::saveToDomElement(QDomDocument &xmlDoc)
{
	QDomElement eRes;
	const int cnt = aliasesList.size();
	if (cnt > 0) {
		eRes = xmlDoc.createElement("aliases");
		for (int i = 0; i < cnt; i++) {
			QDomElement child = xmlDoc.createElement("alias");
			const AliasItem *pItem = &aliasesList.at(i);
			child.setAttribute("name", pItem->name);
			if (pItem->prefix)
				child.setAttribute("prefix", "yes");
			child.appendChild(xmlDoc.createTextNode(pItem->command));
			eRes.appendChild(child);
		}
	}
	return eRes;
}
