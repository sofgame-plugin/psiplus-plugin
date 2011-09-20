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

// Типы данных, отправляемых окну
#define TYPE_INTEGER_INC                1
#define TYPE_INTEGER_FULL               2
#define TYPE_LONGLONG_INC               3
#define TYPE_LONGLONG_FULL              4
#define TYPE_STRING                     5
#define TYPE_NA                         6

//--------------------
#define SLOT_ITEMS_COUNT                9

#define STAT_PARAMS_COUNT               11
// Значения отправляемые окну
#define VALUE_LAST_GAME_JID             0
#define VALUE_LAST_CHAT_JID             1
#define VALUE_MESSAGES_COUNT            2
#define VALUE_DAMAGE_MAX_FROM_PERS      3
#define VALUE_DAMAGE_MIN_FROM_PERS      4
#define VALUE_FIGHTS_COUNT              5
#define VALUE_DROP_MONEYS               6
#define VALUE_THINGS_DROP_COUNT         7
#define VALUE_THING_DROP_LAST           8
#define VALUE_EXPERIENCE_DROP_COUNT     9
#define VALUE_KILLED_ENEMIES            10

#define VALUE_TIMEOUT                   1019

// Типы команд отправляемых ядру плагина
#define COMMAND_SAVE_SETTINGS           1
#define COMMAND_CLOSE_WINDOW            2

// Коды ошибок
#define ERROR_INCORRECT_ACCOUNT         1
#define ERROR_NO_MORE_MIRRORS           2
#define ERROR_SERVER_TIMEOUT            3
#define ERROR_QUICK_COMMAND             4

#endif
