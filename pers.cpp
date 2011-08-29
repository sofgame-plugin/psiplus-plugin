/*
 * pers.cpp - Sof Game Psi plugin
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

#include "pers.h"
#include "pers_info.h"
#include "common.h"
#include "plugin_core.h"
#include "fight.h"
#include "utils.h"

Pers::Pers(QObject *parent):
	QObject(parent),
	pers_name(""),
	beginSetPersParamsFlag(false),
	persLevelValue(-1), persLevelValue_(-1), setPersLevelValueFlag(false),
	persExperienceMax(-1), persExperienceMax_(-1), setPersExperienceMaxFlag(false),
	persExperienceCurr(-1), persExperienceCurr_(-1), setPersExperienceCurrFlag(false),
	persStatus(StatusNotKnow), persStatus_(StatusNotKnow), setPersStatusFlag(false),
	persHealthMax(QINT32_MIN), persHealthMax_(QINT32_MIN), setPersHealthMaxFlag(false),
	persHealthCurr(QINT32_MIN), persHealthCurr_(QINT32_MIN), setPersHealthCurrFlag(false),
	persEnergyMax(QINT32_MIN), persEnergyMax_(QINT32_MIN), setPersEnergyMaxFlag(false),
	persEnergyCurr(QINT32_MIN), persEnergyCurr_(QINT32_MIN), setPersEnergyCurrFlag(false),
	setPersLevelFlag(false),
	loadingFings(false),
	fingChanged(false),
	fingsPos(0), fingsSize(0),
	settingWatchRestHealthEnergy(-1),
	watchHealthStartValue(QINT32_MIN), watchHealthStartValue2(QINT32_MIN),
	watchHealthSpeed(0.0f), watchHealthSpeedDelta(0),
	watchRestTimer(NULL), watchHealthRestTimer(NULL)
{
	/**
	* Конструктор
	**/
	// Инициализация
	things = new ThingsModel(NULL);
	thingModels.clear();
}

Pers::~Pers()
{
	/**
	* Деструктор
	*/
	if (watchRestTimer) {
		if (watchRestTimer->isActive()) watchRestTimer->stop();
		disconnect(watchRestTimer, SIGNAL(timeout()), this, SLOT(doWatchRestTime()));
		delete watchRestTimer;
		watchRestTimer = 0;
	}
	if (watchHealthRestTimer) {
		if (watchHealthRestTimer->isActive()) watchHealthRestTimer->stop();
		disconnect(watchHealthRestTimer, SIGNAL(timeout()), this, SLOT(doWatchHealthRestTime()));
		delete watchHealthRestTimer;
		watchHealthRestTimer = 0;
	}
	// Удаляем оставшиеся прокси модели
	QList<int> keys_list = thingModels.keys();
	while (!keys_list.isEmpty()) {
		ThingsProxyModel* fm = thingModels.take(keys_list.takeLast());
		if (fm)
			delete fm;
	}
	// --
	while (!fingFiltersEx.isEmpty())
		delete fingFiltersEx.takeFirst();
	if (things) {
		delete things;
	}
}

Pers *Pers::instance_ = NULL;

Pers *Pers::instance()
{
	if (Pers::instance_ == NULL) {
		Pers::instance_ = new Pers(NULL);
	}
	return Pers::instance_;
}

void Pers::reset()
{
	if (Pers::instance_ != NULL) {
		delete Pers::instance_;
		Pers::instance_ = NULL;
	}
}

void Pers::init()
{
	// Удаляем оставшиеся прокси модели
	QList<int> keys_list = thingModels.keys();
	while (!keys_list.isEmpty()) {
		ThingsProxyModel* fm = thingModels.take(keys_list.takeLast());
		if (fm)
			delete fm;
	}
	// --
	while (!fingFiltersEx.isEmpty())
		delete fingFiltersEx.takeFirst();
	persStatus = StatusNotKnow;
	persHealthCurr = QINT32_MIN;
	persHealthMax = QINT32_MIN;
	persEnergyCurr = QINT32_MIN;
	persEnergyMax = QINT32_MIN;
	things->clear();
	fingPrice.clear();
	if (watchRestTimer) {
		watchRestTimer->stop();
		delete watchRestTimer;
		watchRestTimer = 0;
	}
	if (watchHealthRestTimer) {
		watchHealthRestTimer->stop();
		delete watchHealthRestTimer;
		watchHealthRestTimer = 0;
	}
	settingWatchRestHealthEnergy = -1;
	watchHealthRestTimer = 0;
	watchHealthSpeedDelta = 0;
	watchHealthSpeed = 0.0;
	watchHealthStartValue = QINT32_MIN;
	persLevelValue = -1;
	pers_name = "";
	Settings *settings = Settings::instance();
	loadBackpackSettingsFromDomNode(settings->getBackpackData());
	setSetting(Settings::SettingWatchRestHealthEnergy, Settings::instance()->getIntSetting(Settings::SettingWatchRestHealthEnergy));
}

void Pers::setName(const QString &new_name)
{
	if (pers_name != new_name) {
		pers_name = new_name;
		if (persHealthCurr != QINT32_MIN) { // TODO Что за проверка?
			emit persParamChanged(ParamPersName, TYPE_STRING, 0);
		} else {
			emit persParamChanged(ParamPersName, TYPE_NA, 0);
		}
	}
}

QString Pers::name() const
{
	return pers_name;
}

