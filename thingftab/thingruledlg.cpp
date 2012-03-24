/*
 * thingruledlg.cpp - Sof Game Psi plugin
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

#include "thingruledlg.h"
#include "pers_info.h"
#include "utils.h"


ThingRuleEditDialog::ThingRuleEditDialog(QWidget* parent, struct ThingFilter::thing_rule_ex* rulePtr) :
	QDialog(parent)
{
	setupUi(this);
	// Сохраняем указатель
	savedRulePtr = rulePtr;
	// Заполняем массив ролей
	paramRoles << ThingFilter::NameRole << ThingFilter::TypeRole << ThingFilter::NamedRole << ThingFilter::DressedRole << ThingFilter::PriceRole << ThingFilter::CountRole;
	operandRoles << ThingFilter::ContainsRole << ThingFilter::EqualRole << ThingFilter::AboveRole << ThingFilter::LowRole;
	actionRoles << ThingFilter::YesRole << ThingFilter::NoRole << ThingFilter::NextRole;
	// Заполняем элемент параметров вещи
	param->clear();
	param->addItem(QString::fromUtf8("Имя вещи"), 0);
	param->addItem(QString::fromUtf8("Тип вещи"), 1);
	param->addItem(QString::fromUtf8("Уровень именной"), 2);
	param->addItem(QString::fromUtf8("Одета"), 3);
	param->addItem(QString::fromUtf8("Цена вещи"), 4);
	param->addItem(QString::fromUtf8("Количество"), 5);
	ThingFilter::ParamRole par_num = rulePtr->param;
	int index = -1;
	for (int i = 0; i < paramRoles.size(); i++) {
		if (paramRoles.at(i) == par_num) {
			index = i;
			break;
		}
	}
	int par_index = param->findData(index);
	param->setCurrentIndex(par_index);
	// Элемент отрицания
	if (rulePtr->negative)
		negative->setCheckState(Qt::Checked);
	// Заполняем элемент операнда
	paramChanged(par_index);
	ThingFilter::OperandRole oper_num = rulePtr->operand;
	index = -1;
	for (int i = 0; i < operandRoles.size(); i++) {
		if (operandRoles.at(i) == oper_num) {
			index = i;
			break;
		}
	}
	operand->setCurrentIndex(operand->findData(index));
	// Заполняем элемент значения
	value_str->setText(rulePtr->value);
	// Заполняем элемент действия
	action->clear();
	action->addItem(QString::fromUtf8("отображать"), 0);
	action->addItem(QString::fromUtf8("не отображать"), 1);
	action->addItem(QString::fromUtf8("следующее правило"), 2);
	ThingFilter::ActionRole action_num = rulePtr->action;
	index = -1;
	for (int i = 0; i < actionRoles.size(); i++) {
		if (actionRoles.at(i) == action_num) {
			index = i;
			break;
		}
	}
	action->setCurrentIndex(param->findData(index));
	// Заполняем элемент цвета
	itemColorBtn->setColor(rulePtr->color);
	enableColor();
	// Сигналы и слоты
	connect(param, SIGNAL(currentIndexChanged(int)), this, SLOT(paramChanged(int)));
	connect(action, SIGNAL(currentIndexChanged(int)), this, SLOT(enableColor()));
	// Удалять диалог после закрытия
	//setAttribute(Qt::WA_DeleteOnClose);
}

ThingRuleEditDialog::~ThingRuleEditDialog()
{
	disconnect(param, SIGNAL(currentIndexChanged(int)), this, SLOT(paramChanged(int)));
}

void ThingRuleEditDialog::paramChanged(int index)
{
	ThingFilter::ParamRole par_num = ThingFilter::NoParamRole;
	int par_data = param->itemData(index).toInt();
	if (par_data >= 0 && par_data < paramRoles.size()) {
		par_num = paramRoles.at(par_data);
	}
	if (par_num == ThingFilter::DressedRole) { // Одето
		operand->setCurrentIndex(-1);
		operand->setEnabled(false);
		negative->setText(QString::fromUtf8("нет"));
		value_str->setText("");
		value_str->setEnabled(false);
	} else {
		operand->setEnabled(true);
		negative->setText(QString::fromUtf8("не"));
		value_str->setEnabled(true);
	}
	// Заполняем элемент операнда
	operand->clear();
	if (par_num == ThingFilter::NameRole) {
		operand->addItem(QString::fromUtf8("содержит"), 0);
	}
	if (par_num == ThingFilter::NameRole || par_num == ThingFilter::TypeRole || par_num == ThingFilter::NamedRole || par_num == ThingFilter::PriceRole || par_num == ThingFilter::CountRole) {
		operand->addItem(QString::fromUtf8("равно"), 1);
	}
	if (par_num == ThingFilter::NamedRole || par_num == ThingFilter::PriceRole || par_num == ThingFilter::CountRole) {
		operand->addItem(QString::fromUtf8("больше"), 2);
	}
	if (par_num == ThingFilter::NamedRole || par_num == ThingFilter::PriceRole || par_num == ThingFilter::CountRole) {
		operand->addItem(QString::fromUtf8("меньше"), 3);
	}
}

void ThingRuleEditDialog::okBtnClick()
{
	int par_index = param->currentIndex();
	if (par_index == -1) {
		QMessageBox::warning(this, QString::fromUtf8("Сохранение правила"), QString::fromUtf8("Необходимо указать параметр вещи"), QMessageBox::Ok);
		param->setFocus();
		return;
	}
	int val_num = 0;
	ThingFilter::OperandRole oper_num = ThingFilter::NoOperRole;
	QString val_str = "";
	ThingFilter::ParamRole par_num = paramRoles.at(param->itemData(par_index).toInt());
	ThingFilter::ActionRole act_num = ThingFilter::NoActionRole;
	if (par_num != ThingFilter::DressedRole) { // Кроме "одето"
		int oper_index = operand->currentIndex();
		if (oper_index == -1) {
			QMessageBox::warning(this, QString::fromUtf8("Сохранение правила"), QString::fromUtf8("Необходимо указать операнд"), QMessageBox::Ok);
			operand->setFocus();
			return;
		}
		oper_num = operandRoles.at(operand->itemData(oper_index).toInt());
		val_str = value_str->text();
		if (val_str.trimmed().isEmpty()) {
			QMessageBox::warning(this, QString::fromUtf8("Сохранение правила"), QString::fromUtf8("Значение должно быть не пустым"), QMessageBox::Ok);
			value_str->setFocus();
			return;
		}
		if (par_num == ThingFilter::NamedRole || par_num == ThingFilter::PriceRole || par_num == ThingFilter::CountRole) {
			bool fOk;
			val_str = val_str.trimmed();
			val_num = val_str.toInt(&fOk);
			if (!fOk) {
				QMessageBox::warning(this, QString::fromUtf8("Сохранение правила"), QString::fromUtf8("Значение должно быть числом"), QMessageBox::Ok);
				value_str->setFocus();
				return;
			}
		} else if (par_num == ThingFilter::TypeRole) {
			val_str = val_str.trimmed();
			val_num = thingTypeFromString(val_str);
			if (val_num == -1) {
				QMessageBox::warning(this, QString::fromUtf8("Сохранение правила"), QString::fromUtf8("Нет такого типа вещи.\nДоступные типы вещей: ") + thingTypes(), QMessageBox::Ok);
				value_str->setFocus();
				return;
			}
		}
	}
	int act_index = action->currentIndex();
	if (act_index == -1) {
		QMessageBox::warning(this, QString::fromUtf8("Сохранение правила"), QString::fromUtf8("Необходимо указать действие"), QMessageBox::Ok);
		action->setFocus();
		return;
	}
	act_num = actionRoles.at(action->itemData(act_index).toInt());
	// Записываем данные из формы в структуру
	savedRulePtr->param = par_num;
	savedRulePtr->negative = (negative->checkState() == Qt::Checked) ? true : false;
	savedRulePtr->operand = oper_num;
	savedRulePtr->int_value = val_num;
	savedRulePtr->value = val_str;
	savedRulePtr->action = act_num;
	savedRulePtr->color = itemColorBtn->getColor();
	accept();
}

void ThingRuleEditDialog::enableColor()
{
	bool enabl = false;
	if (actionRoles.at(action->itemData(action->currentIndex()).toInt()) == ThingFilter::YesRole)
			enabl = true;
	itemColorLb->setEnabled(enabl);
	itemColorBtn->setEnabled(enabl);
}
