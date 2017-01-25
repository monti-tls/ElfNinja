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
    #define DEF_FIELD(name, ident, elf_field, attrs, descr) SHDR_ ## ident,
    #include "elfninja/dump/shdr.def"
};

static int _shdr_field(void* arg0, void* arg1, int fd, int width, char pad, enj_error** err)
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
    enj_elf_shdr* section = (enj_elf_shdr*) arg1;
    enj_elf* elf = section->elf;

    switch (field)
    {
        case SHDR_INDEX:
            enjd_padded_fprintf(f, width, pad, "%d", (int) section->index);
                break;

        case SHDR_NAME_INDEX:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_ELF_SHDR_GET(section, sh_name)), ENJ_ELF_SHDR_GET(section, sh_name));
            break;

        case SHDR_NAME:
        {
            if (section->cached_name)
                enjd_padded_fprintf(f, width, pad, "%s", section->cached_name->string);

            break;
        }

        case SHDR_TYPE:
        {
            size_t value = ENJ_ELF_SHDR_GET(section, sh_type);

            switch (value)
            {
                case SHT_NULL:           enjd_padded_fprintf(f, width, pad, "SHT_NULL");           break;
                case SHT_PROGBITS:       enjd_padded_fprintf(f, width, pad, "SHT_PROGBITS");       break;
                case SHT_SYMTAB:         enjd_padded_fprintf(f, width, pad, "SHT_SYMTAB");         break;
                case SHT_STRTAB:         enjd_padded_fprintf(f, width, pad, "SHT_STRTAB");         break;
                case SHT_RELA:           enjd_padded_fprintf(f, width, pad, "SHT_RELA");           break;
                case SHT_HASH:           enjd_padded_fprintf(f, width, pad, "SHT_HASH");           break;
                case SHT_DYNAMIC:        enjd_padded_fprintf(f, width, pad, "SHT_DYNAMIC");        break;
                case SHT_NOTE:           enjd_padded_fprintf(f, width, pad, "SHT_NOTE");           break;
                case SHT_NOBITS:         enjd_padded_fprintf(f, width, pad, "SHT_NOBITS");         break;
                case SHT_REL:            enjd_padded_fprintf(f, width, pad, "SHT_REL");            break;
                case SHT_SHLIB:          enjd_padded_fprintf(f, width, pad, "SHT_SHLIB");          break;
                case SHT_DYNSYM:         enjd_padded_fprintf(f, width, pad, "SHT_DYNSYM");         break;
                case SHT_INIT_ARRAY:     enjd_padded_fprintf(f, width, pad, "SHT_INIT_ARRAY");     break;
                case SHT_FINI_ARRAY:     enjd_padded_fprintf(f, width, pad, "SHT_FINI_ARRAY");     break;
                case SHT_PREINIT_ARRAY:  enjd_padded_fprintf(f, width, pad, "SHT_PREINIT_ARRAY");  break;
                case SHT_GROUP:          enjd_padded_fprintf(f, width, pad, "SHT_GROUP");          break;
                case SHT_SYMTAB_SHNDX:   enjd_padded_fprintf(f, width, pad, "SHT_SYMTAB_SHNDX");   break;
                case SHT_GNU_ATTRIBUTES: enjd_padded_fprintf(f, width, pad, "SHT_GNU_ATTRIBUTES"); break;
                case SHT_GNU_HASH:       enjd_padded_fprintf(f, width, pad, "SHT_GNU_HASH");       break;
                case SHT_GNU_LIBLIST:    enjd_padded_fprintf(f, width, pad, "SHT_GNU_LIBLIST");    break;
                case SHT_CHECKSUM:       enjd_padded_fprintf(f, width, pad, "SHT_CHECKSUM");       break;
                case SHT_SUNW_move:      enjd_padded_fprintf(f, width, pad, "SHT_SUNW_move");      break;
                case SHT_SUNW_COMDAT:    enjd_padded_fprintf(f, width, pad, "SHT_SUNW_COMDAT");    break;
                case SHT_SUNW_syminfo:   enjd_padded_fprintf(f, width, pad, "SHT_SUNW_syminfo");   break;
                case SHT_GNU_verdef:     enjd_padded_fprintf(f, width, pad, "SHT_GNU_verdef");     break;
                case SHT_GNU_verneed:    enjd_padded_fprintf(f, width, pad, "SHT_GNU_verneed");    break;
                case SHT_GNU_versym:     enjd_padded_fprintf(f, width, pad, "SHT_GNU_versym");     break;
                default: enjd_padded_fprintf(f, width, pad, "SHT_USER(%0*lX)", (int) sizeof(ENJ_ELF_SHDR_GET(section, sh_type)), value);
            }

            break;
        }

        case SHDR_FLAGS:
        {
            size_t value = ENJ_ELF_SHDR_GET(section, sh_flags);

            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_SHDR_GET(section, sh_flags)), value);

            if (value & SHF_WRITE)            fprintf(f, " SHF_WRITE");
            if (value & SHF_ALLOC)            fprintf(f, " SHF_ALLOC");
            if (value & SHF_EXECINSTR)        fprintf(f, " SHF_EXECINSTR");
            if (value & SHF_MERGE)            fprintf(f, " SHF_MERGE");
            if (value & SHF_STRINGS)          fprintf(f, " SHF_STRINGS");
            if (value & SHF_INFO_LINK)        fprintf(f, " SHF_INFO_LINK");
            if (value & SHF_LINK_ORDER)       fprintf(f, " SHF_LINK_ORDER");
            if (value & SHF_OS_NONCONFORMING) fprintf(f, " SHF_OS_NONCONFORMING");
            if (value & SHF_GROUP)            fprintf(f, " SHF_GROUP");
            if (value & SHF_TLS)              fprintf(f, " SHF_TLS");
            if (value & SHF_ORDERED)          fprintf(f, " SHF_ORDERED");
            if (value & SHF_EXCLUDE)          fprintf(f, " SHF_EXCLUDE");

            break;
        }

        case SHDR_SHORT_FLAGS:
        {
            size_t value = ENJ_ELF_SHDR_GET(section, sh_flags);

            char str[5] = "    ";

            if (value & SHF_WRITE)            str[0] = 'W';
            if (value & SHF_ALLOC)            str[1] = 'A';
            if (value & SHF_EXECINSTR)        str[2] = 'X';
            if (value & SHF_MERGE)            str[3] = '+';
            if (value & SHF_STRINGS)          str[3] = '+';
            if (value & SHF_INFO_LINK)        str[3] = '+';
            if (value & SHF_LINK_ORDER)       str[3] = '+';
            if (value & SHF_OS_NONCONFORMING) str[3] = '+';
            if (value & SHF_GROUP)            str[3] = '+';
            if (value & SHF_TLS)              str[3] = '+';
            if (value & SHF_ORDERED)          str[3] = '+';
            if (value & SHF_EXCLUDE)          str[3] = '+';

            fprintf(f, "%s", &str[0]);
            break;
        }

        case SHDR_ADDR:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_SHDR_GET(section, sh_addr)), ENJ_ELF_SHDR_GET(section, sh_addr));
            break;

        case SHDR_OFFSET:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_SHDR_GET(section, sh_offset)), ENJ_ELF_SHDR_GET(section, sh_offset));
            break;

        case SHDR_SIZE:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_SHDR_GET(section, sh_size)), ENJ_ELF_SHDR_GET(section, sh_size));
            break;

        case SHDR_LINK:
            enjd_padded_fprintf(f, width, pad, "%d", ENJ_ELF_SHDR_GET(section, sh_link));
            break;

        case SHDR_LINK_NAME:
        {
            enj_elf_shdr* link = enj_elf_find_shdr_by_index(elf, ENJ_ELF_SHDR_GET(section, sh_link), 0);
            if (link && link->cached_name)
                enjd_padded_fprintf(f, width, pad, "%s", link->cached_name->string);
            else if (!link)
                enjd_padded_fprintf(f, width, pad, "<corrupt>");
            break;
        }

        case SHDR_INFO:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_ELF_SHDR_GET(section, sh_info)), ENJ_ELF_SHDR_GET(section, sh_info));
            break;

        case SHDR_INFO_NAME:
        {
            if (ENJ_ELF_SHDR_GET(section, sh_flags) & SHF_INFO_LINK)
            {
                enj_elf_shdr* info = enj_elf_find_shdr_by_index(elf, ENJ_ELF_SHDR_GET(section, sh_info), 0);
                if (info && info->cached_name)
                    enjd_padded_fprintf(f, width, pad, "%s", info->cached_name->string);
                else if (!info)
                    enjd_padded_fprintf(f, width, pad, "<corrupt>");
            }
            else
                enjd_padded_fprintf(f, width, pad, "<N/A>");
            break;
        }

        case SHDR_ADDRALIGN:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_SHDR_GET(section, sh_addralign)), ENJ_ELF_SHDR_GET(section, sh_addralign));
            break;

        case SHDR_ENTSIZE:
            fprintf(f, "%02lX", ENJ_ELF_SHDR_GET(section, sh_entsize));
            break;
    }

    fclose(f);
    return 0;
}

enjd_formatter* enjd_shdr_formatter_create(enj_error** err)
{
    enjd_formatter* fmt = enjd_formatter_create(err);
    if (!fmt)
        return 0;

    #define DEF_FIELD(name, ident, elf_field, attrs, descr) \
        || !enjd_formatter_new_elem(fmt, name, &_shdr_field, (void*) SHDR_ ## ident, err)

    if (0
        #include "elfninja/dump/shdr.def"
        )
    {
        enjd_formatter_delete(fmt);
        return 0;
    }

    return fmt;
}