void Pers::setFingsStart(bool /*clear*/)
{
	/**
	* Старт загрузки списка вещей
	**/
	loadingFings = true;
	fingChanged = false;
	fingsPos = 0;
	fingsSize = things->rowCount();
}

void Pers::setFingsEnd()
{
	loadingFings = false;
	if (fingsPos < fingsSize) {
		// Остались еще старые записи
		if (fingsPos == 0) {
			things->clear();
		} else {
			if (fingsPos < fingsSize) {
				things->removeRows(fingsPos, fingsSize - fingsPos);
			}
		}
		fingChanged = true;
		fingsSize = fingsPos;
	}
	if (fingChanged) {
		emit fingsChanged();
	}
}

/**
 * Обновляет сведения о вещи в массиве вещей.
 * Если вещь изменилась, то выставляется соотв. флаг
 */
void Pers::setFingElement(int mode, Thing* thing)
{
	if (loadingFings) {
		if (mode == FING_APPEND) {
			if (fingsPos < fingsSize) {
				if (!fingChanged) {
					const Thing* old_thing = things->getThingByRow(fingsPos);
					if (old_thing) {
						if (!thing->isEqual(old_thing)) {
							fingChanged = true;
						}
					} else {
						fingChanged = true;
					}
				}
				if (fingChanged) {
					things->setThing(thing, fingsPos);
				}
			} else {
				things->insertThing(thing, things->rowCount());
				things->setThing(thing, fingsPos);
				fingChanged = true;
				fingsSize++;
			}
			//if (fingChanged) {
				// Ищем цену в прайсе
				int priceCnt = fingPrice.size();
				int nType = thing->type();
				QString sName = thing->name().toLower();
				for (int j = 0; j < priceCnt; j++) {
					if (fingPrice[j].type == nType) {
						if (fingPrice[j].name.toLower() == sName) {
							thing->setPrice(fingPrice[j].price);
							break;
						}
					}
				}
			//}
			fingsPos++;
		}
	}
}

/**
 * Возвращает общее количество вещей для указанного интерфейса
 */
int Pers::getFingsCount(int iface) const
{
	int all_cnt = 0;
	ThingsProxyModel *tpm = thingModels.value(iface, NULL);
	if (tpm) {
		int cnt = tpm->rowCount();
		for (int i = 0; i < cnt; i++) {
			const Thing* thg = tpm->getThingByRow(i);
			if (thg) {
				if (thg->isValid()) {
					all_cnt += thg->count();
				}
			}
		}
	}
	return all_cnt;
}

/**
 * Возвращает полную стоимость вещей для указанного интерфейса
 */
int Pers::getPriceAll(int iface) const
{
	int all_price = 0;
	ThingsProxyModel *tpm = thingModels.value(iface, NULL);
	if (tpm) {
		int cnt = tpm->rowCount();
		for (int i = 0; i < cnt; i++) {
			const Thing* thg = tpm->getThingByRow(i);
			if (thg) {
				if (thg->isValid()) {
					int price = thg->price();
					if (price != -1) {
						all_price += thg->count() * price;
					}
				}
			}
		}
	}
	return all_price;
}

/**
 * Возвращает количество позиций вещей без цены для указанного интерфейса
 */
int Pers::getNoPriceCount(int iface) const
{
	int no_price = 0;
	ThingsProxyModel *tpm = thingModels.value(iface, NULL);
	if (tpm) {
		int cnt = tpm->rowCount();
		for (int i = 0; i < cnt; i++) {
			const Thing* thg = tpm->getThingByRow(i);
			if (thg) {
				if (thg->isValid()) {
					if (thg->price() == -1)
						++no_price;
				}
			}
		}
	}
	return no_price;
}

/**
 * Возвращает указатель на объект вещи, учитывая номер позиции и заданный интерфейс
 */
const Thing* Pers::getFingByRow(int row, int iface) const
{
	const Thing* thg = NULL;
	ThingsProxyModel* tpm = thingModels.value(iface, NULL);
	if (tpm)
		thg = tpm->getThingByRow(row);
	return thg;
}

/**
 * Возращает список указателей на фильтры
 */
void Pers::getFingsFiltersEx(QList<FingFilter*>* filtersPtr) const
{
	filtersPtr->clear();
	int cnt = fingFiltersEx.size();
	for (int i = 0; i < cnt; i++) {
		filtersPtr->push_back(fingFiltersEx.at(i));
	}
}

/**
 * Переписывает существующие фильтры фильтрами из массива
 */
void Pers::setFingsFiltersEx(QList<FingFilter*> newFilters)
{
	// Удаляем старый список фильтров
	while (!fingFiltersEx.isEmpty())
		delete fingFiltersEx.takeFirst();
	// Копируем новые фильтры
	int cnt = newFilters.size();
	for (int i = 0; i < cnt; i++)
		fingFiltersEx.push_back(new FingFilter(*newFilters.at(i)));
	emit filtersChanged();
}

/**
 * Возвращает указатель на список цен
 */
const QVector<Pers::price_item>* Pers::getFingsPrice() const
{
	return &fingPrice;
}

/**
 * Выгружает данные о вещах в DOM элемент
 */
QDomElement Pers::exportThingsToDomElement(QDomDocument &xmlDoc) const
{
	QDomElement eThings = xmlDoc.createElement("fings");
	int thingsCnt = things->rowCount();
	for (int i = 0; i < thingsCnt; i++) {
		const Thing* thing = things->getThingByRow(i);
		if (!thing) continue;
		if (!thing->isValid()) continue;
		QDomElement eThing = thing->exportToXml(xmlDoc);
		eThings.appendChild(eThing);
	}
	return eThings;
}

