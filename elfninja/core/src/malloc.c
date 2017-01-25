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

#include "elfninja/core/malloc.h"

#include <stdlib.h>
#include <memory.h>

void* enj_malloc(size_t bytes)
{
    void* ptr = malloc(bytes);

    if (ptr)
        memset(ptr, 0, bytes);

    return ptr;
}

void* enj_realloc(void* ptr, size_t bytes)
{
    if (!ptr)
        return enj_malloc(bytes);
    else if (!bytes)
    {
        enj_free(ptr);
        return 0;
    }

    return realloc(ptr, bytes);
}

void enj_free(void* ptr)
{
    if (!ptr)
        return;

    free(ptr);
}
