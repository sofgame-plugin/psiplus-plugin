/*
 * fightparse.cpp - Sof Game Psi plugin
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

#include "fightparse.h"
#include "common.h"
#include "plugin_core.h"
#include "fight.h"
#include "settings.h"

/**
 * Парсит строки с описанием команды союзников, противников и их ауры
 * Возврат - количество принятых парсингом строк
 */
int PluginCore::parseFinghtGroups(QStringList strs, int start_pos)
{
/*
Открытый бой. (*- закр.)
до завершения хода №2: 2мин.0сек.
ваша команда:
 demon/В/[у:27|з:12464/12464]
противник:
 3- матерый волк[95/95]. 8- матерый волк[95/95].
 */
	int n_pos = start_pos;
	int cnt = strs.size();
	if (n_pos >= cnt)
		return 0;
	// Строчка с номером хода и таймаутом
	//n_pos++;
	if (fightOneTimeoutReg.indexIn(strs.at(n_pos).trimmed(), 0) == -1)
		return 0;
	// Устанавливаем номер хода боя
	fight->setStep(fightOneTimeoutReg.cap(1).toInt());
	// Получаем значение таймаута
	int n_timeout = fightOneTimeoutReg.cap(2).toInt() * 60 + fightOneTimeoutReg.cap(3).toInt();
	fight->setTimeout(n_timeout);
	n_pos++;
	if (n_pos < cnt) {
		QString str1 = strs.at(n_pos).trimmed();
		if (str1 == QString::fromUtf8("ваша команда:")) {
			// Анализируем нашу команду
			n_pos++;
			if (n_pos < cnt) {
				QStringList tmp_list = strs.at(n_pos).trimmed().split(QString::fromUtf8("Ауры команды:"));
				// Разбор союзников
				fight->startAddAllyes();
				int pos = 0;
				bool b_my_pers = false;
				QString str1 = tmp_list.at(0).trimmed();
				while ((pos = fightElement0Reg.indexIn(str1, pos)) != -1) {
					QString s_name = fightElement0Reg.cap(1).trimmed();  // Имя члена нашей команды
					int n_health_curr = fightElement0Reg.cap(6).toInt();
					int n_health_max = fightElement0Reg.cap(7).toInt();
					if (fightElement0Reg.cap(5).isEmpty()) {
						// Нет данных об уровне персонажа - значит игровой моб (Медведь, например)
						fight->setGameMobAlly(s_name, n_health_curr, n_health_max);
					} else {
						// Союзник является человеком
						QString s_country = fightElement0Reg.cap(3).trimmed();
						int n_level = fightElement0Reg.cap(5).toInt();
						if (s_name == Pers::instance()->name()) {
							// Наш персонаж
							b_my_pers = true;
							Pers::instance()->beginSetPersParams();
							// Данные об уровне
							Pers::instance()->setPersParams(Pers::ParamPersLevel, TYPE_INTEGER_FULL, n_level);
							// Наше здоровье
							Pers::instance()->setPersParams(Pers::ParamHealthCurr, TYPE_INTEGER_FULL, n_health_curr);
							Pers::instance()->setPersParams(Pers::ParamHealthMax, TYPE_INTEGER_FULL, n_health_max);
							Pers::instance()->endSetPersParams();
						} else {
							// Добавляем описание союзника, управляемого человеком
							fight->setGameHumanAlly(s_name, s_country, n_level, n_health_curr, n_health_max);
						}
					}
					pos += fightElement0Reg.matchedLength();
				}
				fight->setMyPersInFight(b_my_pers);
				if (tmp_list.size() > 1) {
					// Есть данные об аурах
					pos = 0;
					while ((pos = fightElement2Reg.indexIn(tmp_list.at(1), pos)) != -1) {
						QString s_name = fightElement2Reg.cap(1).trimmed();
						QString s_param = fightElement2Reg.cap(2);
						fight->setAuraAlly(s_name, s_param, fightElement2Reg.cap(3).toInt());
						pos += fightElement2Reg.matchedLength();
					}
				}
				fight->stopAddAllyes();
			}
			n_pos++;
			if (n_pos < cnt && strs.at(n_pos).startsWith(QString::fromUtf8("противник:"))) {
				// Строчка наших врагов
				n_pos++;
				if (n_pos < cnt) {
					QStringList tmp_list = strs.at(n_pos).trimmed().split(QString::fromUtf8("Ауры команды:"));
					fight->startAddEnemies();
					// Разбираем и добавляем противников
					int pos = 0;
					QString str1 = tmp_list.at(0).trimmed();
					while ((pos = fightElement1Reg.indexIn(str1, pos)) != -1) {
						int n_index = fightElement1Reg.cap(1).toInt();
						QString s_name = fightElement1Reg.cap(2).trimmed();
						int n_health_curr = fightElement1Reg.cap(7).toInt();
						int n_health_max = fightElement1Reg.cap(8).toInt();
						if (fightElement1Reg.cap(5).trimmed().isEmpty()) {
							// Нет данных об уровне персонажа - значит игровой моб
							fight->setGameMobEnemy(n_index, s_name, n_health_curr, n_health_max);
						} else {
							// Противник управляется человеком
							QString s_country = fightElement1Reg.cap(4).trimmed();
							fight->setHumanEnemy(n_index, s_name, s_country, fightElement1Reg.cap(6).toInt(), n_health_curr, n_health_max);
						}
						pos += fightElement1Reg.matchedLength();
					}
					if (tmp_list.size() > 1) {
						// Анализируем ауры противника
						pos = 0;
						str1 = tmp_list.at(1).trimmed();
						while ((pos = fightElement2Reg.indexIn(str1, pos)) != -1) {
							QString s_name = fightElement2Reg.cap(1).trimmed();
							QString s_param = fightElement2Reg.cap(2).trimmed();
							fight->setAuraEnemy(s_name, s_param, fightElement2Reg.cap(3).toInt());
							pos += fightElement2Reg.matchedLength();
						}
					}
					fight->stopAddEnemies();
				}
			}
		} else if (parPersPower1Reg.indexIn(str1, 0) != -1) {
			// Получаем значение энергии (Это режим выбора умения)
			fight->setMyPersInFight(true); // Наш персонаж в бою
			Pers::instance()->beginSetPersParams();
			Pers::instance()->setPersParams(Pers::ParamEnergyCurr, TYPE_INTEGER_FULL, parPersPower1Reg.cap(1).toInt());
			Pers::instance()->setPersParams(Pers::ParamEnergyMax, TYPE_INTEGER_FULL, parPersPower1Reg.cap(2).toInt());
			Pers::instance()->endSetPersParams();
			// Следующая строчка - выбор умения. Пока тупо пропускаем.
			++n_pos;
		}
	}
	n_pos -= start_pos;
	return n_pos;
}