/**
 * Выгружает данные о ценах в DOM элемент
 */
QDomElement Pers::exportPriceToDomElement(QDomDocument &xmlDoc) const
{
	QDomElement ePrice;
	int priceCnt = fingPrice.size();
	if (priceCnt > 0) {
		ePrice = xmlDoc.createElement("price");
		for (int i = 0; i < priceCnt; i++) {
			QDomElement ePriceItem = xmlDoc.createElement("item");
			int nType = fingPrice[i].type;
			QString sType = thingTypeToString(nType);
			if (!sType.isEmpty()) {
				ePriceItem.setAttribute("type", sType);
				ePriceItem.setAttribute("name", fingPrice[i].name);
				ePriceItem.setAttribute("price", QString::number(fingPrice[i].price));
				ePrice.appendChild(ePriceItem);
			}
		}
	}
	return ePrice;
}

/**
 * Выгружает настройки рюкзака в DOM элемент
 */
QDomElement Pers::exportBackpackSettingsToDomElement(QDomDocument &xmlDoc) const
{
	QDomElement eBackpack = xmlDoc.createElement("backpack");
	QDomElement eFilters = xmlDoc.createElement("filters");
	eBackpack.appendChild(eFilters);
	int filtersCnt = fingFiltersEx.size();
	for (int i = 0; i < filtersCnt; i++) {
		FingFilter* oFilter = fingFiltersEx.at(i);
		QDomElement eFilter = xmlDoc.createElement("filter");
		if (!oFilter->isActive())
			eFilter.setAttribute("disabled", "true");
		eFilter.setAttribute("name", oFilter->name());
		eFilters.appendChild(eFilter);
		int rulesCnt = oFilter->rulesCount();
		if (rulesCnt > 0) {
			QDomElement eRules = xmlDoc.createElement("rules");
			eFilter.appendChild(eRules);
			int ruleIndex = 0;
			while (true) {
				const struct FingFilter::fing_rule_ex* fre = oFilter->getRule(ruleIndex++);
				if (!fre)
					break;
				QDomElement eRule = xmlDoc.createElement("rule");
				QString str1 = "";
				FingFilter::ParamRole nParam = fre->param;
				if (nParam == FingFilter::NameRole) {
					str1 = "name";
				} else if (nParam == FingFilter::TypeRole) {
					str1 = "type";
				} else if (nParam == FingFilter::NamedRole) {
					str1 = "named-level";
				} else if (nParam == FingFilter::DressedRole) {
					str1 = "dressed";
				} else if (nParam == FingFilter::PriceRole) {
					str1 = "price";
				} else if (nParam == FingFilter::CountRole) {
					str1 = "count";
				}
				if (!str1.isEmpty())
					eRule.setAttribute("field", str1);
				if (fre->negative)
					eRule.setAttribute("not", "not");
				str1 = "";
				FingFilter::OperandRole nOper = fre->operand;
				if (nOper == FingFilter::EqualRole) {
					str1 = "equal";
				} else if (nOper == FingFilter::ContainsRole) {
					str1 = "contains";
				} else if (nOper == FingFilter::AboveRole) {
					str1 = "above";
				} else if (nOper == FingFilter::LowRole) {
					str1 = "low";
				}
				if (!str1.isEmpty())
					eRule.setAttribute("operand", str1);
				if (nParam == FingFilter::TypeRole) {
					int nValue = fre->int_value;
					str1 = thingTypeToString(nValue);
					if (str1.isEmpty())
						continue;
				} else {
					str1 = fre->value;
				}
				if (!str1.isEmpty())
					eRule.setAttribute("value", str1);
				str1 = "";
				FingFilter::ActionRole nAction = fre->action;
				if (nAction == FingFilter::YesRole) {
					str1 = "yes";
				} else if (nAction == FingFilter::NoRole) {
					str1 = "no";
				} else if (nAction == FingFilter::NextRole) {
					str1 = "next";
				}
				if (!str1.isEmpty())
					eRule.setAttribute("action", str1);
				eRules.appendChild(eRule);
			}
		}
	}
	return eBackpack;
}

/**
 * Получает данные о содержимом рюкзака персонажа из DOM элемента
 */
void Pers::loadThingsFromDomElement(QDomElement &eBackpack)
{
	things->clear();
	fingPrice.clear();
	QDomElement eChild1 = eBackpack.firstChildElement();
	while (!eChild1.isNull()) {
		const QString sTagName = eChild1.tagName();
		if (sTagName == "fings") {
			QDomElement eThing = eChild1.firstChildElement("fing");
			while (!eThing.isNull()) {
				Thing* thing = new Thing();
				thing->importFromXml(eThing);
				if (thing->isValid()) {
					things->insertThing(thing, things->rowCount());
				} else {
					delete thing;
				}
				eThing = eThing.nextSiblingElement("fing");
			}
		} else if (sTagName == "price") {
			// Просматриваем прайс и создаем массив с ценами
			QDomElement ePriceItem = eChild1.firstChildElement("item");
			while (!ePriceItem.isNull()) {
				struct price_item priceItem;
				priceItem.type = thingTypeFromString(ePriceItem.attribute("type"));
				if (priceItem.type != -1) {
					priceItem.name = ePriceItem.attribute("name").trimmed();
					if (!priceItem.name.isEmpty()) {
						bool fOk = false;
						priceItem.price = ePriceItem.attribute("price").toInt(&fOk);
						if (fOk && priceItem.price >= 0) {
							fingPrice.push_back(priceItem);
						}
					}
				}
				ePriceItem = ePriceItem.nextSiblingElement("item");
			}
		}
		eChild1 = eChild1.nextSiblingElement();
	}
	// Заполняем цены вещей из прайса
	int thingsCnt = things->rowCount();
	int priceCnt = fingPrice.size();
	for (int i = 0; i < thingsCnt; i++) {
		for (int j = 0; j < priceCnt; j++) {
			Thing* thing = things->getThingByRow(i);
			if (thing) {
				if (thing->type() == fingPrice[j].type) {
					if (thing->name().toLower() ==  fingPrice[j].name.toLower()) {
						thing->setPrice(fingPrice[j].price);
						break;
					}
				}
			}
		}
	}
}

