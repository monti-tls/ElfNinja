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

#ifndef __ELFNINJA_CORE_SYMTAB_H__
#define __ELFNINJA_CORE_SYMTAB_H__

#include "elfninja/core/error.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/fstring.h"

#include <elf.h>

struct enj_symtab;
struct enj_symbol;

typedef struct enj_symtab
{
    enj_elf_shdr* section;
    enj_elf_shdr* strtab;

    struct enj_symbol* symbols;
    struct enj_symbol* last_symbol;
} enj_symtab;

typedef struct enj_symbol
{
    enj_symtab* symtab;

    size_t index;
    enj_fstring* cached_name;

    enj_blob_anchor* header;
    enj_blob_anchor* name;
    enj_blob_cursor* target;

    union
    {
        Elf32_Sym sym32;
        Elf64_Sym sym64;
    };

    struct enj_symbol* next;
    struct enj_symbol* prev;
} enj_symbol;

#define ENJ_SYMBOL_SIZE(sym) (sym->symtab->section->elf->bits == 64 ? sizeof(Elf64_Sym) : sizeof(Elf32_Sym))
#define ENJ_SYMBOL_GET(sym, field) (sym->symtab->section->elf->bits == 64 ? sym->sym64.field : sym->sym32.field)
#define ENJ_SYMBOL_SET(sym, field, value) do {\
        if (sym->symtab->section->elf->bits == 64) sym->sym64.field = (value); \
        else sym->sym32.field = (value); \
    } while (0);

#define ENJ_SYMBOL_BIND(sym) \
        (sym->symtab->section->elf->bits == 64 ? ELF64_ST_BIND(ENJ_SYMBOL_GET(sym, st_info)) : ELF32_ST_BIND(ENJ_SYMBOL_GET(sym, st_info)))

#define ENJ_SYMBOL_TYPE(sym) \
        (sym->symtab->section->elf->bits == 64 ? ELF64_ST_TYPE(ENJ_SYMBOL_GET(sym, st_info)) : ELF32_ST_TYPE(ENJ_SYMBOL_GET(sym, st_info)))

#define ENJ_SYMBOL_INFO(sym, bind, type) do {\
        if (sym->symtab->section->elf->bits == 64) sym->sym64.st_info = ELF64_ST_INFO(bind, type); \
        else sym->sym32.st_info = ELF32_ST_INFO(bind, type); \
    } while (0);

#define ENJ_SYMBOL_VISIBILITY(sym) \
        (sym->symtab->section->elf->bits == 64 ? ELF64_ST_VISIBILITY(ENJ_SYMBOL_GET(sym, st_other)) : ELF32_ST_VISIBILITY(ENJ_SYMBOL_GET(sym, st_other)))

enum
{
    ENJ_SYMBOL_CLEAR_NAME   = 0x01,
    ENJ_SYMBOL_DISCARD_NAME = 0x02
};

enj_symbol* enj_symtab_find_symbol(enj_symtab* symtab, const char* name, enj_error** err);
enj_symbol* enj_symtab_new_symbol(enj_symtab* symtab, const char* name, enj_error** err);

int enj_symbol_pull(enj_symbol* sym, enj_error** err);
int enj_symbol_update(enj_symbol* sym, enj_error** err);
int enj_symbol_push(enj_symbol* sym, enj_error** err);
int enj_symbol_rename(enj_symbol* sym, const char* name, enj_error** err);
int enj_symbol_remove(enj_symbol* sym, int flags, enj_error** err);

int enj_symbol__delete(enj_symbol* sym, enj_error** err);

int enj_symtab__pull(enj_elf_shdr* section, enj_error** err);
int enj_symtab__update(enj_elf_shdr* section, enj_error** err);
int enj_symtab__push(enj_elf_shdr* section, enj_error** err);
int enj_symtab__delete(enj_elf_shdr* section, enj_error** err);

#endif // __ELFNINJA_CORE_SYMTAB_H__
