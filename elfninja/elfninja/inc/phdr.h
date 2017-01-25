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

#ifndef __ELFNINJA_PHDR_H__
#define __ELFNINJA_PHDR_H__

#include "elfninja/core/core.h"
#include "elfninja/input/input.h"

#include "plugin.h"

typedef struct enjp_phdr_tool
{
    enji_cmdline* cmd;
    enj_elf* elf;
} enjp_phdr_tool;

typedef struct enjp_phdr_command
{
    const char* name;
    const char* description;

    int(*help)();
    int(*run)(enjp_phdr_tool*, enji_cmdline_argument*, enj_error**);

    struct enjp_phdr_command* next;
    struct enjp_phdr_command* prev;
} enjp_phdr_command;

ENJP_PLUGIN_API enjp_phdr_command* enjp_phdr_commands();
ENJP_PLUGIN_API enjp_phdr_command* enjp_phdr_resolve_command(const char* name, enj_error** err);
ENJP_PLUGIN_API int enjp_phdr_register_command(enjp_phdr_command* cmd, enj_error** err);

int enjp_phdr_help(enji_cmdline* cmd);
int enjp_phdr_run(enji_cmdline* cmd);

#endif // __ELFNINJA_PHDR_H__