/**
 * Получает настройки рюкзака персонажа из DOM ноды
 */
void Pers::loadBackpackSettingsFromDomNode(const QDomElement &eBackpack)
{
	// Очищаем старые настройки
	while (!fingFiltersEx.isEmpty()) {
		delete fingFiltersEx.takeFirst();
	}
	// Анализируем DOM ноду
	QDomElement eFilters = eBackpack.firstChildElement("filters");
	if (eFilters.isNull())
		return;
	QDomElement eFilter = eFilters.firstChildElement("filter");
	while (!eFilter.isNull()) {
		const QString sName = eFilter.attribute("name");
		if (!sName.isEmpty()) {
			FingFilter* ffe = new FingFilter();
			ffe->setName(sName);
			if (eFilter.attribute("disabled").toLower() == "true")
				ffe->setActive(false);
			// Теперь ищем правила
			QDomElement eRules = eFilter.firstChildElement("rules");
			if (!eRules.isNull()) {
				QDomElement eRule = eRules.firstChildElement("rule");
				while (!eRule.isNull()) {
					// Грузим элемент правила
					QString str1 = eRule.attribute("field").trimmed().toLower();
					FingFilter::ParamRole nField = FingFilter::NoParamRole;
					if (str1 == "name") {
						nField = FingFilter::NameRole;
					} else if (str1 == "type") {
						nField = FingFilter::TypeRole;
					} else if (str1 == "named-level") {
						nField = FingFilter::NamedRole;
					} else if (str1 == "dressed") {
						nField = FingFilter::DressedRole;
					} else if (str1 == "price") {
						nField = FingFilter::PriceRole;
					} else if (str1 == "count") {
						nField = FingFilter::CountRole;
					}
					str1 = eRule.attribute("not");
					bool fNot = !str1.isEmpty();
					FingFilter::OperandRole nOperand = FingFilter::NoOperRole;
					str1 = eRule.attribute("operand").trimmed().toLower();
					if (str1 == "contains") {
						nOperand = FingFilter::ContainsRole;
					} else if (str1 == "equal") {
						nOperand = FingFilter::EqualRole;
					} else if (str1 == "above") {
						nOperand = FingFilter::AboveRole;
					} else if (str1 == "low") {
						nOperand = FingFilter::LowRole;
					}
					str1 = eRule.attribute("action").trimmed().toLower();
					FingFilter::ActionRole nAction = FingFilter::NoActionRole;
					if (str1 == "yes") {
						nAction = FingFilter::YesRole;
					} else if (str1 == "no") {
						nAction = FingFilter::NoRole;
					} else if (str1 == "next") {
						nAction = FingFilter::NextRole;
					}
					str1 = eRule.attribute("value");
					ffe->appendRule(nField, fNot, nOperand, str1, nAction);
					eRule = eRule.nextSiblingElement("rule");
				}
			}
			fingFiltersEx.push_back(ffe);
		}
		eFilter = eFilter.nextSiblingElement("filter");
	}
	emit filtersChanged();
}

/**
 * Устанавливает цену для вещи
 */
void Pers::setFingPrice(int iface, int row, int price)
{
	ThingsProxyModel* tpm = thingModels.value(iface, NULL);
	if (tpm == NULL)
		return;
	tpm->setPrice(row, price);
	// Правим прайс
	const Thing *thg = tpm->getThingByRow(row);
	if (!thg || !thg->isValid())
		return;
	int n_type = thg->type();
	QString s_name = thg->name();
	QString s_name2 = s_name.toLower();
	int price_cnt = fingPrice.size();
	bool f_find = false;
	for (int i = 0; i < price_cnt; i++) {
		if (fingPrice[i].type == n_type) {
			if (fingPrice[i].name.toLower() == s_name2) {
				fingPrice[i].price = price;
				f_find = true;
				break;
			}
		}
	}
	if (!f_find) {
		struct price_item p_i;
		p_i.type = n_type;
		p_i.name = s_name;
		p_i.price = price;
		fingPrice.push_back(p_i);
	}
}

void Pers::beginSetPersParams()
{
	beginSetPersParamsFlag = true;
	setPersStatusFlag = false;
	setPersHealthMaxFlag = false;
	setPersHealthCurrFlag = false;
	setPersEnergyMaxFlag = false;
	setPersEnergyCurrFlag = false;
	setPersLevelFlag = false;
	setPersExperienceMaxFlag = false;
	setPersExperienceCurrFlag = false;
	setPersLevelValueFlag = false;
}

/**
 * Устанавливает основные параметры персонажа
 */
