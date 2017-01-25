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

#include "elfninja/dump/dynamic_entry.h"
#include "elfninja/core/dynamic.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum
{
    #define DEF_FIELD(name, ident, elf_field, attrs, descr) DYNAMIC_ENTRY_ ## ident,
    #include "elfninja/dump/dynamic_entry.def"
};

static int _dynamic_entry_field(void* arg0, void* arg1, int fd, int width, char pad, enj_error** err)
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
    enj_dynamic_entry* dyn = (enj_dynamic_entry*) arg1;

    switch (field)
    {
        case DYNAMIC_ENTRY_TAG:
        {
            size_t tag = ENJ_DYNAMIC_ENTRY_GET(dyn, d_tag);

            switch (tag)
            {
                case DT_NULL:            enjd_padded_fprintf(f, width, pad, "DT_NULL");            break;
                case DT_NEEDED:          enjd_padded_fprintf(f, width, pad, "DT_NEEDED");          break;
                case DT_PLTRELSZ:        enjd_padded_fprintf(f, width, pad, "DT_PLTRELSZ");        break;
                case DT_PLTGOT:          enjd_padded_fprintf(f, width, pad, "DT_PLTGOT");          break;
                case DT_HASH:            enjd_padded_fprintf(f, width, pad, "DT_HASH");            break;
                case DT_STRTAB:          enjd_padded_fprintf(f, width, pad, "DT_STRTAB");          break;
                case DT_SYMTAB:          enjd_padded_fprintf(f, width, pad, "DT_SYMTAB");          break;
                case DT_RELA:            enjd_padded_fprintf(f, width, pad, "DT_RELA");            break;
                case DT_RELASZ:          enjd_padded_fprintf(f, width, pad, "DT_RELASZ");          break;
                case DT_RELAENT:         enjd_padded_fprintf(f, width, pad, "DT_RELAENT");         break;
                case DT_STRSZ:           enjd_padded_fprintf(f, width, pad, "DT_STRSZ");           break;
                case DT_SYMENT:          enjd_padded_fprintf(f, width, pad, "DT_SYMENT");          break;
                case DT_INIT:            enjd_padded_fprintf(f, width, pad, "DT_INIT");            break;
                case DT_FINI:            enjd_padded_fprintf(f, width, pad, "DT_FINI");            break;
                case DT_SONAME:          enjd_padded_fprintf(f, width, pad, "DT_SONAME");          break;
                case DT_RPATH:           enjd_padded_fprintf(f, width, pad, "DT_RPATH");           break;
                case DT_SYMBOLIC:        enjd_padded_fprintf(f, width, pad, "DT_SYMBOLIC");        break;
                case DT_REL:             enjd_padded_fprintf(f, width, pad, "DT_REL");             break;
                case DT_RELSZ:           enjd_padded_fprintf(f, width, pad, "DT_RELSZ");           break;
                case DT_RELENT:          enjd_padded_fprintf(f, width, pad, "DT_RELENT");          break;
                case DT_PLTREL:          enjd_padded_fprintf(f, width, pad, "DT_PLTREL");          break;
                case DT_DEBUG:           enjd_padded_fprintf(f, width, pad, "DT_DEBUG");           break;
                case DT_TEXTREL:         enjd_padded_fprintf(f, width, pad, "DT_TEXTREL");         break;
                case DT_JMPREL:          enjd_padded_fprintf(f, width, pad, "DT_JMPREL");          break;
                case DT_BIND_NOW:        enjd_padded_fprintf(f, width, pad, "DT_BIND_NOW");        break;
                case DT_INIT_ARRAY:      enjd_padded_fprintf(f, width, pad, "DT_INIT_ARRAY");      break;
                case DT_FINI_ARRAY:      enjd_padded_fprintf(f, width, pad, "DT_FINI_ARRAY");      break;
                case DT_INIT_ARRAYSZ:    enjd_padded_fprintf(f, width, pad, "DT_INIT_ARRAYSZ");    break;
                case DT_FINI_ARRAYSZ:    enjd_padded_fprintf(f, width, pad, "DT_FINI_ARRAYSZ");    break;
                case DT_RUNPATH:         enjd_padded_fprintf(f, width, pad, "DT_RUNPATH");         break;
                case DT_FLAGS:           enjd_padded_fprintf(f, width, pad, "DT_FLAGS");           break;
                // case DT_ENCODING:        enjd_padded_fprintf(f, width, pad, "DT_ENCODING");        break;
                case DT_PREINIT_ARRAY:   enjd_padded_fprintf(f, width, pad, "DT_PREINIT_ARRAY");   break;
                case DT_PREINIT_ARRAYSZ: enjd_padded_fprintf(f, width, pad, "DT_PREINIT_ARRAYSZ"); break;
                case DT_GNU_PRELINKED:   enjd_padded_fprintf(f, width, pad, "DT_GNU_PRELINKED");   break;
                case DT_GNU_CONFLICTSZ:  enjd_padded_fprintf(f, width, pad, "DT_GNU_CONFLICTSZ");  break;
                case DT_GNU_LIBLISTSZ:   enjd_padded_fprintf(f, width, pad, "DT_GNU_LIBLISTSZ");   break;
                case DT_CHECKSUM:        enjd_padded_fprintf(f, width, pad, "DT_CHECKSUM");        break;
                case DT_PLTPADSZ:        enjd_padded_fprintf(f, width, pad, "DT_PLTPADSZ");        break;
                case DT_MOVEENT:         enjd_padded_fprintf(f, width, pad, "DT_MOVEENT");         break;
                case DT_MOVESZ:          enjd_padded_fprintf(f, width, pad, "DT_MOVESZ");          break;
                case DT_FEATURE_1:       enjd_padded_fprintf(f, width, pad, "DT_FEATURE_1");       break;
                case DT_POSFLAG_1:       enjd_padded_fprintf(f, width, pad, "DT_POSFLAG_1");       break;
                case DT_SYMINSZ:         enjd_padded_fprintf(f, width, pad, "DT_SYMINSZ");         break;
                case DT_SYMINENT:        enjd_padded_fprintf(f, width, pad, "DT_SYMINENT");        break;
                case DT_GNU_HASH:        enjd_padded_fprintf(f, width, pad, "DT_GNU_HASH");        break;
                case DT_TLSDESC_PLT:     enjd_padded_fprintf(f, width, pad, "DT_TLSDESC_PLT");     break;
                case DT_TLSDESC_GOT:     enjd_padded_fprintf(f, width, pad, "DT_TLSDESC_GOT");     break;
                case DT_GNU_CONFLICT:    enjd_padded_fprintf(f, width, pad, "DT_GNU_CONFLICT");    break;
                case DT_GNU_LIBLIST:     enjd_padded_fprintf(f, width, pad, "DT_GNU_LIBLIST");     break;
                case DT_CONFIG:          enjd_padded_fprintf(f, width, pad, "DT_CONFIG");          break;
                case DT_DEPAUDIT:        enjd_padded_fprintf(f, width, pad, "DT_DEPAUDIT");        break;
                case DT_AUDIT:           enjd_padded_fprintf(f, width, pad, "DT_AUDIT");           break;
                case DT_PLTPAD:          enjd_padded_fprintf(f, width, pad, "DT_PLTPAD");          break;
                case DT_MOVETAB:         enjd_padded_fprintf(f, width, pad, "DT_MOVETAB");         break;
                case DT_SYMINFO:         enjd_padded_fprintf(f, width, pad, "DT_SYMINFO");         break;
                case DT_VERSYM:          enjd_padded_fprintf(f, width, pad, "DT_VERSYM");          break;
                case DT_RELACOUNT:       enjd_padded_fprintf(f, width, pad, "DT_RELACOUNT");       break;
                case DT_RELCOUNT:        enjd_padded_fprintf(f, width, pad, "DT_RELCOUNT");        break;
                case DT_FLAGS_1:         enjd_padded_fprintf(f, width, pad, "DT_FLAGS_1");         break;
                case DT_VERDEF:          enjd_padded_fprintf(f, width, pad, "DT_VERDEF");          break;
                case DT_VERDEFNUM:       enjd_padded_fprintf(f, width, pad, "DT_VERDEFNUM");       break;
                case DT_VERNEED:         enjd_padded_fprintf(f, width, pad, "DT_VERNEED");         break;
                case DT_VERNEEDNUM:      enjd_padded_fprintf(f, width, pad, "DT_VERNEEDNUM");      break;
                case DT_AUXILIARY:       enjd_padded_fprintf(f, width, pad, "DT_AUXILIARY");       break;
                case DT_FILTER:          enjd_padded_fprintf(f, width, pad, "DT_FILTER");          break;
                default: enjd_padded_fprintf(f, width, pad, "DT_USER(%0*lX)", 2 * (int) sizeof(ENJ_DYNAMIC_ENTRY_GET(dyn, d_tag)), tag);
            }

            break;
        }

        case DYNAMIC_ENTRY_RAW_VALUE:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_DYNAMIC_ENTRY_GET(dyn, d_un.d_val)), ENJ_DYNAMIC_ENTRY_GET(dyn, d_un.d_val));
            break;

        case DYNAMIC_ENTRY_VALUE:
        {
            size_t tag = ENJ_DYNAMIC_ENTRY_GET(dyn, d_tag);

            if (dyn->cached_string)
            {
                enjd_padded_fprintf(f, width, pad, "%s", dyn->cached_string->string);
            }
            else
            {
                switch (tag)
                {
                    case DT_NULL:
                    case DT_SYMBOLIC:
                    case DT_TEXTREL:
                    case DT_BIND_NOW:
                        enjd_padded_fprintf(f, width, pad, "N/A");
                        break;

                    default:
                        enjd_padded_fprintf(f, width, pad, "%0*lX", 2 * (int) sizeof(ENJ_DYNAMIC_ENTRY_GET(dyn, d_un.d_val)), ENJ_DYNAMIC_ENTRY_GET(dyn, d_un.d_val));
                        break;
                }
            }

            break;
        }
    }

    fclose(f);
    return 0;
}

enjd_formatter* enjd_dynamic_entry_formatter_create(enj_error** err)
{
    enjd_formatter* fmt = enjd_formatter_create(err);
    if (!fmt)
        return 0;

    #define DEF_FIELD(name, ident, elf_field, attrs, descr) \
        || !enjd_formatter_new_elem(fmt, name, &_dynamic_entry_field, (void*) DYNAMIC_ENTRY_ ## ident, err)

    if (0
        #include "elfninja/dump/dynamic_entry.def"
        )
    {
        enjd_formatter_delete(fmt);
        return 0;
    }

    return fmt;
}
