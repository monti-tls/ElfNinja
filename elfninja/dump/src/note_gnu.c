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
#include "elfninja/core/note.h"
#include "elfninja/core/note_gnu.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

enum
{
    #define DEF_FIELD_GNU_ABI_TAG(name, ident, elf_field, attrs, descr) GNU_ABI_TAG_ ## ident,
    #define DEF_FIELD_GNU_BUILD_ID(name, ident, elf_field, attrs, descr) GNU_BUILD_ID_ ## ident,
    #include "elfninja/dump/note_gnu.def"
};

static int _gnu_abi_tag_field(void* arg0, void* arg1, int fd, int width, char pad, enj_error** err)
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
    enj_note* note = (enj_note*) arg1;
    enj_note_gnu_abi_tag* tag = (enj_note_gnu_abi_tag*) note->content;

    if (!tag)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    switch (field)
    {
        case GNU_ABI_TAG_OS:
        {
            switch (tag->os)
            {
                case ELF_NOTE_OS_LINUX:    enjd_padded_fprintf(f, width, pad, "ELF_NOTE_OS_LINUX");    break;
                case ELF_NOTE_OS_GNU:      enjd_padded_fprintf(f, width, pad, "ELF_NOTE_OS_GNU");      break;
                case ELF_NOTE_OS_SOLARIS2: enjd_padded_fprintf(f, width, pad, "ELF_NOTE_OS_SOLARIS2"); break;
                case ELF_NOTE_OS_FREEBSD:  enjd_padded_fprintf(f, width, pad, "ELF_NOTE_OS_FREEBSD");  break;
                default: enjd_padded_fprintf(f, width, pad, "ELF_NOTE_OS_USER(%ld)", tag->os);
            }

            break;
        }

        case GNU_ABI_TAG_ABI_MAJOR:
            fprintf(f, "%ld", tag->abi_major);
            break;

        case GNU_ABI_TAG_ABI_MINOR:
            fprintf(f, "%ld", tag->abi_minor);
            break;

        case GNU_ABI_TAG_ABI_SUBMINOR:
            fprintf(f, "%ld", tag->abi_subminor);
            break;
    }

    fclose(f);
    return 0;
}

static int _gnu_build_id_field(void* arg0, void* arg1, int fd, int width, char pad, enj_error** err)
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
    enj_note* note = (enj_note*) arg1;
    enj_note_gnu_build_id* build_id = (enj_note_gnu_build_id*) note->content;

    if (!build_id)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    switch (field)
    {
        case GNU_BUILD_ID_ID:
        {
            char hex[2 * build_id->length + 1];

            for (size_t i = 0; i < build_id->length; ++i)
                snprintf(&hex[2 * i], sizeof(hex), "%02X", build_id->bytes[i]);

            hex[2 * build_id->length] = '\0';

            enjd_padded_fprintf(f, width, pad, "%s", &hex[0]);
            break;
        }
    }

    fclose(f);
    return 0;
}

enjd_formatter* enjd_note_gnu_abi_tag_formatter_create(enj_error** err)
{
    enjd_formatter* fmt = enjd_formatter_create(err);
    if (!fmt)
        return 0;

    #define DEF_FIELD_GNU_ABI_TAG(name, ident, elf_field, attrs, descr) \
        || !enjd_formatter_new_elem(fmt, name, &_gnu_abi_tag_field, (void*) GNU_ABI_TAG_ ## ident, err)

    if (0
        #include "elfninja/dump/note_gnu.def"
        )
    {
        enjd_formatter_delete(fmt);
        return 0;
    }

    return fmt;
}

enjd_formatter* enjd_note_gnu_build_id_formatter_create(enj_error** err)
{
    enjd_formatter* fmt = enjd_formatter_create(err);
    if (!fmt)
        return 0;

    #define DEF_FIELD_GNU_BUILD_ID(name, ident, elf_field, attrs, descr) \
        || !enjd_formatter_new_elem(fmt, name, &_gnu_build_id_field, (void*) GNU_BUILD_ID_ ## ident, err)

    if (0
        #include "elfninja/dump/note_gnu.def"
        )
    {
        enjd_formatter_delete(fmt);
        return 0;
    }

    return fmt;
}