void Pers::setPersParams(int valueId, int valueType, int value) // TODO Переписать на массив!
{
	if (!beginSetPersParamsFlag) return;
	if (valueType == TYPE_INTEGER_FULL) {
		if (valueId == ParamPersStatus) {
			if (persStatus != value) {
				// Изменился статус персонажа
				setPersStatusFlag = true;
				persStatus_ = (PersStatus)value;
			}
		} else if (valueId == ParamHealthMax) {
			if (persHealthMax != value) {
				// Изменилось максимальное значение здоровья
				setPersHealthMaxFlag = true;
				persHealthMax_ = value;
			}
		} else if (valueId == ParamHealthCurr) {
			if (persHealthCurr != value) {
				// Изменилось текущее значение здоровья
				setPersHealthCurrFlag = true;
				persHealthCurr_ = value;
			}
		} else if (valueId == ParamEnergyMax) {
			if (persEnergyMax != value) {
				// Изменилось максимальное значение энергии
				setPersEnergyMaxFlag = true;
				persEnergyMax_ = value;
			}
		} else if (valueId == ParamEnergyCurr) {
			if (persEnergyCurr != value) {
				// Изменилось текущее значение энергии
				setPersEnergyCurrFlag = true;
				persEnergyCurr_ = value;
			}
		} else if (valueId == ParamPersLevel) {
			if (persLevelValue != value) {
				// Изменилось значение уровня персонажа
				setPersLevelValueFlag = true;
				persLevelValue_ = value;
			}
		}
	}
}

/**
 * Устанавливает основные параметры персонажа
 */
void Pers::setPersParams(int valueId, int valueType, long long value) // TODO Переписать на массив!
{
	if (!beginSetPersParamsFlag) return;
	if (valueType == TYPE_LONGLONG_FULL) {
		if (valueId == ParamExperienceMax) {
			if (persExperienceMax != value) {
				// Изменился максимальный опыт персонажа
				setPersExperienceMaxFlag = true;
				persExperienceMax_ = value;
			}
		} else if (valueId == ParamExperienceCurr) {
			if (persExperienceCurr != value) {
				// Изменился текущий опыт персонажа
				setPersExperienceCurrFlag = true;
				persExperienceCurr_ = value;
			}
		}
	}
}

/**
 * Фиксация изменений параметров персонажа
 */
