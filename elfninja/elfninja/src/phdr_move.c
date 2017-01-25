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

#include "phdr.h"
#include "tool.h"
#include "log.h"

#include "elfninja/core/core.h"
#include "elfninja/input/input.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fnmatch.h>

int enjp_phdr_move_help()
{
    return 0;
}

int enjp_phdr_move_run(enjp_phdr_tool* p, enji_cmdline_argument* arg, enj_error** err)
{
    if (!p || !p->cmd || !arg || !p->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!arg->value)
    {
        enjp_error(0, "No source segment provided. Try 'elfninja help phdr move'");
        goto fail;
    }

    size_t index = enji_parse_number(arg->value, err);
    enj_elf_phdr* source = enj_elf_find_phdr_by_index(p->elf, index, err);

    if (*err || !source)
    {
        enjp_error(err, "Invalid source segment index.");
        goto fail;
    }

    enji_cmdline_option* opt = enji_cmdline_find_option(p->cmd, "to", ENJI_CMDLINE_TOOL, arg, 0);
    if (!opt || !opt->value)
    {
        enjp_error(0, opt ? "Option 'to' requires a value" : "Option 'to' is required");
        goto fail;
    }

    index = enji_parse_number(opt->value, err);
    enj_elf_phdr* dest = enj_elf_find_phdr_by_index(p->elf, index, err);

    if (*err || !dest)
    {
        enjp_error(err, "Invalid destination segment index.");
        goto fail;
    }

    if (enj_elf_phdr_move(source, dest->index, err) < 0)
    {
        enjp_error(err, "Unable to move segment headers");
        goto fail;
    }

    if (enj_elf_push(p->elf, err) < 0)
    {
        enjp_error(err, "Unable to push changes to ELF");
        goto fail;
    }

    return 0;

fail:
    return -1;
}

static enjp_phdr_command _this_cmd =
{
    "move",
    "Move program headers",
    &enjp_phdr_move_help,
    &enjp_phdr_move_run
};

static __attribute__((constructor(203))) void _register()
{
    enj_error* err = 0;

    if (enjp_phdr_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register segment command 'move'");
}
