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

#include "elfninja/dump/shdr.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

enum
{
    #define DEF_FIELD(name, ident, elf_field, attrs, descr) PHDR_ ## ident,
    #include "elfninja/dump/phdr.def"
};

static int _phdr_field(void* arg0, void* arg1, int fd, int width, char pad, enj_error** err)
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
    enj_elf_phdr* segment = (enj_elf_phdr*) arg1;

    switch (field)
    {
        case PHDR_INDEX:
            enjd_padded_fprintf(f, width, pad, "%d", (int) segment->index);
            break;

        case PHDR_TYPE:
        {
            size_t value = ENJ_ELF_PHDR_GET(segment, p_type);

            switch (value)
            {
                case PT_NULL:         enjd_padded_fprintf(f, width, pad, "PT_NULL");         break;
                case PT_LOAD:         enjd_padded_fprintf(f, width, pad, "PT_LOAD");         break;
                case PT_DYNAMIC:      enjd_padded_fprintf(f, width, pad, "PT_DYNAMIC");      break;
                case PT_INTERP:       enjd_padded_fprintf(f, width, pad, "PT_INTERP");       break;
                case PT_NOTE:         enjd_padded_fprintf(f, width, pad, "PT_NOTE");         break;
                case PT_SHLIB:        enjd_padded_fprintf(f, width, pad, "PT_SHLIB");        break;
                case PT_PHDR:         enjd_padded_fprintf(f, width, pad, "PT_PHDR");         break;
                case PT_TLS:          enjd_padded_fprintf(f, width, pad, "PT_TLS");          break;
                case PT_GNU_EH_FRAME: enjd_padded_fprintf(f, width, pad, "PT_GNU_EH_FRAME"); break;
                case PT_GNU_STACK:    enjd_padded_fprintf(f, width, pad, "PT_GNU_STACK");    break;
                case PT_GNU_RELRO:    enjd_padded_fprintf(f, width, pad, "PT_GNU_RELRO");    break;
                case PT_SUNWBSS:      enjd_padded_fprintf(f, width, pad, "PT_SUNWBSS");      break;
                case PT_SUNWSTACK:    enjd_padded_fprintf(f, width, pad, "PT_SUNWSTACK");    break;
                default: enjd_padded_fprintf(f, width, pad, "PT_USER(%ld)", value); break;
            }

            break;
        }

        case PHDR_OFFSET:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_PHDR_GET(segment, p_offset)), ENJ_ELF_PHDR_GET(segment, p_offset));
            break;

        case PHDR_VADDR:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_PHDR_GET(segment, p_vaddr)), ENJ_ELF_PHDR_GET(segment, p_vaddr));
            break;

        case PHDR_PADDR:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_PHDR_GET(segment, p_paddr)), ENJ_ELF_PHDR_GET(segment, p_paddr));
            break;

        case PHDR_FILESZ:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_PHDR_GET(segment, p_filesz)), ENJ_ELF_PHDR_GET(segment, p_filesz));
            break;

        case PHDR_MEMSZ:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_PHDR_GET(segment, p_memsz)), ENJ_ELF_PHDR_GET(segment, p_memsz));
            break;

        case PHDR_FLAGS:
        {
            size_t value = ENJ_ELF_PHDR_GET(segment, p_flags);

            fprintf(f, "%0*lX", (int) sizeof(ENJ_ELF_PHDR_GET(segment, p_flags)), value);

            if (value & PF_X) fprintf(f, " PF_X");
            if (value & PF_W) fprintf(f, " PF_W");
            if (value & PF_R) fprintf(f, " PF_R");

            break;
        }

        case PHDR_SHORT_FLAGS:
        {
            size_t value = ENJ_ELF_PHDR_GET(segment, p_flags);

            char str[4] = "   ";

            if (value & PF_X) str[0] = 'X';
            if (value & PF_W) str[1] = 'W';
            if (value & PF_R) str[2] = 'R';

            fprintf(f, "%s", &str[0]);
            break;
        }

        case PHDR_ALIGN:
            fprintf(f, "%0*lX", (int) sizeof(ENJ_ELF_PHDR_GET(segment, p_align)), ENJ_ELF_PHDR_GET(segment, p_align));
            break;
    }

    fclose(f);
    return 0;
}

enjd_formatter* enjd_phdr_formatter_create(enj_error** err)
{
    enjd_formatter* fmt = enjd_formatter_create(err);
    if (!fmt)
        return 0;

    #define DEF_FIELD(name, ident, elf_field, attrs, descr) \
        || !enjd_formatter_new_elem(fmt, name, &_phdr_field, (void*) PHDR_ ## ident, err)

    if (0
        #include "elfninja/dump/phdr.def"
        )
    {
        enjd_formatter_delete(fmt);
        return 0;
    }

    return fmt;
}
