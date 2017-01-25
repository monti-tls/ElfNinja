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
#include <fnmatch.h>

static const char* _symbol_header32 =
    "Index  Value            Size             Type          Bind           Visibility    Name                Section\n"
    "------ ---------------- ---------------- ------------- -------------- ------------- ------------------- ----------------\n";

static const char* _symbol_header64 =
    "Index  Value            Size             Type          Bind           Visibility    Name                Section\n"
    "------ ---------------- ---------------- ------------- -------------- ------------- ------------------- ----------------\n";

static const char* _symbol_fmt_string =
    "[`!-4index`] `value` `size` `!13type` `!14bind` `!13visibility` `!20name` `!-3shndx` (`sh_name`)\n";

int enjp_dump_symbols_help()
{
    return 0;
}

int enjp_dump_symbols_run(enjp_dump_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    const char* pattern = arg->value;
    const char* fmt_string = _symbol_fmt_string;
    const char* filter = 0;

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

    if ((opt = enji_cmdline_find_option(d->cmd, "filter", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'filter' expects a value");
            return -1;
        }

        filter = opt->value;
    }

    // Create the dump format associated with section headers
    enjd_formatter* fmt = enjd_symbol_formatter_create(err);
    if (!fmt)
    {
        enjp_error(err, "Unable to create formatter object");
        return -1;
    }

    // Use headers if the default format is used and the user is OK with it
    int allow_headers = 1;
    if (enji_cmdline_find_option(d->cmd, "no-headers", ENJI_CMDLINE_TOOL | ENJI_CMDLINE_ORPHAN, arg, 0) ||
        fmt_string != _symbol_fmt_string)
        allow_headers = 0;

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
        if (!section->content_view || section->content_view->tag != ENJ_ELF_SYMTAB)
        {
            // Warn the user if his filter matches bad sections
            if (pattern)
                enjp_warning(0, "section #%ld (%s) does not contain any symbols, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

            // And ignore this section
            continue;
        }

        // If the section does not have any content, it may be malformed
        if (!section->content)
        {
            enjp_warning(0, "section #%ld (%s) has no content, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

            // And ignore this section
            continue;
        }

        if (!first_one && d->allow_spacers)
        {
            // Don't forget to flush as libelfninja_dump uses dup()
            printf("\n");
            fflush(stdout);
        }

        if (d->allow_flourish)
        {
            printf(".:: Symbols for section #%ld (%s) ::.\n\n", section->index, section->cached_name ? section->cached_name->string : "");
            fflush(stdout);
        }

        if (allow_headers)
        {
            printf("%s", d->elf->bits == 64 ? _symbol_header64 : _symbol_header32);
            fflush(stdout);
        }

        // Dump that shitz !
        enj_symtab* symtab = (enj_symtab*) section->content;
        for (enj_symbol* sym = symtab->symbols; sym; sym = sym->next)
        {
            // Process the eventual symbol filter pattern
            if (filter)
            {
                // Ignore unnamed symbols
                if (!sym->cached_name)
                    continue;

                // Ignore symbols that do not match the pattern
                if (fnmatch(filter, sym->cached_name->string, FNM_EXTMATCH) != 0)
                    continue;
            }

            // Run the formatter
            if (enjd_formatter_run(fmt, fmt_string, (void*) sym, 1, err) < 0)
            {
                enjp_error(err, "Unable to run formatter");
                goto fail;
            }
        }

        first_one = 0;
    }

    enjd_formatter_delete(fmt);
    return 0;

fail:
    enjd_formatter_delete(fmt);
    return -1;
}

static enjp_dump_command _this_cmd =
{
    "symbols",
    "Dump symbol information",
    &enjp_dump_symbols_help,
    &enjp_dump_symbols_run
};

static __attribute__((constructor(202))) void _register()
{
    enj_error* err = 0;

    if (enjp_dump_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register dump command 'symbols'");
}
