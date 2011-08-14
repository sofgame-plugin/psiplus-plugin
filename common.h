/*
 * common.h - Sof Game Psi plugin
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

#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#define QINT32_MIN                      qint32(1) << 31

// Статусы персонажа
enum PersStatus {
	NotKnow,
	Stand,
	FightMultiSelect,
	FightOpenBegin,
	FightCloseBegin,
	FightFinish,
	Miniforum,
	SecretBefore,
	SecretGet,
	ThingsList,
	PersInform,
	FightShow,
	OtherPersPos,
	TakeBefore,
	Take,
	KillerAttack,
	Yard,
	MasterRoom1,
	MasterRoom2,
	MasterRoom3,
	DealerBuy,
	DealerSale,
	BuyOk,
	HelpMenu,
	TopList,
	ServerStatistic1,
	ServerStatistic2,
	RealEstate,
	Warehouse,
	WarehouseShelf,
	InKillersCup,
	ThingIsTaken
};

// Типы данных, отправляемых окну
#define TYPE_INTEGER_INC                1
#define TYPE_INTEGER_FULL               2
#define TYPE_STRING                     3
#define TYPE_NA                         4

//--------------------
#define STAT_PARAMS_COUNT               11

#define SLOT_ITEMS_COUNT                9
// Значения отправляемые окну
#define VALUE_LAST_GAME_JID             0
#define VALUE_LAST_CHAT_JID             1
#define VALUE_MESSAGES_COUNT            2
#define VALUE_DAMAGE_MAX_FROM_PERS      3
#define VALUE_DAMAGE_MIN_FROM_PERS      4
#define VALUE_FIGHTS_COUNT              5
#define VALUE_DROP_MONEYS               6
#define VALUE_FINGS_DROP_COUNT          7
#define VALUE_FING_DROP_LAST            8
#define VALUE_EXPERIENCE_DROP_COUNT     9
#define VALUE_KILLED_ENEMIES            10

#define VALUE_PERS_NAME                 1010
#define VALUE_PERS_LEVEL                1011
#define VALUE_EXPERIENCE_CURR           1012
#define VALUE_EXPERIENCE_MAX            1013
#define VALUE_HEALTH_CURR               1014
#define VALUE_HEALTH_MAX                1015
#define VALUE_ENERGY_CURR               1016
#define VALUE_ENERGY_MAX                1017
#define VALUE_PERS_STATUS               1018
#define VALUE_TIMEOUT                   1019
#define VALUE_CHANGE_PERS_POS           1020

// Типы команд отправляемых ядру плагина
#define COMMAND_SAVE_SETTINGS           1
#define COMMAND_CLOSE_WINDOW            2

// Типы настроек
#define SETTING_PERS_NAME                     0
#define SETTING_SLOT1                         1
#define SETTING_SLOT2                         2
#define SETTING_SLOT3                         3
#define SETTING_SLOT4                         4
#define SETTING_SLOT5                         5
#define SETTING_SLOT6                         6
#define SETTING_SLOT7                         7
#define SETTING_SLOT8                         8
#define SETTING_SLOT9                         9
#define SETTING_PERS_PARAM_SAVE_MODE          100
#define SETTING_SAVE_PERS_PARAM               101
#define SETTING_SAVE_PERS_BACKPACK            102
#define SETTING_SAVE_PERS_STAT                103
#define SETTING_CHANGE_MIRROR_MODE            104
#define SETTING_WINDOW_SIZE_POS               105
#define SETTING_WINDOW_POS_X                  106
#define SETTING_WINDOW_POS_Y                  107
#define SETTING_WINDOW_WIDTH                  108
#define SETTING_WINDOW_HEIGHT                 109
#define SETTING_FIGHT_TIMER                   110
#define SETTING_AUTOCLOSE_FIGHT               111
#define SETTING_FING_DROP_POPUP               112
#define SETTING_REST_HEALTH_ENERGY            113
#define SETTING_FIGHT_SELECT_ACTION           114
#define SETTING_KILLER_ATTACK_POPUP           115
#define SETTING_SHOW_QUEUE_LENGTH             116
#define SETTING_PERS_NAME_FONT                117
#define SETTING_SERVER_TEXT_FONT              118
#define SETTING_RESET_QUEUE_FOR_UNKNOW_STATUS 119
#define SETTING_IN_KILLERS_CUP_POPUP          120
#define SETTING_SERVER_TEXT_BLOCKS_COUNT      121
#define SETTING_MAPS_PARAM_SAVE_MODE          122
#define SETTING_RESET_QUEUE_POPUP_SHOW        123

// Коды ошибок
#define ERROR_INCORRECT_ACCOUNT         1
#define ERROR_NO_MORE_MIRRORS           2
#define ERROR_SERVER_TIMEOUT            3
#define ERROR_QUICK_COMMAND             4

#endif
