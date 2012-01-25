/*
* thing.cpp - Sof Game Psi plugin
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

#include <QTextDocument>

#include "thing.h"
#include "pers_info.h"
#include "utils.h"

QRegExp thingElementReg(QString::fromUtf8("^(([0-9]+)- )?(.+)\\((\\w+)\\)((.+)\\{Треб:(.+)\\}(И:([0-9]+)ур\\.)?)?(- ([0-9]+)шт\\.)?$"));
QRegExp thingParamElementReg(QString::fromUtf8("^(\\w+):([0-9.]+)(\\*\\w+)?$"));
QRegExp thingRequElementReg(QString::fromUtf8("(\\D+)([0-9.]+)"));


Thing::Thing()
{
	valid_status = Invalid;
	init();
}

Thing::Thing(const QString thing_str)
{
	valid_status = Invalid;
	init();
	if (thingElementReg.indexIn(thing_str, 0) == -1)
		return;
	valid_status = Unknow;
	// Основные данные вещи
	QString sType = thingElementReg.cap(4).trimmed();
	type_ = thingTypeFromString(sType);
	if (!thingElementReg.cap(1).isEmpty()) {
		number_ = thingElementReg.cap(2).toInt();
	}
	name_ = thingElementReg.cap(3).trimmed();
	if (!thingElementReg.cap(8).isEmpty()) {
		up_level = thingElementReg.cap(9).toInt();
	}
	if (!thingElementReg.cap(10).isEmpty()) {
		count_ = thingElementReg.cap(11).toInt();
	} else {
		count_ = 1;
	}
	// Модификаторы (параметры)
	QString sParam = thingElementReg.cap(6).trimmed();
	if (!sParam.isEmpty()) {
		QStringList aParams = sParam.split(";", QString::SkipEmptyParts);
		QStringList aUnknowParam;
		//aUnknowParam.clear();
		int cnt = aParams.size();
		for (int i = 0; i < cnt; i++) {
			if (thingParamElementReg.indexIn(aParams[i], 0) != -1) {
				sParam = thingParamElementReg.cap(1);
				if (sParam == QString::fromUtf8("урон")) {
					if (thingParamElementReg.cap(3).isEmpty()) {
						loss_ += thingParamElementReg.cap(2).toInt();
					} else {
						loss_mul += thingParamElementReg.cap(2).toFloat();
					}
				} else if (sParam == QString::fromUtf8("защ")) {
					if (thingParamElementReg.cap(3).isEmpty()) {
						protect_ += thingParamElementReg.cap(2).toInt();
					} else {
						protect_mul += thingParamElementReg.cap(2).toFloat();
					}
				} else if (sParam == QString::fromUtf8("сила")) {
					if (thingParamElementReg.cap(3).isEmpty()) {
						force_ += thingParamElementReg.cap(2).toInt();
					} else {
						force_mul += thingParamElementReg.cap(2).toFloat();
					}
				} else if (sParam == QString::fromUtf8("ловк")) {
					if (thingParamElementReg.cap(3).isEmpty()) {
						dext_ += thingParamElementReg.cap(2).toInt();
					} else {
						dext_mul += thingParamElementReg.cap(2).toFloat();
					}
				} else if (sParam == QString::fromUtf8("инт")) {
					if (thingParamElementReg.cap(3).isEmpty()) {
						intell_ += thingParamElementReg.cap(2).toInt();
					} else {
						intell_mul += thingParamElementReg.cap(2).toFloat();
					}
				} else {
					aUnknowParam.push_back(aParams[i]);
				}
			} else {
				aUnknowParam.push_back(aParams[i]);
			}
		}
		if (aUnknowParam.size() > 0) {
			param_str = aUnknowParam.join(";");
		}
	}
	// Требования
	QString sRequ = thingElementReg.cap(7).trimmed();
	if (!sRequ.isEmpty()) {
		int pos = 0;
		while ((pos = thingRequElementReg.indexIn(sRequ, pos)) != -1) {
			QString sReqName = thingRequElementReg.cap(1).trimmed(); // Название требования
			if (sReqName == QString::fromUtf8("Ур")) {
				req_level = thingRequElementReg.cap(2).toInt();
			} else if (sReqName == QString::fromUtf8("Сил")) {
				req_force = thingRequElementReg.cap(2).toInt();
			} else if (sReqName == QString::fromUtf8("Ловк")) {
				req_dext = thingRequElementReg.cap(2).toInt();
			} else if (sReqName == QString::fromUtf8("Инт")) {
				req_intell = thingRequElementReg.cap(2).toInt();
			}
			pos += thingRequElementReg.matchedLength();
		}
	}
}

Thing::~Thing()
{
}

/**
 * Инициирует параметры вещей
 */
