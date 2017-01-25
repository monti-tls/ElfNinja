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

int enjp_dump_hex_help()
{
    return 0;
}

int enjp_dump_hex_run(enjp_dump_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    const char* pattern = arg->value;

    // Create the hex dumper object
    enjd_hex_dumper* hd = enjd_hex_dumper_create(err);
    if (!hd)
    {
        enjp_error(err, "Unable to create dumper object");
        return -1;
    }

    // Process options for this command
    enji_cmdline_option* opt;
    if ((opt = enji_cmdline_find_option(d->cmd, "stride", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'stride' expects a value");
            goto fail;
        }

        hd->stride = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Option 'stride' expects a number as value");
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(d->cmd, "grid", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'grid' expects a value");
            goto fail;
        }

        hd->grid = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Option 'grid' expects a number as value");
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(d->cmd, "no-indices", ENJI_CMDLINE_TOOL, arg, 0)))
        hd->no_indices = 1;

    if ((opt = enji_cmdline_find_option(d->cmd, "show-ascii", ENJI_CMDLINE_TOOL, arg, 0)))
        hd->show_ascii = 1;

    if ((opt = enji_cmdline_find_option(d->cmd, "no-hex", ENJI_CMDLINE_TOOL, arg, 0)))
        hd->show_hex = 0;

    if ((opt = enji_cmdline_find_option(d->cmd, "from", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'from' expects a value");
            goto fail;
        }

        if (pattern)
            hd->from = enji_parse_number(opt->value, err);
        else
            hd->from = enji_parse_offset(d->elf, opt->value, err);

        if (*err)
        {
            enjp_error(err, "Invalid value for option 'from'");
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(d->cmd, "to", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'to' expects a value");
            goto fail;
        }

        if (pattern)
            hd->to = enji_parse_number(opt->value, err);
        else
            hd->to = enji_parse_offset(d->elf, opt->value, err);

        if (*err)
        {
            enjp_error(err, "Invalid value for option 'to'");
            goto fail;
        }
    }

    if (pattern && (hd->from >= 0 || hd->to >= 0))
    {
        enjp_warning(0, "Bounds will be used for every matching section");
    }

    if (!pattern)
    {
        if (d->allow_flourish)
        {
            printf(".:: Hex dump for whole file ::.\n\n");
            fflush(stdout);
        }

        if (enjd_hex_dumper_run(hd, d->elf->blob->buffer, d->elf->blob->buffer_size, 1, err) < 0)
        {
            enjp_error(err, "Unable to run dumper");
            return -1;
        }
    }
    else
    {
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

            // If the section does not have any content, it may be malformed
            if (!section->data)
            {
                enjp_warning(0, "Section #%ld (%s) has no content, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

                // And ignore this section
                continue;
            }

            // Ignore empty sections
            if (!section->data->length)
            {
                if (pattern)
                    enjp_warning(0, "Section #%ld (%s) is empty, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

                continue;
            }

            if (!first_one && d->allow_spacers)
            {
                printf("\n");
                fflush(stdout);
            }

            if (d->allow_flourish)
            {
                printf(".:: Hex dump for section #%ld (%s) ::.\n\n", section->index, section->cached_name ? section->cached_name->string : "");
                fflush(stdout);
            }

            if (enjd_hex_dumper_run(hd, section->elf->blob->buffer + section->data->start->pos, section->data->length, 1, err) < 0)
            {
                enjp_error(err, "Unable to run dumper");
                return -1;
            }

            first_one = 0;
        }
    }

    enjd_hex_dumper_delete(hd);
    return 0;

fail:
    enjd_hex_dumper_delete(hd);
    return -1;
}

static enjp_dump_command _this_cmd =
{
    "hex",
    "Hexdump section content",
    &enjp_dump_hex_help,
    &enjp_dump_hex_run
};

static __attribute__((constructor(206))) void _register()
{
    enj_error* err = 0;

    if (enjp_dump_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register dump command 'hex'");
}
