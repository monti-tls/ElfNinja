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

#include "ehdr.h"
#include "tool.h"
#include "log.h"

#include "elfninja/core/core.h"
#include "elfninja/input/input.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fnmatch.h>

int enjp_ehdr_set_help()
{
    return 0;
}

int enjp_ehdr_set_run(enjp_ehdr_tool* p, enji_cmdline_argument* arg, enj_error** err)
{
    if (!p || !p->cmd || !arg || !p->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enji_cmdline_option* opt = 0;

    if ((opt = enji_cmdline_find_option(p->cmd, "entry", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        size_t value = enji_parse_address(p->elf, opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }

        ENJ_ELF_EHDR_SET(p->elf, e_entry, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "phoff", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        size_t value = enji_parse_offset(p->elf, opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }

        ENJ_ELF_EHDR_SET(p->elf, e_phoff, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "shoff", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        size_t value = enji_parse_offset(p->elf, opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }

        ENJ_ELF_EHDR_SET(p->elf, e_shoff, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "shnum", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_EHDR_SET(p->elf, e_shnum, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "phnum", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_EHDR_SET(p->elf, e_phnum, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "shstrndx", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_EHDR_SET(p->elf, e_shstrndx, value);
    }

    // Write header (not pushing) to overrite auto managed fields such as shoff or shnum
    if (enj_elf_write_header(p->elf, err) < 0 ||
        enj_elf_push(p->elf, err) < 0)
    {
        enjp_error(err, "Unable to push changes to ELF");
        goto fail;
    }

    return 0;

fail:
    return -1;
}

static enjp_ehdr_command _this_cmd =
{
    "set",
    "Modify ELF file header",
    &enjp_ehdr_set_help,
    &enjp_ehdr_set_run
};

static __attribute__((constructor(201))) void _register()
{
    enj_error* err = 0;

    if (enjp_ehdr_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register ehdr command 'set'");
}