void Pers::endSetPersParams()
{
	if (!beginSetPersParamsFlag) return;
	// Меняем
	if (setPersStatusFlag) persStatus = persStatus_;
	if (setPersHealthMaxFlag) {
		persHealthMax = persHealthMax_;
		if (settingWatchRestHealthEnergy == 1) {
			// Сбрасываем замеры скорости восстановления и начальный отсчет
			watchHealthSpeedDelta = 0;
			watchHealthSpeed = 0.0;
			watchHealthStartValue = QINT32_MIN;
		}
	}
	if (setPersHealthCurrFlag) {
		if (settingWatchRestHealthEnergy == 1 && watchHealthSpeedDelta != 0 && watchHealthStartValue != QINT32_MIN) {
			if (persHealthCurr != persHealthCurr_) {
				// Возможно необходим сброс скорости восстановления
				int i = persHealthCurr - persHealthCurr_;
				if (i < -1 || i > 1) {
					float j = (float)persHealthCurr_ / (float)persHealthCurr;
					if (j < 0.95 || j > 1.05) {
						watchHealthSpeedDelta = 0;
						watchHealthSpeed = 0.0;
						watchHealthStartValue = QINT32_MIN;
					}
				}
			}
		}
		persHealthCurr = persHealthCurr_;
	}
	if (setPersEnergyMaxFlag) persEnergyMax = persEnergyMax_;
	if (setPersEnergyCurrFlag) persEnergyCurr = persEnergyCurr_;
	if (setPersLevelValueFlag) persLevelValue = persLevelValue_;
	if (setPersExperienceMaxFlag) persExperienceMax = persExperienceMax_;
	if (setPersExperienceCurrFlag) persExperienceCurr = persExperienceCurr_;
	// Фиксируем изменения на сигналах
	// и реагируем на изменения согласно настроек
	if (setPersStatusFlag) {
		emit persParamChanged(ParamPersStatus, TYPE_INTEGER_FULL, persStatus);
	}
	if (setPersHealthMaxFlag) {
		if (persHealthMax != QINT32_MIN) {
			emit persParamChanged(ParamHealthMax, TYPE_INTEGER_FULL, persHealthMax);
		} else {
			emit persParamChanged(ParamHealthMax, TYPE_NA, 0);
		}
	}
	if (setPersHealthCurrFlag) {
		if (persHealthCurr != QINT32_MIN) {
			emit persParamChanged(ParamHealthCurr, TYPE_INTEGER_FULL, persHealthCurr);
		} else {
			emit persParamChanged(ParamHealthCurr, TYPE_NA, 0);
		}
	}
	if (setPersEnergyMaxFlag) {
		if (persEnergyMax != QINT32_MIN) {
			emit persParamChanged(ParamEnergyMax, TYPE_INTEGER_FULL, persEnergyMax);
		} else {
			emit persParamChanged(ParamEnergyMax, TYPE_NA, 0);
		}
		////////////////////////if (settingWatchRestHealthEnergy == 1) watchStatus = 0; // Сбрасываем замеры
	}
	if (setPersEnergyCurrFlag) {
		if (persEnergyCurr != QINT32_MIN) {
			emit persParamChanged(ParamEnergyCurr, TYPE_INTEGER_FULL, persEnergyCurr);
		} else {
			emit persParamChanged(ParamEnergyCurr, TYPE_NA, 0);
		}
	}
	if (setPersLevelValueFlag) {
		if (persLevelValue != -1) {
			emit persParamChanged(ParamPersLevel, TYPE_INTEGER_FULL, persLevelValue);
		} else {
			emit persParamChanged(ParamPersLevel, TYPE_NA, 0);
		}
	}
	if (setPersExperienceMaxFlag) {
		if (persExperienceMax != -1) {
			emit persParamChanged(ParamExperienceMax, TYPE_LONGLONG_FULL, 0);
		} else {
			emit persParamChanged(ParamExperienceMax, TYPE_NA, 0);
		}
	}
	if (setPersExperienceCurrFlag) {
		if (persExperienceCurr != -1) {
			emit persParamChanged(ParamExperienceCurr, TYPE_LONGLONG_FULL, 0);
		} else {
			emit persParamChanged(ParamExperienceCurr, TYPE_NA, 0);
		}
	}
	//--
	if (settingWatchRestHealthEnergy >= 2) {
		if (setPersEnergyCurrFlag || setPersHealthCurrFlag || setPersHealthMaxFlag || setPersEnergyMaxFlag) {
			// Простое периодическое отслеживание здоровья и энергии
			if (persStatus == StatusStand) {
				// Запускаем таймер, если неполное здоровье или энергия
				if (persHealthCurr < persHealthMax || persEnergyCurr < persEnergyMax) {
					if (watchRestTimer->isActive()) watchRestTimer->stop();
					int interval = 0;
					if (settingWatchRestHealthEnergy == 2) {
						interval = 10000; // 10 sec.
					} else if (settingWatchRestHealthEnergy == 3) {
						interval = 30000; // 30 sec.
					} else if (settingWatchRestHealthEnergy == 4) {
						interval = 60000; // 60 sec.
					}
					watchRestTimer->setSingleShot(false);
					watchRestTimer->start(interval);
				} else {
					if (watchRestTimer->isActive()) watchRestTimer->stop();
				}
			} else {
				// Гасим таймер, т.к. запросы на обновление посылаем только когда стоим
				if (watchRestTimer->isActive()) watchRestTimer->stop();
			}
		}
	} else if (settingWatchRestHealthEnergy == 1) {
		if (setPersHealthCurrFlag || setPersHealthMaxFlag) {
			if (fight->isActive() && fight->isPersInFight()) { // Персонаж в бою.
				// Останавливаем таймер
				if (watchHealthRestTimer->isActive()) watchHealthRestTimer->stop();
				// Сбрасывыем начальные параметры расчета
				watchHealthStartValue = QINT32_MIN;
			} else { // Персонаж не в бою
				if (persHealthCurr != QINT32_MIN && persHealthMax != QINT32_MIN) {
					if (setPersHealthCurrFlag) {
						watchHealthStartValue2 = persHealthCurr; // Новое стартовое значение для отображения (не для расчета!!!)
						watchHealthStartTime2.start();
					}
					if (watchHealthSpeedDelta == 0 && watchHealthStartValue == QINT32_MIN) {
						// У нас нет стартового значения здоровья и нет и замеров скорости,
						// надо запускать принудительный замер
						if (persStatus == StatusStand) {
							if (watchHealthRestTimer->isActive()) watchHealthRestTimer->stop();
							if (persHealthCurr < persHealthMax) {
								watchHealthStartValue = persHealthCurr;
								watchHealthStartTime.start();
								//watchHealthSpeedDelta = 0; // И так 0 по условию
								watchHealthRestTimer->setSingleShot(true);
								watchHealthRestTimer->start(15000); // время для анализа скорости восстановления
							}
						}
					} else {
						if (watchHealthStartValue != QINT32_MIN) { // Есть стартовое значение для замера
							if (persHealthCurr < persHealthMax) {
								int watchTimeDelta = watchHealthStartTime.elapsed();
								if (watchTimeDelta >= 10000) {
									if (watchHealthSpeedDelta < watchTimeDelta) { // Старые замеры менее точные
										if (watchHealthRestTimer->isActive()) watchHealthRestTimer->stop();
										watchHealthSpeed = 0.0;
										if (watchHealthStartValue < persHealthCurr) {
											// Скорость обновления здоровья
											watchHealthSpeed = ((float)persHealthCurr - (float)watchHealthStartValue) / watchTimeDelta;
											watchHealthSpeedDelta = watchTimeDelta;
										}
									}
								}
							} else { // Замер не закончился, а перс уже отрегенился
								if (watchHealthRestTimer->isActive()) watchHealthRestTimer->stop();
							}
						} else { // Скорость есть, но нет стартового замера, возможно из за сброса его во время боя
							watchHealthStartValue = persHealthCurr;
							watchHealthStartTime.start();
						}
						if (watchHealthSpeedDelta > 0) { // У нас есть замеры
							if (watchHealthSpeed != 0.0 && persHealthCurr < persHealthMax) {
								int watchTimeDelta = watchHealthStartTime.elapsed();
								if (watchTimeDelta <= 0) {
									// Новые стартовые значения
									watchHealthStartValue = persHealthCurr;
									watchHealthStartTime.start();
								}
								// Запускаем таймер
								if (!watchHealthRestTimer->isActive()) {
									watchHealthRestTimer->setSingleShot(false);
									watchHealthRestTimer->start(500); // Обновляем значения два раза в секунду TODO Сделать более оптимально
								}
							}
						}
					}
				} else { // Данные по персу неизвестны
					// Останавливаем таймер
					if (watchHealthRestTimer->isActive()) watchHealthRestTimer->stop();
					// Сбрасывыем начальные параметры расчета
					watchHealthStartValue = QINT32_MIN;
					watchHealthSpeed = 0.0;
				}
			}
		}
	}
}

