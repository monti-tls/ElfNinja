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

#include "shdr.h"
#include "tool.h"
#include "log.h"

#include "elfninja/core/core.h"
#include "elfninja/input/input.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fnmatch.h>

int enjp_shdr_swap_help()
{
    return 0;
}

int enjp_shdr_swap_run(enjp_shdr_tool* s, enji_cmdline_argument* arg, enj_error** err)
{
    if (!s || !s->cmd || !arg || !s->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!arg->value)
    {
        enjp_error(0, "No source section provided. Try 'elfninja help section swap'");
        goto fail;
    }

    enj_elf_shdr* source = enji_pattern_match_unique(s->elf, arg->value, err);
    if (!source)
    {
        enjp_error(err, "Unable to find source header");
        goto fail;
    }

    enji_cmdline_option* opt = enji_cmdline_find_option(s->cmd, "with", ENJI_CMDLINE_TOOL, arg, 0);
    if (!opt || !opt->value)
    {
        enjp_error(0, opt ? "Option 'with' requires a value" : "Option 'with' is required");
        goto fail;
    }

    enj_elf_shdr* dest = enji_pattern_match_unique(s->elf, opt->value, err);
    if (!dest)
    {
        enjp_error(err, "Unable to find destination header");
        goto fail;
    }

    if (enj_elf_shdr_swap(source, dest->index, err) < 0)
    {
        enjp_error(err, "Unable to swap section headers");
        goto fail;
    }

    if (enj_elf_push(s->elf, err) < 0)
    {
        enjp_error(err, "Unable to push changes to ELF");
        goto fail;
    }

    return 0;

fail:
    return -1;
}

static enjp_shdr_command _this_cmd =
{
    "swap",
    "Swap section headers",
    &enjp_shdr_swap_help,
    &enjp_shdr_swap_run
};

static __attribute__((constructor(203))) void _register()
{
    enj_error* err = 0;

    if (enjp_shdr_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register section command 'swap'");
}
