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

#ifndef __ELFNINJA_DUMP_STRINGS_H__
#define __ELFNINJA_DUMP_STRINGS_H__

#include "elfninja/core/error.h"
#include "elfninja/core/elf.h"

typedef struct enjd_strings_dumper
{
    int no_indices;
    int use_quotes;
    int show_empty;
    char unprintable_char;
} enjd_strings_dumper;

enjd_strings_dumper* enjd_strings_dumper_create(enj_error** err);
void enjd_strings_dumper_delete(enjd_strings_dumper* sd);

int enjd_strings_dumper_run(enjd_strings_dumper* sd, void const* data, size_t length, int fd, enj_error** err);

#endif // __ELFNINJA_DUMP_STRINGS_H__
