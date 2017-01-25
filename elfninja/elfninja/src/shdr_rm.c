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

int enjp_shdr_rm_help()
{
    return 0;
}

int enjp_shdr_rm_run(enjp_shdr_tool* s, enji_cmdline_argument* arg, enj_error** err)
{
    if (!s || !s->cmd || !arg || !s->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    const char* pattern = arg->value;
    if (!pattern)
    {
        enjp_error(0, "No section pattern provided. Try 'elfninja help shdr rm'");
        goto fail;
    }

    if (pattern && enji_pattern_is_index(pattern, 0) &&
        !enji_pattern_match_unique(s->elf, pattern, err))
    {
        enjp_error(err, "Invalid section pattern ; removing multiple sections by index is not allowed.");
        goto fail;
    }

    for (enj_elf_shdr* section = s->elf->sections; section; section = section->next)
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

        if (enj_elf_shdr_remove(section, ENJ_ELF_DISCARD_NAME, err) < 0)
        {
            enjp_error(err, "Unable to remove section #%ld (%s)\n", section->index, section->cached_name ? section->cached_name->string : 0);
            goto fail;
        }

        if (enji_pattern_is_index(pattern, 0))
            break;
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
    "rm",
    "Delete section headers",
    &enjp_shdr_rm_help,
    &enjp_shdr_rm_run
};

static __attribute__((constructor(202))) void _register()
{
    enj_error* err = 0;

    if (enjp_shdr_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register section command 'rm'");
}
