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

#ifndef __ELFNINJA_INPUT_CMDLINE_H__
#define __ELFNINJA_INPUT_CMDLINE_H__

#include "elfninja/core/error.h"
#include "elfninja/core/fstring.h"
#include "elfninja/core/elf.h"

#include <stddef.h>

struct enji_cmdline_option;
struct enji_cmdline_argument;
struct enji_cmdline;

typedef struct enji_cmdline_option
{
    int is_global;
    struct enji_cmdline_argument* base_arg;

    enj_fstring* name;
    char* value;

    struct enji_cmdline_option* next;
    struct enji_cmdline_option* prev;
} enji_cmdline_option;

typedef struct enji_cmdline_argument
{
    size_t index;

    enj_fstring* name;
    char* value;

    struct enji_cmdline_argument* next;
    struct enji_cmdline_argument* prev;
} enji_cmdline_argument;

typedef struct enji_cmdline
{
    int argc;
    char** argv;

    char* tool;
    size_t argument_count;

    enji_cmdline_option* options;
    enji_cmdline_option* last_option;

    enji_cmdline_argument* arguments;
    enji_cmdline_argument* last_argument;
} enji_cmdline;

enum
{
    ENJI_CMDLINE_GLOBAL = 0x01, // Search on global options
    ENJI_CMDLINE_TOOL   = 0x02, // Search on tool options
    ENJI_CMDLINE_ORPHAN = 0x04, // Search on options not attached to a base argument
};

enji_cmdline* enji_cmdline_create(int argc, char** argv, enj_error** err);
void enji_cmdline_delete(enji_cmdline* cmd);

int enji_cmdline_rebase_options(enji_cmdline* cmd, enji_cmdline_argument* base, enj_error** err);

enji_cmdline_option* enji_cmdline_find_option(enji_cmdline* cmd, const char* name, int flags, enji_cmdline_argument* base_arg, enj_error** err);
enji_cmdline_argument* enji_cmdline_find_argument_by_index(enji_cmdline* cmd, size_t index, enj_error** err);
enji_cmdline_argument* enji_cmdline_find_argument_by_name(enji_cmdline* cmd, const char* name, enj_error** err);

#endif // __ELFNINJA_INPUT_CMDLINE_H__