bool Pers::getIntParamValue(int paramId, int *paramValue) const
{
	if (paramId == ParamPersStatus) {
		*paramValue = persStatus;
	} else if (paramId == ParamHealthCurr) {
		if (persHealthCurr == QINT32_MIN)
			return false;
		*paramValue = persHealthCurr;
	} else if (paramId == ParamHealthMax) {
		if (persHealthMax == QINT32_MIN)
			return false;
		*paramValue = persHealthMax;
	} else if (paramId == ParamEnergyCurr) {
		if (persEnergyCurr == QINT32_MIN)
			return false;
		*paramValue = persEnergyCurr;
	} else if (paramId == ParamEnergyMax) {
		if (persEnergyMax == QINT32_MIN)
			return false;
		*paramValue = persEnergyMax;
	} else if (paramId == ParamPersLevel) {
		if (persLevelValue == -1)
			return false;
		*paramValue = persLevelValue;
	} else {
		return false;
	}
	return true;
}

bool Pers::getLongParamValue(int paramId, long long *paramValue) const
{
	if (paramId == ParamExperienceMax) {
		if (persExperienceMax == -1)
			return false;
		*paramValue = persExperienceMax;
	} else if (paramId == ParamExperienceCurr) {
		if (persExperienceCurr == -1)
			return false;
		*paramValue = persExperienceCurr;
	} else {
		return false;
	}
	return true;
}

bool Pers::getStringParamValue(PersParams paramId, QString *paramValue) const
{
	if (paramId == ParamPersName) {
		if (pers_name.isEmpty()) return false;
		*paramValue = pers_name;
	} else {
		return false;
	}
	return true;
}

void Pers::setSetting(Settings::SettingKey setId, int setValue)
{
	switch (setId) {
	case Settings::SettingWatchRestHealthEnergy:
		if (setValue == settingWatchRestHealthEnergy) return;
		if (setValue == 0) {
			settingWatchRestHealthEnergy = 0;
			if (watchRestTimer) {
				if (watchRestTimer->isActive()) watchRestTimer->stop();
				disconnect(watchRestTimer, SIGNAL(timeout()), this, SLOT(doWatchRestTime()));
				delete watchRestTimer;
				watchRestTimer = 0;
			}
			if (watchHealthRestTimer) {
				if (watchHealthRestTimer->isActive()) watchHealthRestTimer->stop();
				disconnect(watchHealthRestTimer, SIGNAL(timeout()), this, SLOT(doWatchHealthRestTime()));
				delete watchHealthRestTimer;
				watchHealthRestTimer = 0;
			}
		} else if (setValue == 1) {
			if (watchRestTimer) {
				if (watchRestTimer->isActive()) watchRestTimer->stop();
				disconnect(watchRestTimer, SIGNAL(timeout()), this, SLOT(doWatchRestTime()));
				delete watchRestTimer;
				watchRestTimer = 0;
			}
			settingWatchRestHealthEnergy = 1;
			watchHealthStartValue = QINT32_MIN;
			watchHealthSpeed = 0.0;
			watchHealthSpeedDelta = 0;
			if (!watchHealthRestTimer) {
				watchHealthRestTimer = new QTimer();
				connect(watchHealthRestTimer, SIGNAL(timeout()), this, SLOT(doWatchHealthRestTime()));
			}
		} else if (setValue >= 2 && setValue <= 4) {
			if (watchHealthRestTimer) {
				if (watchHealthRestTimer->isActive()) watchHealthRestTimer->stop();
				disconnect(watchHealthRestTimer, SIGNAL(timeout()), this, SLOT(doWatchHealthRestTime()));
				delete watchHealthRestTimer;
				watchHealthRestTimer = 0;
			}
			settingWatchRestHealthEnergy = setValue;
			if (!watchRestTimer) {
				watchRestTimer = new QTimer();
				connect(watchRestTimer, SIGNAL(timeout()), this, SLOT(doWatchRestTime()));
			}
		}
		break;
	default:
		;
	}
}

/**
 * Cоздает новую ProxyModel и возвращает индекс из массива моделей
 */
int Pers::getThingsInterface()
{
	for (int i = 1; ; i++) {
		if (!thingModels.contains(i)) {
			ThingsProxyModel* tpm = new ThingsProxyModel();
			if (!tpm)
				break;
			tpm->setFingsSource(things);
			thingModels.insert(i, tpm);
			return i;
		}
	}
	return 0;
}

void Pers::setThingsInterfaceFilter(int iface, int filter_num)
{
	ThingsProxyModel* tpm = thingModels.value(iface, NULL);
	if (tpm) {
		FingFilter* ff = NULL;
		if (filter_num > 0 && filter_num <= fingFiltersEx.size())
			ff = fingFiltersEx.at(filter_num - 1);
		tpm->setFilter(ff);
	}
}

/**
 * Освобождает память занятую под ProxyModel и удаляет запись в массиве
 */
