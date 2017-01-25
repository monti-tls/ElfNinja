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

int enjp_dump_strings_help()
{
    return 0;
}

int enjp_dump_strings_run(enjp_dump_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    const char* pattern = arg->value;

    // Create the string dumper object
    enjd_strings_dumper* sd = enjd_strings_dumper_create(err);
    if (!sd)
    {
        enjp_error(err, "Unable to create dumper object");
        return -1;
    }

    // Process options for this command
    enji_cmdline_option* opt;
    if ((opt = enji_cmdline_find_option(d->cmd, "no-indices", ENJI_CMDLINE_TOOL, arg, 0)))
        sd->no_indices = 1;
    if ((opt = enji_cmdline_find_option(d->cmd, "use-quotes", ENJI_CMDLINE_TOOL, arg, 0)))
        sd->use_quotes = 1;
    if ((opt = enji_cmdline_find_option(d->cmd, "show-empty", ENJI_CMDLINE_TOOL, arg, 0)))
        sd->show_empty = 1;

    // To manage spacers
    int first_one = 1;

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

        // Only process symbol sections
        int sh_type = ENJ_ELF_SHDR_GET(section, sh_type);
        int sh_flags = ENJ_ELF_SHDR_GET(section, sh_flags);
        if (sh_type != SHT_STRTAB && !(sh_flags & SHF_STRINGS))
        {
            // Warn the user if his filter matches bad sections
            if (pattern)
                enjp_warning(0, "section #%ld (%s) is not an SHT_STRTAB nor have SHF_STRINGS, dumping anyway", section->index, section->cached_name ? section->cached_name->string : "");
            else
                continue;
        }

        // If the section does not have any content, it may be malformed
        if (!section->data)
        {
            enjp_warning(0, "section #%ld (%s) has no content, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

            // And ignore this section
            continue;
        }

        if (!first_one && d->allow_spacers)
        {
            printf("\n");
            fflush(stdout);
        }

        if (d->allow_flourish)
        {
            printf(".:: Strings for section #%ld (%s) ::.\n\n", section->index, section->cached_name ? section->cached_name->string : "");
            fflush(stdout);
        }

        if (enjd_strings_dumper_run(sd, section->elf->blob->buffer + section->data->start->pos, section->data->length, 1, err) < 0)
        {
            enjp_error(err, "Unable to run dumper");
            return -1;
        }

        first_one = 0;
    }

    enjd_strings_dumper_delete(sd);
    return 0;

fail:
    enjd_strings_dumper_delete(sd);
    return -1;
}

static enjp_dump_command _this_cmd =
{
    "strings",
    "Dump string tables",
    &enjp_dump_strings_help,
    &enjp_dump_strings_run
};

static __attribute__((constructor(205))) void _register()
{
    enj_error* err = 0;

    if (enjp_dump_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register dump command 'strings'");
}
