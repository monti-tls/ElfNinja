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

#ifndef __ELFNINJA_CORE_FSTRING_H__
#define __ELFNINJA_CORE_FSTRING_H__

#include <stdint.h>
#include <stddef.h>

typedef uint32_t enj_fstring_hash_t;

typedef struct enj_fstring
{
    char* string;
    size_t length;
    enj_fstring_hash_t hash;
} enj_fstring;

enj_fstring* enj_fstring_create(const char* string, enj_error** err);
enj_fstring* enj_fstring_create_n(const char* string, size_t size, enj_error** err);
void enj_fstring_delete(enj_fstring* fstr);

enj_fstring_hash_t enj_fstring_hash(const char* string);
enj_fstring_hash_t enj_fstring_hash_n(const char* string, size_t size);

#endif // __ELFNINJA_CORE_FSTRING_H__
