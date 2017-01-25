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

#ifndef __ELFNINJA_DUMP_FORMATTER_H__
#define __ELFNINJA_DUMP_FORMATTER_H__

#include "elfninja/core/error.h"
#include "elfninja/core/fstring.h"

#include <stdio.h>

#define ENJ_DUMP_FORMAT_ESCAPE_CHAR '`'
#define ENJ_DUMP_FORMAT_MODIFIER_CHAR '!'

struct enjd_formatter;
struct enjd_formatter_elem;

typedef int(*enjd_formatter_handler_t)(void*, void*, int fd, int width, char pad, enj_error**);

typedef struct enjd_formatter_elem {
    struct enjd_formatter* fmt;

    enj_fstring* name;
    enjd_formatter_handler_t handler;
    void* arg0;

    struct enjd_formatter_elem* prev;
    struct enjd_formatter_elem* next;
} enjd_formatter_elem;

typedef struct enjd_formatter
{
    char escape0;
    char escape1;
    char modifier;

    enjd_formatter_elem* elems;
    enjd_formatter_elem* last_elem;
} enjd_formatter;

enjd_formatter* enjd_formatter_create(enj_error** err);
void enjd_formatter_delete(enjd_formatter* fmt);

enjd_formatter_elem* enjd_formatter_new_elem(enjd_formatter* fmt, const char* name, enjd_formatter_handler_t handler, void* arg0, enj_error** err);
int enjd_formatter_remove_elem(enjd_formatter* fmt, enjd_formatter_elem* elem, enj_error** err);
enjd_formatter_elem* enjd_formatter_find_elem(enjd_formatter* fmt, const char* name, enj_error** err);

int enjd_formatter_run(enjd_formatter* fmt, const char* string, void* arg1, int fd, enj_error** err);

void enjd_padded_fprintf(FILE* f, int width, char pad, const char* fmt, ...) __attribute__ ((format(printf, 4, 5)));

#endif // __ELFNINJA_DUMP_FORMATTER_H__
