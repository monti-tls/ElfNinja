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

static const char* _shdr_header32 =
    "Index Name                 Type            Address          Offset           Size             Flag ES Lnk Info\n"
    "----- -------------------- --------------- ---------------- ---------------- ---------------- ---- -- --- --------\n";

static const char* _shdr_header64 =
    "Index Name                 Type            Address          Offset           Size             Flag ES Lnk Info\n"
    "----- -------------------- --------------- ---------------- ---------------- ---------------- ---- -- --- --------\n";

static const char* _shdr_fmt_string =
    "[`!-3index`] `!20name` `!15type` `addr` `offset` `size` `short_flags` `entsize` `!-3link` `info`\n";

int enjp_dump_shdr_help()
{
    return 0;
}

int enjp_dump_shdr_run(enjp_dump_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    if (!d || !d->cmd || !arg || !d->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    const char* pattern = arg->value;
    const char* fmt_string = _shdr_fmt_string;

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
        printf(".:: ELF Section Headers ::.\n\n");
        fflush(stdout);
    }

    // Create the dump formatter associated with section headers
    enjd_formatter* fmt = enjd_shdr_formatter_create(err);
    if (!fmt)
    {
        enjp_error(err, "Unable to create formatter object");
        return -1;
    }

    // Print out a header if the format is the default one and if the user has
    //   nothing to say about it
    if (!enji_cmdline_find_option(d->cmd, "no-headers", ENJI_CMDLINE_TOOL | ENJI_CMDLINE_ORPHAN, arg, 0) &&
        fmt_string == _shdr_fmt_string)
    {
        // Don't forget to flush as libelfninja_dump uses dup()
        printf("%s", d->elf->bits == 64 ? _shdr_header64 : _shdr_header32);
        fflush(stdout);
    }

    // Now process the sections
    for (enj_elf_shdr* section = d->elf->sections; section; section = section->next)
    {
        // Process the eventual input pattern
        if (pattern)
        {
            // Ignore unnamed sections
            if (!section->cached_name)
                continue;

            // Ignore sections that do not match the pattern
            int match;
            if ((match = enji_pattern_match(section, pattern, err)) <= 0)
            {
                if (match < 0)
                {
                    enjp_error(err, "Invalid section pattern");
                    goto fail;
                }

                continue;
            }
        }

        // Dump the section contents
        if (enjd_formatter_run(fmt, fmt_string, (void*) section, 1, err) < 0)
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
    "shdr",
    "Dump ELF section headers",
    &enjp_dump_shdr_help,
    &enjp_dump_shdr_run
};

static __attribute__((constructor(201))) void _register()
{
    enj_error* err = 0;

    if (enjp_dump_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register dump command 'shdr'");
}