void Pers::removeThingsInterface(int index)
{
	ThingsProxyModel* tpm = thingModels.value(index, NULL);
	if (tpm) {
		delete tpm;
		thingModels.remove(index);
	}
}

/**
 * Возвращает указатель на модель по индексу интерфейса
 */
QSortFilterProxyModel* Pers::getThingsModel(int index) const
{
	ThingsProxyModel* tpm = thingModels.value(index, NULL);
	if (tpm) {
		return tpm;
	}
	return NULL;
}

QHash<Pers::PersStatus, QString> Pers::statusStrings;

QString Pers::getPersStatusString()
{
	if (Pers::statusStrings.size() == 0) {
		// Таблица пуста, заполняем
		Pers::statusStrings[StatusStand]            = QString::fromUtf8("Стоим...");
		Pers::statusStrings[StatusFightMultiSelect] = QString::fromUtf8("Тут идут бои...");
		Pers::statusStrings[StatusFightOpenBegin]   = QString::fromUtf8("В бою! (Открытый)");
		Pers::statusStrings[StatusFightCloseBegin]  = QString::fromUtf8("В бою! (Закрытый)");
		Pers::statusStrings[StatusFightFinish]      = QString::fromUtf8("Бой завершен!");
		Pers::statusStrings[StatusMiniforum]        = QString::fromUtf8("Минифорум");
		Pers::statusStrings[StatusSecretBefore]     = QString::fromUtf8("Перед тайником");
		Pers::statusStrings[StatusSecretGet]        = QString::fromUtf8("Грабим тайник :)");
		Pers::statusStrings[StatusThingsList]       = QString::fromUtf8("Список вещей");
		Pers::statusStrings[StatusThingIsTaken]     = QString::fromUtf8("Выбрана вещь");
		Pers::statusStrings[StatusPersInform]       = QString::fromUtf8("Информация о персонаже");
		Pers::statusStrings[StatusFightShow]        = QString::fromUtf8("Просмотр боя");
		Pers::statusStrings[StatusOtherPersPos]     = QString::fromUtf8("Осматриваемся");
		Pers::statusStrings[StatusTakeBefore]       = QString::fromUtf8("Будем брать завоеванное");
		Pers::statusStrings[StatusTake]             = QString::fromUtf8("Забираем завоеванное");
		Pers::statusStrings[StatusInKillersCup]     = QString::fromUtf8("Вас заказали");
		Pers::statusStrings[StatusKillerAttack]     = QString::fromUtf8("Нападение убийцы");
		Pers::statusStrings[StatusYard]             = QString::fromUtf8("Во дворе");
		Pers::statusStrings[StatusMasterRoom1]      = QString::fromUtf8("Станок");
		Pers::statusStrings[StatusMasterRoom2]      = QString::fromUtf8("Малая мастерская");
		Pers::statusStrings[StatusMasterRoom3]      = QString::fromUtf8("Мастерская");
		Pers::statusStrings[StatusDealerBuy]        = QString::fromUtf8("Покупка у торговца");
		Pers::statusStrings[StatusBuyOk]            = QString::fromUtf8("Куплено");
		Pers::statusStrings[StatusDealerSale]       = QString::fromUtf8("Продажа торговцу");
		Pers::statusStrings[StatusHelpMenu]         = QString::fromUtf8("Меню по 09");
		Pers::statusStrings[StatusTopList]          = QString::fromUtf8("Сильнейшие персонажи");
		Pers::statusStrings[StatusServerStatistic1] = QString::fromUtf8("Статистика игры");
		Pers::statusStrings[StatusServerStatistic2] = QString::fromUtf8("Статистика по странам");
		Pers::statusStrings[StatusRealEstate]       = QString::fromUtf8("Ваша недвижимость");
		Pers::statusStrings[StatusWarehouse]        = QString::fromUtf8("Склад");
		Pers::statusStrings[StatusWarehouseShelf]   = QString::fromUtf8("Полка с вещами на складе");
		Pers::statusStrings[StatusAtHome]           = QString::fromUtf8("Дома");
	}
	return Pers::statusStrings.value(persStatus, "???");
}

void Pers::doWatchRestTime()
{
	if (persStatus == StatusStand) {
		QString str1 = "0";
		PluginCore::instance()->sendString(str1); // TODO Сделать отсылку только если нет очереди сообщений
	}
}

void Pers::doWatchHealthRestTime()
{
	if (settingWatchRestHealthEnergy == 1 && watchHealthSpeedDelta == 0) {
		if (persStatus == StatusStand) {
			QString str1 = "0";
			PluginCore::instance()->sendString(str1); // TODO Сделать отсылку только если нет очереди сообщений
		}
	} else if (settingWatchRestHealthEnergy == 1) {
		if (watchHealthSpeedDelta > 0) { // Имеются результаты замеров
			int timeDelta = watchHealthStartTime2.elapsed();
			if (persHealthCurr == QINT32_MIN || persHealthMax == QINT32_MIN || persHealthCurr >= persHealthMax) {
				watchHealthRestTimer->stop();
			} else {
				persHealthCurr = (float)watchHealthStartValue2 + (float)timeDelta * watchHealthSpeed;
				if (persHealthCurr > persHealthMax) persHealthCurr = persHealthMax;
				if (persHealthCurr == persHealthMax) watchHealthRestTimer->stop();
				emit persParamChanged(ParamHealthCurr, TYPE_INTEGER_FULL, persHealthCurr);
			}
		}
	}
}