int PluginCore::parseFinghtStepResult(QStringList strs, int start_pos)
{
	int n_pos = start_pos;
	int cnt = strs.size();
	bool f_res = false;
	// Сначала идут описания атак
	/*
матерый волк, матерый волк, матерый волк, матерый волк, матерый волк, матерый волк, матерый волк, матерый волк атаковали xxxxx/повр:0
xxxxx бросил огненный шар.
xxxxx зацепил волной взрыва шара матерый волк Повреждения: 13088
xxxxx*2 атаковали матерый волк/повр:3280
xxxxx зацепил волной взрыва шара матерый волк Повреждения: 1475
xxxxx зацепил волной взрыва шара матерый волк Повреждения: 1652
xxxxx зацепил волной взрыва шара матерый волк Повреждения: 3049
xxxxx*2 атаковали матерый волк/повр:4524
-----
	*/
	while (n_pos < cnt) {
		QString str = strs.at(n_pos).trimmed();
		if (str == "-----") {
			n_pos++;
			break;
		} else if (fightDamageFromPersReg1.indexIn(str, 0) != -1) {
			f_res = true;
			fight->setDamage(fightDamageFromPersReg1.cap(1).toInt());
		} else if (fightDamageFromPersReg2.indexIn(str, 0) != -1) {
			f_res = true;
		} else if (fightDamageFromPersReg3.indexIn(str, 0) != -1) {
			f_res = true;
			fight->setDamage(0);
		} else if (str.startsWith(QString::fromUtf8("таймауты:"))) {
			f_res = true;
		}
		n_pos++;
	}
	if (!f_res)
		return 0; // Не найдено элементов боя
	// Теперь список выбывших
	/*
Выбыли: матерый волк, матерый волк, матерый волк, матерый волк, матерый волк, матерый волк
-----
	*/
	if (n_pos < cnt) {
		QString str = strs.at(n_pos).trimmed();
		if (str.startsWith(QString::fromUtf8("Выбыли: "))) {
			QStringList killed_list = str.mid(8).split(",");
			int k_cnt = killed_list.size();
			QString s_name = Pers::instance()->name();
			int n_killed_enemies = 0;
			int ally_cnt = fight->allyCount();
			for (int i = 0; i < k_cnt; i++) {
				if (killed_list.at(i).trimmed() == s_name) {
					fight->setMyPersInFight(false);
				} else {
					if (ally_cnt == 0) { // Нет союзников
						n_killed_enemies++;
					}
				}
			}
			// Отправляем сколько противников повержено
			if (n_killed_enemies != 0) {
				statKilledEnemies += n_killed_enemies;
				valueChanged(VALUE_KILLED_ENEMIES, TYPE_INTEGER_FULL, statKilledEnemies);
				statisticsChanged();
			}
			//--
			n_pos++;
			if (n_pos < cnt) {
				if (strs.at(n_pos).trimmed().trimmed() == "-----") {
					n_pos++;
				}
			}
		}
	}
	// Список полученного добра
	/*
Получено: demon +5 дринк;demon +5 дринк;demon +5 дринк;demon +5 дринк
-----
	*/
	if (n_pos < cnt) {
		QString str = strs.at(n_pos).trimmed();
		if (str.startsWith(QString::fromUtf8("Получено: "))) {
			str = str.mid(10);
			QStringList get_list = str.split(";");
			int get_cnt = get_list.size();
			if (get_cnt > 0) {
				int nDropMoneys = 0;
				int nDropThings = 0;
				QString sDropThingLast;
				for (int i = 0; i < get_cnt; i++) {
					str = get_list.at(i).trimmed();
					if (fightDropMoneyReg1.indexIn(str, 0) != -1) { // Ещем упавшие деньги
						nDropMoneys = nDropMoneys + fightDropMoneyReg1.cap(1).toInt();
					} else if (fightDropThingReg1.indexIn(str, 0) != -1) { // Ещем упавшие вещи
						++nDropThings;
						sDropThingLast = fightDropThingReg1.cap(1);
						if (Settings::instance()->getBoolSetting(Settings::SettingThingDropPopup)) {
							initPopup(QString::fromUtf8("Sof game"), "+" + sDropThingLast, 3);
						}
					}
				}
				if (nDropMoneys != 0) {
					statMoneysDropCount += nDropMoneys;
					valueChanged(VALUE_DROP_MONEYS, TYPE_INTEGER_FULL, statMoneysDropCount);
					statisticsChanged();
				}
				if (nDropThings != 0) {
					statFingsDropCount += nDropThings;
					statFingDropLast = sDropThingLast;
					valueChanged(VALUE_FINGS_DROP_COUNT, TYPE_INTEGER_FULL, statFingsDropCount);
					statisticsChanged();
				}
			}
			n_pos++;
			if (n_pos < cnt) {
				if (strs.at(n_pos).trimmed().trimmed() == "-----") {
					n_pos++;
				}
			}
		}
	}
	n_pos -= start_pos;
	return n_pos;
}
