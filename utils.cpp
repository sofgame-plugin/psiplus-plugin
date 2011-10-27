/*
 * utils.cpp - Sof Game Psi plugin
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

#include <QtCore>
#include <QDomDocument>
#include <QMessageBox>

#include "utils.h"
#include "pluginhosts.h"

QStringList thingTypeStrings = (QStringList() << QString::fromUtf8("вещь") << QString::fromUtf8("список") << QString::fromUtf8("снадобье") << QString::fromUtf8("дубины") << QString::fromUtf8("кинжал") << QString::fromUtf8("клинки") << QString::fromUtf8("шлем") << QString::fromUtf8("броня") << QString::fromUtf8("обувь") << QString::fromUtf8("щит") << QString::fromUtf8("штаны") << QString::fromUtf8("амулет") << QString::fromUtf8("браслет") << QString::fromUtf8("наплечники") << QString::fromUtf8("пояс"));
const QString emptyString;

bool savePluginXml(QDomDocument* xmlDoc, QString filename)
{
	// ***** Сохраняет переданный xml документ в файл с указанным именем в каталог psidata/sof_game
	if (PluginHosts::appInfoHost) {
		QString path = PluginHosts::appInfoHost->appHomeDir(ApplicationInfoAccessingHost::DataLocation) + QDir::separator() + "sof_game";
		return saveXmlToFile(xmlDoc, path + QDir::separator() + filename);
	}
	return false;
}

bool saveXmlToFile(QDomDocument* xmlDoc, QString filename)
{
	// ***** Сохраняет переданный xml документ в файл
	QString path = "";
	int sepPos = filename.lastIndexOf(QDir::separator());
	if (sepPos != -1) {
		path = filename.left(sepPos);
		QDir dirToSave(path);
		if (!dirToSave.exists()) {
			dirToSave.mkdir(path);
		}
	}
	if (!path.isEmpty())
		path.append(QDir::separator());
	QFile file(filename);
	if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QTextStream out(&file);
		//out.seek(file.size());
		xmlDoc->save(out, 2);
		//out << text << endl;
		out.setGenerateByteOrderMark(false);
		return true;
	}
	return false;
}

bool loadPluginXml(QDomDocument* xmlDoc, QString filename)
{
	QString sFile = PluginHosts::appInfoHost->appHomeDir(ApplicationInfoAccessingHost::DataLocation) + QDir::separator() + "sof_game" + QDir::separator() + filename;
	return loadXmlFromFile(xmlDoc, sFile);
}

bool loadXmlFromFile(QDomDocument* xmlDoc, QString filename)
{
	if (filename.isEmpty())
		return false;
	int errorLine;
	int errorColumn;
	QString errorStr;
	QFile file(filename);
	if (!xmlDoc->setContent(&file, true, &errorStr,  &errorLine, &errorColumn)) {
		qWarning("Line %d, column %d: %s", errorLine, errorColumn, errorStr.toAscii().data());
		return false;
	}
	return true;
}

QString getTextFromNode(QDomNode* nodePtr) {
	QDomNode childNode = nodePtr->firstChild();
	while (!childNode.isNull()) {
		if (childNode.nodeType() == QDomNode::TextNode) {
			return childNode.toText().data().trimmed();
		}
		childNode = childNode.nextSibling();
	}
	return "";
}

QString numToStr(qint64 number, QString sep)
{
	/**
	* Преобразует число в строку с разделителем
	**/
	qint64 num = number;
	bool lowF = false;
	if (num < 0) {
		lowF = true;
		num = num * -1;
	}
	QString str1 = QString::number(num);
	QString str2;
	int strLen = str1.length() - 3;
	if (!sep.isEmpty() && strLen > 0) {
		int i = strLen % 3;
		str2 = str1.left(i);
		for (; i <= strLen; i += 3) {
			if (!str2.isEmpty())
				str2.append(sep);
			str2.append(str1.mid(i, 3));
		}
	} else {
		str2 = str1;
	}
	if (lowF)
		str2 = "-" + str2;
	return str2;
}

/**
 * Преобразует числовой тип вещи в строку
 */
QString thingTypeToString(int type)
{
	if (type >= 0 && type < thingTypeStrings.size()) {
		return thingTypeStrings.at(type);
	}
	return "";
}

/**
 * Получает числовой тип вещи из строки
 */
int thingTypeFromString(QString type_str)
{
	return thingTypeStrings.indexOf(type_str.trimmed().toLower());
}

/**
 * Возвращает доступные строковые типы вещей разделенные запятой
 */
QString thingTypes()
{
	return thingTypeStrings.join(", ");
}

/**
 * Работает как стандартный сплит с пробелом, но учитывает экранированные пробелы и слеши
 * Например строка "Папа. Я\ и\ мама." будет разбита на две: "Папа.", "Я и мама."
 */
QStringList splitCommandString(const QString &str)
{
	QStringList resList;
	const int len = str.length();
	unsigned int slCnt = 0;
	int strPos = 0;
	for (int i = 0; i < len; i++) {
		QChar ch = str.at(i);
		if (ch == '\\') {
			slCnt++;
		} else {
			if (ch == ' ') {
				if ((slCnt & 1) == 0) { // четное кол-во слешей
					resList.append(str.mid(strPos, i - strPos).replace("\\ ", " ").replace("\\\\", "\\"));
					strPos = i + 1;
				}
			}
			slCnt = 0;
		}
	}
	if (strPos < len) {
		resList.push_back(str.mid(strPos).replace("\\ ", " ").replace("\\\\", "\\"));
	}
	return resList;
}
