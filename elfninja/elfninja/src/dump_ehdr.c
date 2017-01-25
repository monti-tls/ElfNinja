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

#include "dump.h"
#include "tool.h"
#include "log.h"

#include "elfninja/dump/dump.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

static const char* _ehdr_fmt_string =
    "class      = `class`\n"
    "data       = `data`\n"
    "version    = `version`\n"
    "osabi      = `osabi`\n"
    "abiversion = `abiversion`\n"
    "type       = `type`\n"
    "machine    = `machine`\n"
    "entry      = `entry`\n"
    "phoff      = `phoff`\n"
    "shoff      = `shoff`\n"
    "flags      = `flags`\n"
    "ehsize     = `ehsize`\n"
    "phentsize  = `phentsize`\n"
    "phnum      = `phnum`\n"
    "shentsize  = `shentsize`\n"
    "shnum      = `shnum`\n"
    "shstrndx   = `shstrndx` (`shstr_name`)\n";

int enjp_dump_ehdr_help()
{
    return 0;
}

int enjp_dump_ehdr_run(enjp_dump_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    if (!d || !d->cmd || !arg || !d->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    const char* fmt_string = _ehdr_fmt_string;

    // Process options for this command
    enji_cmdline_option* opt = 0;
    if ((opt = enji_cmdline_find_option(d->cmd, "format", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'format' expects a value");
            return -1;
        }

        fmt_string = opt->value;
    }

    if (d->allow_flourish)
    {
        printf(".:: ELF File Header ::.\n\n");
        fflush(stdout);
    }

    // Create the formatter object, run it on the format
    enjd_formatter* fmt = enjd_ehdr_formatter_create(err);
    if (!fmt)
    {
        enjp_error(err, "Unable to create formatter object");
        return -1;
    }

    if (enjd_formatter_run(fmt, fmt_string, (void*) d->elf, 1, err) < 0)
    {
        enjp_error(err, "Unable to run formatter");
        enjd_formatter_delete(fmt);
        return -1;
    }

    enjd_formatter_delete(fmt);
    return 0;
}

static enjp_dump_command _this_cmd =
{
    "ehdr",
    "Dump ELF file header",
    &enjp_dump_ehdr_help,
    &enjp_dump_ehdr_run
};

static __attribute__((constructor(200))) void _register()
{
    enj_error* err = 0;

    if (enjp_dump_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register dump command 'ehdr'");
}
