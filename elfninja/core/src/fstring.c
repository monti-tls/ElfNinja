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

#include "elfninja/core/error.h"
#include "elfninja/core/malloc.h"
#include "elfninja/core/fstring.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

enj_fstring* enj_fstring_create(const char* string, enj_error** err)
{
    if (!string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    return enj_fstring_create_n(string, strlen(string), err);
}

enj_fstring* enj_fstring_create_n(const char* string, size_t size, enj_error** err)
{
    if (!string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_fstring* fstr = enj_malloc(sizeof(enj_fstring));
    if (!fstr)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    fstr->length = size + 1;
    fstr->string = enj_malloc(fstr->length);
    if (!fstr->string)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        enj_free(fstr);
        return 0;
    }

    memcpy(fstr->string, string, fstr->length);
    fstr->string[size] = '\0';
    fstr->hash = enj_fstring_hash(fstr->string);

    return fstr;
}

void enj_fstring_delete(enj_fstring* fstr)
{
    if (!fstr)
        return;

    enj_free(fstr->string);
    enj_free(fstr);
}

enj_fstring_hash_t enj_fstring_hash(const char* string)
{
    if (!string)
        return 0;

    return enj_fstring_hash_n(string, strlen(string));
}

enj_fstring_hash_t enj_fstring_hash_n(const char* string, size_t size)
{
    enj_fstring_hash_t h = 0;
    enj_fstring_hash_t g;

    for (const char* p = string; size-- && p && *p; )
    {
        h = (h << 4) + *p++;
        if ((g = h & 0xF0000000))
            h ^= g >> 24;
        h &= 0x0FFFFFFF;
    }

    return h;
}
