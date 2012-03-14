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

// Значения отправляемые окну

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
