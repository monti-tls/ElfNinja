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

#include "phdr.h"
#include "tool.h"
#include "log.h"

#include "elfninja/core/core.h"
#include "elfninja/input/input.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fnmatch.h>

int enjp_phdr_set_help()
{
    return 0;
}

int enjp_phdr_set_run(enjp_phdr_tool* p, enji_cmdline_argument* arg, enj_error** err)
{
    if (!p || !p->cmd || !arg || !p->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    size_t index = enji_parse_number(arg->value, err);
    enj_elf_phdr* segment = enj_elf_find_phdr_by_index(p->elf, index, 0);

    if (*err || !segment)
    {
        enjp_error(err, "Invalid segment index.");
        goto fail;
    }

    enji_cmdline_option* opt = 0;

    if ((opt = enji_cmdline_find_option(p->cmd, "type", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        size_t value = enji_parse_p_type(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }

        ENJ_ELF_PHDR_SET(segment, p_type, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "flags", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        size_t value = enji_parse_p_flags(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }

        ENJ_ELF_PHDR_SET(segment, p_flags, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "offset", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_PHDR_SET(segment, p_offset, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "vaddr", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_PHDR_SET(segment, p_vaddr, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "paddr", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_PHDR_SET(segment, p_paddr, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "filesz", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_PHDR_SET(segment, p_filesz, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "memsz", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_PHDR_SET(segment, p_memsz, value);
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "align", ENJI_CMDLINE_TOOL, arg, 0)))
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

        ENJ_ELF_PHDR_SET(segment, p_align, value);
    }

    // Write & pull segment to overrite auto managed fields such as offset and size
    if (enj_elf_phdr_write(segment, err) < 0 ||
        enj_elf_phdr_pull(segment, err) < 0 ||
        enj_elf_push(p->elf, err) < 0)
    {
        enjp_error(err, "Unable to push changes to ELF");
        goto fail;
    }

    return 0;

fail:
    return -1;
}

static enjp_phdr_command _this_cmd =
{
    "set",
    "Modify program header",
    &enjp_phdr_set_help,
    &enjp_phdr_set_run
};

static __attribute__((constructor(201))) void _register()
{
    enj_error* err = 0;

    if (enjp_phdr_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register phdr command 'set'");
}
