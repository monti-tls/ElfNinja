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

#ifndef __ELFNINJA_CORE_TRAIT_LAYOUT_H__
#define __ELFNINJA_CORE_TRAIT_LAYOUT_H__

#include "elfninja/core/error.h"
#include "elfninja/core/elf.h"

enum
{
    ENJ_TRAIT_LAYOUT_FREE,
    ENJ_TRAIT_LAYOUT_EHDR,
    ENJ_TRAIT_LAYOUT_PHDRS,
    ENJ_TRAIT_LAYOUT_SHDRS,
    ENJ_TRAIT_LAYOUT_SECTION
};

typedef struct enj_trait_layout_chunk
{
    size_t offset;
    size_t size;
    size_t type;

    enj_elf_shdr* section;

    struct enj_trait_layout_chunk* prev;
    struct enj_trait_layout_chunk* next;
} enj_trait_layout_chunk;

typedef struct enj_trait_layout
{
    enj_elf_trait trait;

    enj_trait_layout_chunk* chunks;
    enj_trait_layout_chunk* last_chunk;
} enj_trait_layout;

enj_trait_layout* enj_trait_layout_create(enj_elf* elf, enj_error** err);
void enj_trait_layout_delete(enj_trait_layout* fs);

int enj_trait_layout__build(void* arg, enj_error** err);

#endif // __ELFNINJA_CORE_TRAIT_LAYOUT_H__
