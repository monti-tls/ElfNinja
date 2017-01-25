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

static const char* _note_gnu_abi_tag_header =
    "GNU ABI tag: ";

static const char* _note_gnu_abi_tag_fmt_string =
    "`os`, ABI version `abi_major`.`abi_minor`.`abi_subminor`\n";

static const char* _note_gnu_build_id_header =
    "GNU unique build ID: ";

static const char* _note_gnu_build_id_fmt_string =
    "`id`\n";

int enjp_dump_notes_help()
{
    return 0;
}

int enjp_dump_notes_run(enjp_dump_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    const char* pattern = arg->value;
    const char* fmt_string = 0;

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

    if (fmt_string != 0 && !pattern)
    {
        enjp_warning(0, "Format will be used regardless of note type");
    }

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
                    return -1;
                }

                continue;
            }
        }

        // Only process symbol sections
        if (!section->content_view || section->content_view->tag != ENJ_ELF_NOTE)
        {
            // Warn the user if his filter matches bad sections
            if (pattern)
                enjp_warning(0, "Section #%ld (%s) does not contain any note, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

            // And ignore this section
            continue;
        }

        // If the section does not have any content, it may be malformed
        if (!section->content)
        {
            enjp_warning(0, "Section #%ld (%s) has no content, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

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
            printf(".:: Notes for section #%ld (%s) ::.\n\n", section->index, section->cached_name ? section->cached_name->string : "");
            fflush(stdout);
        }

        enj_nsect* nsect = (enj_nsect*) section->content;

        for (enj_note* note = nsect->notes; note; note = note->next)
        {
            // Create the dump format associated with the note type
            enjd_formatter* fmt = enjd_note_formatter_create_by_tag(note->content_view->tag, err);
            if (!fmt)
            {
                enjp_error(err, "Unable to create formatter object");
                return -1;
            }

            // Print the header if needed
            if (!enji_cmdline_find_option(d->cmd, "no-headers", ENJI_CMDLINE_TOOL | ENJI_CMDLINE_ORPHAN, arg, 0) &&
                !fmt_string)
            {
                const char* header = 0;

                switch (note->content_view->tag)
                {
                    case ENJ_NOTE_GNU_ABI_TAG:
                        header = _note_gnu_abi_tag_header;
                        break;

                    case ENJ_NOTE_GNU_BUILD_ID:
                        header = _note_gnu_build_id_header;
                        break;
                }

                if (header)
                {
                    printf("%s", header);
                    fflush(stdout);
                }
            }

            // Get default format string for note contents if not user-provided
            const char* note_fmt_string = fmt_string;
            if (!note_fmt_string)
            {
                switch (note->content_view->tag)
                {
                    case ENJ_NOTE_GNU_ABI_TAG:
                        note_fmt_string = _note_gnu_abi_tag_fmt_string;
                        break;

                    case ENJ_NOTE_GNU_BUILD_ID:
                        note_fmt_string = _note_gnu_build_id_fmt_string;
                        break;

                    default:
                        enjp_error(0, "No default format string for note tag %ld.", note->content_view->tag);
                        break;
                }
            }

            // Run the formatter
            if (enjd_formatter_run(fmt, note_fmt_string, (void*) note, 1, err) < 0)
            {
                enjp_error(err, "Unable to run formatter");
                enjd_formatter_delete(fmt);
                return -1;
            }

            enjd_formatter_delete(fmt);
        }

        first_one = 0;
    }

    return 0;
}

static enjp_dump_command _this_cmd =
{
    "notes",
    "Dump ELF notes",
    &enjp_dump_notes_help,
    &enjp_dump_notes_run
};

static __attribute__((constructor(203))) void _register()
{
    enj_error* err = 0;

    if (enjp_dump_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register dump command 'notes'");
}
