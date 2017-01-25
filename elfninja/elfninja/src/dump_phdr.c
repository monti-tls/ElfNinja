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

static const char* _phdr_header32 =
    "Index Type            Offset           Filesz           Vaddr            Memsz            Flg Align\n"
    "----- --------------- ---------------- ---------------- ---------------- ---------------- --- --------\n";

static const char* _phdr_header64 =
    "Index Type            Offset           Filesz           Vaddr            Memsz            Flg Align\n"
    "----- --------------- ---------------- ---------------- ---------------- ---------------- --- --------\n";

static const char* _phdr_fmt_string =
    "[`!-3index`] `!15type` `offset` `filesz` `vaddr` `memsz` `short_flags` `align`\n";

int enjp_dump_phdr_help()
{
    return 0;
}

int enjp_dump_phdr_run(enjp_dump_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    if (!d || !d->cmd || !arg || !d->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    const char* fmt_string = _phdr_fmt_string;

    // Process options for this command
    enji_cmdline_option* opt;
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
        printf(".:: ELF Program Headers ::.\n\n");
        fflush(stdout);
    }

    // Create the dump formatter associated with program headers
    enjd_formatter* fmt = enjd_phdr_formatter_create(err);
    if (!fmt)
    {
        enjp_error(err, "Unable to create formatter object");
        return -1;
    }

    // Print out a header if the format is the default one and if the user has
    //   nothing to say about it
    if (!enji_cmdline_find_option(d->cmd, "no-headers", ENJI_CMDLINE_TOOL | ENJI_CMDLINE_ORPHAN, arg, 0) &&
        fmt_string == _phdr_fmt_string)
    {
        // Don't forget to flush as libelfninja_dump uses dup()
        printf("%s", d->elf->bits == 64 ? _phdr_header64 : _phdr_header32);
        fflush(stdout);
    }

    // Now process the segments
    for (enj_elf_phdr* segment = d->elf->segments; segment; segment = segment->next)
    {
        // Dump the segment contents
        if (enjd_formatter_run(fmt, fmt_string, (void*) segment, 1, err) < 0)
        {
            enjp_error(err, "Unable to run formatter");
            goto fail;
        }
    }

    enjd_formatter_delete(fmt);
    return 0;

fail:
    enjd_formatter_delete(fmt);
    return -1;
}

static enjp_dump_command _this_cmd =
{
    "phdr",
    "Dump ELF program headers",
    &enjp_dump_phdr_help,
    &enjp_dump_phdr_run
};

static __attribute__((constructor(201))) void _register()
{
    enj_error* err = 0;

    if (enjp_dump_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register dump command 'phdr'");
}