void Thing::init()
{
	type_ = -1;
	dressed_ = false;
	number_ = 0;
	name_ = "";
	param_str = "";
	up_level = 0;
	count_ = 0;
	price_ = -1;
	loss_ = 0;
	loss_mul = 0.0f;
	protect_ = 0;
	protect_mul = 0.0f;
	force_ = 0;
	force_mul = 0.0f;
	dext_ = 0;
	dext_mul = 0.0f;
	intell_ = 0;
	intell_mul = 0.0f;
	req_level = 0;
	req_force = 0;
	req_dext = 0;
	req_intell = 0;
}

bool Thing::isEqual(const Thing* oth_thing) const
{
	// !! Цена не сверяется !!
	// Если указатели равны, то и вещь одна и та же
	if (this == oth_thing)
		return true;
	// Наиболее вероятные расхождения сверяем первыми
	if (type_ == oth_thing->type() && number_ == oth_thing->number() && name_ == oth_thing->name()) {
		if (count_ == oth_thing->count() && up_level == oth_thing->uplevel() && dressed_ == oth_thing->isDressed()) {
			if (loss_ == oth_thing->loss() && loss_mul == oth_thing->lossmul()) {
				if (protect_ == oth_thing->protect() && protect_mul == oth_thing->protectmul()) {
					if (force_ == oth_thing->force() && force_mul == oth_thing->forcemul()) {
						if (dext_ == oth_thing->dext() && dext_mul == oth_thing->dextmul()) {
							if (intell_ == oth_thing->intell() && intell_mul == oth_thing->intellmul()) {
								if (req_level == oth_thing->reqlevel() && req_force == oth_thing->reqforce()) {
									if (req_dext == oth_thing->reqdext() && req_intell == oth_thing->reqintell()) {
										if (param_str == oth_thing->othermodif()) {
											return true;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool Thing::isValid() const
{
	if (valid_status != Unknow) {
		if (valid_status == Valid) {
			return true;
		}
		return false;
	}
	valid_status = Invalid;
	if (type_ != -1) {
		if (!name_.isEmpty()) {
			if (type_ <= 1 || loss_ != 0 || loss_mul != 0.0 || protect_ != 0 || protect_mul != 0.0 || force_ != 0 || force_mul != 0.0 || dext_ != 0 || dext_mul != 0.0 || intell_ != 0 || intell_mul != 0.0 || !param_str.isEmpty()) {
				valid_status = Valid;
				return true;
			}
		}
	}
	return false;
}

void Thing::setName(QString &new_name)
{
	name_ = new_name;
	valid_status = Unknow;
}

void Thing::setType(int _type)
{
	type_ = _type;
	valid_status = Unknow;
}

void Thing::setCount(int _count)
{
	if (_count < 0) {
		count_ = 0;
	} else {
		count_ = _count;
	}
}

void Thing::setPrice(int new_price)
{
	if (new_price >= -1) {
		price_ = new_price;
	} else {
		price_ = -1;
	}
}

void Thing::setLoss(int _loss)
{
	loss_ = _loss;
	valid_status = Unknow;
}

void Thing::setLossmul(float _lossmul)
{
	loss_mul = _lossmul;
	valid_status = Unknow;
}

void Thing::setProtect(int _protect)
{
	protect_ = _protect;
	valid_status = Unknow;
}

void Thing::setProtectmul(float _protectmul)
{
	protect_mul = _protectmul;
	valid_status = Unknow;
}

void Thing::setForce(int _force)
{
	force_ = _force;
	valid_status = Unknow;
}

void Thing::setForcemul(float _forcemul)
{
	force_mul = _forcemul;
	valid_status = Unknow;
}

void Thing::setDext(int _dext)
{
	dext_ = _dext;
	valid_status = Unknow;
}

void Thing::setDextmul(float _dextmul)
{
	dext_mul = _dextmul;
	valid_status = Unknow;
}

void Thing::setIntell(int _intell)
{
	intell_ = _intell;
	valid_status = Unknow;
}

void Thing::setIntellmul(float _intellmul)
{
	intell_mul = _intellmul;
	valid_status = Unknow;
}

void Thing::setOthermodif(QString &_othermodif)
{
	param_str = _othermodif;
	valid_status = Unknow;
}

/**
 * Генерация строки с описанием вещи
 */
QString Thing::toString(QFlags<enum ToStringFlag> flags) const
{
	QString res = "";
	if (!isValid()) return res;
	if (flags.testFlag(ShowNumber)) {
		if (number_ > 0) {
			res.append(QString::number(number_) + "- "); // Номер вещи
		}
	}
	if (flags.testFlag(ShowName)) {
		res.append(name_); // Имя вещи
	}
	if (flags.testFlag(ShowType)) {
		QString sType = thingTypeToString(type_);
		if (!sType.isEmpty()) {
			if (flags == ShowType) {
				// Запрос только типа вещи
				res.append(sType);
			} else {
				res.append("(" + sType + ")");
			}
		}
	}
	if (flags.testFlag(ShowModif)) {
		// Параметры и модификаторы
		QStringList aModifList;
		aModifList.clear();
		if (loss_mul != 0.0f || loss_ != 0) {
			aModifList.push_back(QString::fromUtf8("урон:%1").arg(paramToStr(loss_mul, loss_)));
		}
		if (protect_mul != 0.0f || protect_ != 0) {
			aModifList.push_back(QString::fromUtf8("защ:%1").arg(paramToStr(protect_mul, protect_)));
		}
		if (force_mul != 0.0f || force_ != 0) {
			aModifList.push_back(QString::fromUtf8("сила:%1").arg(paramToStr(force_mul, force_)));
		}
		if (dext_mul != 0.0f || dext_ != 0) {
			aModifList.push_back(QString::fromUtf8("ловк:%1").arg(paramToStr(dext_mul, dext_)));
		}
		if (intell_mul != 0.0f || intell_ != 0) {
			aModifList.push_back(QString::fromUtf8("инт:%1").arg(paramToStr(intell_mul, intell_)));
		}
		//--
		if (aModifList.size() > 0 || !param_str.isEmpty()) {
			res.append(" ");
		}
		res.append(aModifList.join(";"));
		if (!param_str.isEmpty()) {
			if (aModifList.size() > 0) {
				res.append(";");
			}
			res.append(param_str);
		}
	}
	if (flags.testFlag(ShowReq)) {
		// Требования {Треб:Ур7Сил20Ловк15Инт10}
		if (req_level != 0 || req_force != 0 || req_dext != 0 || req_intell != 0) {
			res.append(QString::fromUtf8(" {Треб:"));
			res.append(QString::fromUtf8("Ур%1").arg(req_level));
			if (req_force != 0) {
				res.append(QString::fromUtf8("Сил%1").arg(req_force));
			}
			if (req_dext != 0) {
				res.append(QString::fromUtf8("Ловк%1").arg(req_dext));
			}
			if (req_intell != 0) {
				res.append(QString::fromUtf8("Инт%1").arg(req_intell));
			}
			res.append("}");
		}
	}
	if (flags.testFlag(ShowUplevel)) {
		// Уровень именной
		if (up_level > 0) {
			res.append(QString::fromUtf8("И:%1ур.").arg(up_level));
		}
	}
	if (flags.testFlag(ShowCount)) {
		// Количество
		res.append(QString::fromUtf8("- %1шт.").arg(count_));
	}
	return res;
}

QString Thing::paramToStr(float mul, int abs)
{
	QString str1;
	if (mul != 0.0f) {
		str1 = QString::number(mul);
		if (str1.indexOf('.') == -1)
			str1.append(".0");
		str1.append(QString::fromUtf8("*ур"));
	}
	if (abs != 0) {
		if (mul != 0.0f) {
			str1.append("+");
		}
		str1.append(QString::number(abs));
	}
	if (str1.isEmpty())
		str1 = QString::fromUtf8("нет");
	return str1;
}

QString Thing::toTip() const
{
	QString tipStr;
	bool mainParam = (loss_mul != 0.0f || loss_ != 0 || protect_mul != 0.0f || protect_ != 0 || force_mul != 0.0f || force_ != 0 || dext_mul != 0.0f || dext_ != 0 || intell_mul != 0.0f || intell_ != 0);
	if (mainParam || !param_str.isEmpty()) {
		tipStr.append(QString::fromUtf8("<tr><td colspan=\"2\"><strong>Параметры</strong></td></tr>"));
		if (mainParam) {
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Урон:</div></td><td>%1</td></tr>").arg(paramToStr(loss_mul, loss_)));
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Защита:</div></td><td>%1</td></tr>").arg(paramToStr(protect_mul, protect_)));
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Сила:</div></td><td>%1</td></tr>").arg(paramToStr(force_mul, force_)));
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Ловкость:</div></td><td>%1</td></tr>").arg(paramToStr(dext_mul, dext_)));
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Интеллект:</div></td><td>%1</td></tr>").arg(paramToStr(intell_mul, intell_)));
		}
		if (!param_str.isEmpty()) {
			QStringList aOtherModifList = param_str.split(';');
			for (int i = 0, cnt = aOtherModifList.size(); i < cnt; ++i) {
				const QString str1 = aOtherModifList.at(i);
				int splitPos = str1.indexOf(':');
				if (splitPos > 0 && splitPos < str1.length() - 1) {
					tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">%1:</div></td><td>%2</td></tr>")
						.arg(str1.left(splitPos)).arg(str1.mid(splitPos + 1)));
				}
			}
		}
	}
	if (req_level != 0 || req_force != 0 || req_dext != 0 || req_intell != 0) {
		tipStr.append(QString::fromUtf8("<tr><td colspan=\"2\"><strong>Требования</strong></td></tr>"));
		if (req_level != 0)
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Уровень:</div></td><td>%1</td></tr>").arg(req_level));
		if (req_force != 0)
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Сила:</div></td><td>%1</td></tr>").arg(req_force));
		if (req_dext != 0)
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Ловкость:</div></td><td>%1</td></tr>").arg(req_dext));
		if (req_intell != 0)
			tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Интеллект:</div></td><td>%1</td></tr>").arg(req_intell));
	}
	if (up_level != 0) {
		tipStr.append(QString::fromUtf8("<tr><td colspan=\"2\"><strong>Именная</strong></td></tr>"));
		tipStr.append(QString::fromUtf8("<tr><td><div class=\"layer2\">Уровень:</div></td><td>%1</td></tr>").arg(up_level));
	}

	QString resStr = "<qt><style type='text/css'>"
			".layer1 {white-space:pre}"
			".layer2 {white-space:normal;margin-left:16px;}"
			"</style>";
	resStr.append(QString("<table><tr><td colspan=\"2\"><div class=\"layer1\"><strong><em><big>%1 (%2)</big></em></strong></div>%3</td></tr>")
			.arg(Qt::escape(name_))
			.arg(Qt::escape(thingTypeToString(type_)))
			.arg(tipStr.isEmpty() ? QString() : "<hr />"));
	resStr.append(tipStr);
	tipStr.append("</table></qt>");
	return resStr;
}

/**
 * Импортирует данные о вещи из XML
 */
void  Thing::importFromXml(const QDomElement &eThing)
{
	valid_status = Unknow;
	bool fOk;
	int i = eThing.attribute("number").toInt(&fOk);
	if (fOk && i > 0) {
		number_ = i;
		name_ = eThing.attribute("name");
		i = eThing.attribute("count").toInt(&fOk);
		if (!fOk || i <= 0) {
			i = 1;
		}
		count_ = i;
		type_ = thingTypeFromString(eThing.attribute("type"));
		i = eThing.attribute("named-level").toInt(&fOk);
		if (!fOk || i < 0)
			i = 0;
		up_level = i;
		if (eThing.attribute("dressed").toLower() == "yes") {
			dressed_ = true;
		} else {
			dressed_ = false;
		}
		QDomElement thingChild = eThing.firstChildElement();
		while (!thingChild.isNull()) {
			QString sThingChildName = thingChild.tagName();
			if (sThingChildName == "modifiers") {
				// Модификаторы вещи
				QDomElement modifChild = thingChild.firstChildElement();
				while (!modifChild.isNull()) {
					QString sModifChidName = modifChild.tagName();
					if (sModifChidName == "loss") {
						loss_ = modifChild.text().toInt();
					} else if (sModifChidName == "loss-mul") {
						loss_mul = modifChild.text().toFloat();
					} else if (sModifChidName == "protect") {
						protect_ = modifChild.text().toInt();
					} else if (sModifChidName == "protect-mul") {
						protect_mul = modifChild.text().toFloat();
					} else if (sModifChidName == "force") {
						force_ = modifChild.text().toInt();
					} else if (sModifChidName == "force-mul") {
						force_mul = modifChild.text().toFloat();
					} else if (sModifChidName == "dext") {
						dext_ = modifChild.text().toInt();
					} else if (sModifChidName == "dext-mul") {
						dext_mul = modifChild.text().toFloat();
					} else if (sModifChidName == "intell") {
						intell_ = modifChild.text().toInt();
					} else if (sModifChidName == "intell-mul") {
						intell_mul = modifChild.text().toFloat();
					} else if (sModifChidName == "other-modif") {
						param_str = modifChild.text();
					}
					modifChild = modifChild.nextSiblingElement();
				}
			} else if (sThingChildName == "requirements") {
				// Требования вещи
				QDomElement requChild = thingChild.firstChildElement();
				while (!requChild.isNull()) {
					QString sRequChidName = requChild.tagName();
					if (sRequChidName == "level") {
						req_level = requChild.text().toInt();
					} else if (sRequChidName == "force") {
						req_force = requChild.text().toInt();
					} else if (sRequChidName == "dext") {
						req_dext = requChild.text().toInt();
					} else if (sRequChidName == "intell") {
						req_intell = requChild.text().toInt();
					}
					requChild = requChild.nextSiblingElement();
				}
			}
			thingChild = thingChild.nextSiblingElement();
		}
	}
}

/**
 * Экспортирует параметры вещи в DOM элемент
 */
QDomElement Thing::exportToXml(QDomDocument &xmlDoc) const
{
	QDomElement eThing = xmlDoc.createElement("fing");
	eThing.setAttribute("number", QString::number(number_));
	eThing.setAttribute("name", name_.trimmed());
	QString sType = thingTypeToString(type_);
	if (!sType.isEmpty()) {
		eThing.setAttribute("type", sType);
	}
	eThing.setAttribute("count", QString::number(count_));
	if (up_level > 0) {
		eThing.setAttribute("named-level", QString::number(up_level));
	}
	// Одета вещь или нет
	if (dressed_) {
		eThing.setAttribute("dressed", "yes");
	}
	// Модификаторы
	if (loss_ != 0 || loss_mul != 0.0f || protect_ != 0 || protect_mul != 0.0f || force_ != 0 || force_mul != 0.0f || dext_ != 0 || dext_mul != 0.0f || intell_ != 0 || intell_mul != 0.0f || !param_str.isEmpty()) {
		QDomElement eModif = xmlDoc.createElement("modifiers");
		// Урон вещи
		if (loss_ != 0) {
			QDomElement domElement = xmlDoc.createElement("loss");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(loss_)));
			eModif.appendChild(domElement);
		}
		if (loss_mul != 0.0f) {
			QDomElement domElement = xmlDoc.createElement("loss-mul");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(loss_mul)));
			eModif.appendChild(domElement);
		}
		// Защита вещи
		if (protect_ != 0) {
			QDomElement domElement = xmlDoc.createElement("protect");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(protect_)));
			eModif.appendChild(domElement);
		}
		if (protect_mul != 0.0f) {
			QDomElement domElement = xmlDoc.createElement("protect-mul");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(protect_mul)));
			eModif.appendChild(domElement);
		}
		// Сила вещи
		if (force_ != 0) {
			QDomElement domElement = xmlDoc.createElement("force");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(force_)));
			eModif.appendChild(domElement);
		}
		if (force_mul != 0.0f) {
			QDomElement domElement = xmlDoc.createElement("force-mul");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(force_mul)));
			eModif.appendChild(domElement);
		}
		// Ловкость вещи
		if (dext_ != 0) {
			QDomElement domElement = xmlDoc.createElement("dext");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(dext_)));
			eModif.appendChild(domElement);
		}
		if (dext_mul != 0.0f) {
			QDomElement domElement = xmlDoc.createElement("dext-mul");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(dext_mul)));
			eModif.appendChild(domElement);
		}
		// Интеллект вещи
		if (intell_ != 0) {
			QDomElement domElement = xmlDoc.createElement("intell");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(intell_)));
			eModif.appendChild(domElement);
		}
		if (intell_mul != 0.0f) {
			QDomElement domElement = xmlDoc.createElement("intell-mul");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(intell_mul)));
			eModif.appendChild(domElement);
		}
		// Дополнительные параметры вещи
		if (!param_str.isEmpty()) {
			 QDomElement domElement = xmlDoc.createElement("other-modif");
			 domElement.appendChild(xmlDoc.createTextNode(param_str));
			 eModif.appendChild(domElement);
		}
		eThing.appendChild(eModif);
	}
	// Требования
	if (req_level != 0 || req_force != 0 || req_dext != 0 || req_intell != 0) {
		QDomElement eRequ = xmlDoc.createElement("requirements");
		// Уровень
		if (req_level != 0) {
			QDomElement domElement = xmlDoc.createElement("level");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(req_level)));
			eRequ.appendChild(domElement);
		}
		if (req_force != 0) {
			QDomElement domElement = xmlDoc.createElement("force");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(req_force)));
			eRequ.appendChild(domElement);
		}
		if (req_dext != 0) {
			QDomElement domElement = xmlDoc.createElement("dext");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(req_dext)));
			eRequ.appendChild(domElement);
		}
		if (req_intell != 0) {
			QDomElement domElement = xmlDoc.createElement("intell");
			domElement.appendChild(xmlDoc.createTextNode(QString::number(req_intell)));
			eRequ.appendChild(domElement);
		}
		eThing.appendChild(eRequ);
	}
	return eThing;
}
