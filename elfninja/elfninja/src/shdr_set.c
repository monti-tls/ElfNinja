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

int enjp_shdr_set_help()
{
    return 0;
}

int enjp_shdr_set_run(enjp_shdr_tool* s, enji_cmdline_argument* arg, enj_error** err)
{
    if (!s || !s->cmd || !arg || !s->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    const char* pattern = arg->value;
    if (!pattern)
    {
        enjp_error(0, "No section pattern provided. Try 'elfninja help shdr set'");
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

        enji_cmdline_option* opt = 0;

        if ((opt = enji_cmdline_find_option(s->cmd, "name", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            if (enj_elf_shdr_rename(section, opt->value, err) < 0)
            {
                enjp_error(err, "Unable to rename section #%ld (%s)", section->index, section->cached_name ? section->cached_name->string : 0);
                goto fail;
            }
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "type", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_sh_type(opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_type, value);
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "flags", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_sh_flags(opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_flags, value);
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "addr", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_number(opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_addr, value);
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "offset", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_offset(s->elf, opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_offset, value);
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "size", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_number(opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_size, value);
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "link", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_number(opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_link, value);
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "info", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_number(opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_info, value);
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "entsize", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_number(opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_entsize, value);
        }

        if ((opt = enji_cmdline_find_option(s->cmd, "addralign", ENJI_CMDLINE_TOOL, arg, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            size_t value = enji_parse_number(opt->value, err);
            if (*err)
            {
                enjp_error(err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }

            ENJ_ELF_SHDR_SET(section, sh_addralign, value);
        }

        if (enj_elf_shdr_write(section, err) < 0 ||
            enj_elf_shdr_pull(section, err) < 0 ||
            enj_elf_push(s->elf, err) < 0 ||
            enj_elf_update_sections(s->elf, err) < 0)
        {
            enjp_error(err, "Unable to push changes to section #%ld (%s)\n", section->index, section->cached_name ? section->cached_name->string : 0);
            goto fail;
        }
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
    "set",
    "Modify section headers",
    &enjp_shdr_set_help,
    &enjp_shdr_set_run
};

static __attribute__((constructor(201))) void _register()
{
    enj_error* err = 0;

    if (enjp_shdr_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register section command 'set'");
}
