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

#include "info.h"
#include "tool.h"
#include "log.h"

#include "elfninja/core/core.h"
#include "elfninja/input/input.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

int enjp_info_layout_help()
{
    return 0;
}

int enjp_info_layout_run(enjp_info_tool* s, enji_cmdline_argument* arg, enj_error** err)
{
    if (!s || !s->cmd || !arg || !s->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_trait_layout* lay = enj_trait_layout_create(s->elf, err);
    if (!lay)
    {
        enjp_error(err, "Unable to create free space trait");
        return -1;
    }

    for (enj_trait_layout_chunk* chunk = lay->chunks; chunk; chunk = chunk->next)
    {
        const char* type;

        switch (chunk->type)
        {
            case ENJ_TRAIT_LAYOUT_EHDR:    type = "EHDR";  break;
            case ENJ_TRAIT_LAYOUT_PHDRS:   type = "PHDRS"; break;
            case ENJ_TRAIT_LAYOUT_SHDRS:   type = "SHDRS"; break;
            case ENJ_TRAIT_LAYOUT_SECTION: type = "DATA";  break;
            case ENJ_TRAIT_LAYOUT_FREE:    type = "FREE";  break;
            default: type = "???"; break;
        }

        printf("%-5s %08lX -> %08lX (%6ld bytes)", type, chunk->offset, chunk->offset + chunk->size - 1, chunk->size);

        if (chunk->section)
            printf(" #%-2ld (%s)", chunk->section->index, chunk->section->cached_name ? chunk->section->cached_name->string : "");

        printf("\n");
    }

    enj_trait_layout_delete(lay);
    return 0;
}

static enjp_info_command _this_cmd =
{
    "layout",
    "Display ELF file layout",
    &enjp_info_layout_help,
    &enjp_info_layout_run
};

static __attribute__((constructor(201))) void _register()
{
    enj_error* err = 0;

    if (enjp_info_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register info command 'layout'");
}
