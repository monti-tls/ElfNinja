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

#define _GNU_SOURCE

#include "elfninja/dump/symbol.h"
#include "elfninja/core/symtab.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

enum
{
    #define DEF_FIELD(name, ident, elf_field, attrs, descr) SYMBOL_ ## ident,
    #include "elfninja/dump/symbol.def"
};

static int _symbol_field(void* arg0, void* arg1, int fd, int width, char pad, enj_error** err)
{
    if (!arg1)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    FILE* f = fdopen(dup(fd), "ab");
    if (!f)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    size_t field = (size_t) arg0;
    enj_symbol* sym = (enj_symbol*) arg1;
    enj_elf* elf = sym->symtab->section->elf;

    switch (field)
    {
        case SYMBOL_INDEX:
            enjd_padded_fprintf(f, width, pad, "%d", (int) sym->index);
            break;

        case SYMBOL_NAME_INDEX:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_SYMBOL_GET(sym, st_name)), ENJ_SYMBOL_GET(sym, st_name));
            break;

        case SYMBOL_NAME:
        {
            if (sym->cached_name)
                enjd_padded_fprintf(f, width, pad, "%s", sym->cached_name->string);

            break;
        }

        case SYMBOL_VALUE:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_SYMBOL_GET(sym, st_value)), ENJ_SYMBOL_GET(sym, st_value));
            break;

        case SYMBOL_SIZE:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_SYMBOL_GET(sym, st_size)), ENJ_SYMBOL_GET(sym, st_size));
            break;

        case SYMBOL_INFO:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_SYMBOL_GET(sym, st_info)), ENJ_SYMBOL_GET(sym, st_info));
            break;

        case SYMBOL_BIND:
        {
            size_t value = ENJ_SYMBOL_BIND(sym);

            switch (value)
            {
                case STB_LOCAL:      enjd_padded_fprintf(f, width, pad, "STB_LOCAL");      break;
                case STB_GLOBAL:     enjd_padded_fprintf(f, width, pad, "STB_GLOBAL");     break;
                case STB_WEAK:       enjd_padded_fprintf(f, width, pad, "STB_WEAK");       break;
                case STB_GNU_UNIQUE: enjd_padded_fprintf(f, width, pad, "STB_GNU_UNIQUE"); break;
                default: enjd_padded_fprintf(f, width, pad, "STB_USER(%1lX)", value);
            }

            break;
        }

        case SYMBOL_TYPE:
        {
            size_t value = ENJ_SYMBOL_TYPE(sym);

            switch (value)
            {
                case STT_NOTYPE:    enjd_padded_fprintf(f, width, pad, "STT_NOTYPE");    break;
                case STT_OBJECT:    enjd_padded_fprintf(f, width, pad, "STT_OBJECT");    break;
                case STT_FUNC:      enjd_padded_fprintf(f, width, pad, "STT_FUNC");      break;
                case STT_SECTION:   enjd_padded_fprintf(f, width, pad, "STT_SECTION");   break;
                case STT_FILE:      enjd_padded_fprintf(f, width, pad, "STT_FILE");      break;
                case STT_COMMON:    enjd_padded_fprintf(f, width, pad, "STT_COMMON");    break;
                case STT_TLS:       enjd_padded_fprintf(f, width, pad, "STT_TLS");       break;
                case STT_GNU_IFUNC: enjd_padded_fprintf(f, width, pad, "STT_GNU_IFUNC"); break;
                default: enjd_padded_fprintf(f, width, pad, "STT_USER(%1lX)", value);
            }

            break;
        }

        case SYMBOL_OTHER:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_SYMBOL_GET(sym, st_other)), ENJ_SYMBOL_GET(sym, st_other));
            break;

        case SYMBOL_VISIBILITY:
        {
            size_t value = ENJ_SYMBOL_VISIBILITY(sym);

            switch (value)
            {
                case STV_DEFAULT:   enjd_padded_fprintf(f, width, pad, "STV_DEFAULT");   break;
                case STV_INTERNAL:  enjd_padded_fprintf(f, width, pad, "STV_INTERNAL");  break;
                case STV_HIDDEN:    enjd_padded_fprintf(f, width, pad, "STV_HIDDEN");    break;
                case STV_PROTECTED: enjd_padded_fprintf(f, width, pad, "STV_PROTECTED"); break;
                default: enjd_padded_fprintf(f, width, pad, "STV_USER(%1lX)", value);
            }

            break;
        }

        case SYMBOL_SHNDX:
            enjd_padded_fprintf(f, width, pad, "%d", (int) ENJ_SYMBOL_GET(sym, st_shndx));
            break;

        case SYMBOL_SH_NAME:
        {
            size_t index = ENJ_SYMBOL_GET(sym, st_shndx);

            if (index == SHN_UNDEF)
            {
                enjd_padded_fprintf(f, width, pad, "SHN_UNDEF");
            }
            else if (index >= SHN_LORESERVE)
            {
                switch (index)
                {
                    case SHN_BEFORE: enjd_padded_fprintf(f, width, pad, "SHN_BEFORE"); break;
                    case SHN_AFTER:  enjd_padded_fprintf(f, width, pad, "SHN_AFTER");  break;
                    case SHN_ABS:    enjd_padded_fprintf(f, width, pad, "SHN_ABS");    break;
                    case SHN_COMMON: enjd_padded_fprintf(f, width, pad, "SHN_COMMON"); break;
                    case SHN_XINDEX: enjd_padded_fprintf(f, width, pad, "SHN_XINDEX"); break;
                    default: enjd_padded_fprintf(f, width, pad, "SHN_RESERVED(%ld)", index);
                }
            }
            else
            {
                enj_elf_shdr* link = enj_elf_find_shdr_by_index(elf, index, 0);
                if (link && link->cached_name)
                    enjd_padded_fprintf(f, width, pad, "%s", link->cached_name->string);
                else if (!link)
                    enjd_padded_fprintf(f, width, pad, "<corrupt>");
            }

            break;
        }
    }

    fclose(f);
    return 0;
}

enjd_formatter* enjd_symbol_formatter_create(enj_error** err)
{
    enjd_formatter* fmt = enjd_formatter_create(err);
    if (!fmt)
        return 0;

    #define DEF_FIELD(name, ident, elf_field, attrs, descr) \
        || !enjd_formatter_new_elem(fmt, name, &_symbol_field, (void*) SYMBOL_ ## ident, err)

    if (0
        #include "elfninja/dump/symbol.def"
        )
    {
        enjd_formatter_delete(fmt);
        return 0;
    }

    return fmt;
}
