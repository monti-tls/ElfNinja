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

#ifndef __ELFNINJA_CORE_DYNAMIC_H__
#define __ELFNINJA_CORE_DYNAMIC_H__

#include "elfninja/core/error.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/fstring.h"

#include <elf.h>

struct enj_dynamic;
struct enj_dynamic_entry;

typedef struct enj_dynamic
{
    enj_elf_shdr* section;
    enj_elf_shdr* strtab;

    struct enj_dynamic_entry* entries;
    struct enj_dynamic_entry* last_entry;
} enj_dynamic;

typedef struct enj_dynamic_entry
{
    enj_dynamic* dynamic;

    size_t tag;
    size_t value;

    enj_fstring* cached_string;

    enj_blob_anchor* header;
    enj_blob_anchor* string;

    union
    {
        Elf32_Dyn dyn32;
        Elf64_Dyn dyn64;
    };

    struct enj_dynamic_entry* next;
    struct enj_dynamic_entry* prev;
} enj_dynamic_entry;

#define ENJ_DYNAMIC_ENTRY_SIZE(dyn) (dyn->dynamic->section->elf->bits == 64 ? sizeof(dyn->dyn64) : sizeof(dyn->dyn32))
#define ENJ_DYNAMIC_ENTRY_GET(dyn, field) (dyn->dynamic->section->elf->bits == 64 ? dyn->dyn64.field : dyn->dyn32.field)
#define ENJ_DYNAMIC_ENTRY_SET(dyn, field, value) do {\
        if (dyn->dynamic->section->elf->bits == 64) dyn->dyn64.field = (value); \
        else dyn->dyn32.field = (value); \
    } while (0);

int enj_dynamic_entry_pull(enj_dynamic_entry* dyn, enj_error** err);
int enj_dynamic_entry_update(enj_dynamic_entry* dyn, enj_error** err);
int enj_dynamic_entry_push(enj_dynamic_entry* dyn, enj_error** err);

int enj_dynamic_entry__delete(enj_dynamic_entry* dyn, enj_error** err);

int enj_dynamic__pull(enj_elf_shdr* section, enj_error** err);
int enj_dynamic__update(enj_elf_shdr* section, enj_error** err);
int enj_dynamic__push(enj_elf_shdr* section, enj_error** err);
int enj_dynamic__delete(enj_elf_shdr* section, enj_error** err);

#endif // __ELFNINJA_CORE_DYNAMIC_H__
