/*
 * aliases.h - Sof Game plugin
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

#ifndef ALIASES_H
#define ALIASES_H

#include <QtCore>
#include <QDomElement>

class Aliases: public QObject
{
Q_OBJECT

public:
	static Aliases *instance();
	static void reset();
	QString command(const QString &) const;
	int count() const {return aliasesList.size();};
	QString aliasName(int) const;
	bool aliasPrefix(int) const;
	QString aliasCommand(int) const;
	bool appendAlias(const QString &name, bool prefix, const QString &command);
	bool removeAlias(int i);
	void loadFromDomElement(const QDomElement &);
	QDomElement saveToDomElement(QDomDocument &xmlDoc);

private:
	struct AliasItem {
		QString name;
		bool    prefix;
		QString command;
	};
	static Aliases *instance_;
	QList<struct AliasItem> aliasesList;

private:
	Aliases(QObject *parent = 0);
	~Aliases();
	void clear();

signals:
	void newAlias(int);
	void removed(int);

};

#endif // ALIASES_H
