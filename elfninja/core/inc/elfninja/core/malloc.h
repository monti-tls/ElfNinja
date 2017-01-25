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

#ifndef __ELFNINJA_CORE_MALLOC_H__
#define __ELFNINJA_CORE_MALLOC_H__

#include "elfninja/core/error.h"

#include <stddef.h>
#include <stdlib.h>

void* enj_malloc(size_t bytes);
void* enj_realloc(void* ptr, size_t bytes);
void enj_free(void* ptr);

#endif // __ELFNINJA_CORE_MALLOC_H__
