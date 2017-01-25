/*
 * This file is part of elfninja
 * Copyright (C) 2017  Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ELFNINJA_ACTION_H__
#define __ELFNINJA_ACTION_H__

#include "elfninja/core/core.h"
#include "elfninja/input/input.h"

#include "plugin.h"

typedef struct enjp_tool
{
    const char* name;
    const char* description;

    int (*help)(enji_cmdline*);
    int (*run)(enji_cmdline*);

    struct enjp_tool* next;
    struct enjp_tool* prev;
} enjp_tool;

ENJP_PLUGIN_API enjp_tool* enjp_tools();
ENJP_PLUGIN_API enjp_tool* enjp_tool_resolve(const char* name, enj_error** err);
ENJP_PLUGIN_API int enjp_tool_register(enjp_tool* tool, enj_error** err);

#endif // __ELFNINJA_ACTION_H__
