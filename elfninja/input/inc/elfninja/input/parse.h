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

#ifndef __ELFNINJA_INPUT_PARSE_H__
#define __ELFNINJA_INPUT_PARSE_H__

#include "elfninja/core/error.h"
#include "elfninja/core/elf.h"

#include <stddef.h>

typedef struct enji_parse_dict
{
    const char* name;
    size_t value;
} enji_parse_dict;

size_t enji_parse_number(const char* string, enj_error** err);
size_t enji_parse_offset(enj_elf* elf, const char* string, enj_error** err);
size_t enji_parse_address(enj_elf* elf, const char* string, enj_error** err);
char* enji_parse_hex(const char* string, size_t* length, enj_error** err);

size_t enji_parse_field(enji_parse_dict* dict, const char* string, enj_error** err);
size_t enji_parse_sh_type(const char* string, enj_error** err);
size_t enji_parse_sh_flags(const char* string, enj_error** err);
size_t enji_parse_p_type(const char* string, enj_error** err);
size_t enji_parse_p_flags(const char* string, enj_error** err);

#endif // __ELFNINJA_INPUT_PARSE_H__
