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

int enjp_shdr_add_help()
{
    return 0;
}

int enjp_shdr_add_run(enjp_shdr_tool* s, enji_cmdline_argument* arg, enj_error** err)
{
    if (!s || !s->cmd || !arg || !s->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    const char* name = 0;
    size_t sh_type = SHT_NULL;
    size_t sh_flags = 0;
    size_t sh_addr = 0;
    size_t sh_offset = 0;
    size_t sh_size = 0;
    size_t sh_link = 0;
    size_t sh_info = 0;
    size_t sh_entsize = 0;
    size_t sh_addralign = 0;

    enji_cmdline_option* opt = 0;

    if ((opt = enji_cmdline_find_option(s->cmd, "name", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        name = opt->value;
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "type", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_type = enji_parse_sh_type(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "flags", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_flags = enji_parse_sh_flags(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "addr", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_addr = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "offset", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_offset = enji_parse_offset(s->elf, opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "size", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_size = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "link", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_link = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "info", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_info = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "entsize", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_entsize = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(s->cmd, "addralign", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        sh_addralign = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    enj_elf_shdr* section = enj_elf_new_shdr(s->elf, sh_type, name, err);
    if (!section)
    {
        enjp_error(err, "Unable to add new section");
        goto fail;
    }

    ENJ_ELF_SHDR_SET(section, sh_flags, sh_flags);
    ENJ_ELF_SHDR_SET(section, sh_addr, sh_addr);
    ENJ_ELF_SHDR_SET(section, sh_offset, sh_offset);
    ENJ_ELF_SHDR_SET(section, sh_size, sh_size);
    ENJ_ELF_SHDR_SET(section, sh_link, sh_link);
    ENJ_ELF_SHDR_SET(section, sh_info, sh_info);
    ENJ_ELF_SHDR_SET(section, sh_entsize, sh_entsize);
    ENJ_ELF_SHDR_SET(section, sh_addralign, sh_addralign);

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
    "add",
    "Add a new section",
    &enjp_shdr_add_help,
    &enjp_shdr_add_run
};

static __attribute__((constructor(200))) void _register()
{
    enj_error* err = 0;

    if (enjp_shdr_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register section command 'add'");
}
