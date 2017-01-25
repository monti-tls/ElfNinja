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

int enjp_phdr_add_help()
{
    return 0;
}

int enjp_phdr_add_run(enjp_phdr_tool* p, enji_cmdline_argument* arg, enj_error** err)
{
    if (!p || !p->cmd || !arg || !p->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    size_t type = PT_NULL;
    size_t flags = 0;
    size_t offset = 0;
    size_t vaddr = 0;
    size_t paddr = 0;
    size_t filesz = 0;
    size_t memsz = 0;
    size_t align = 0;

    enji_cmdline_option* opt = 0;

    if ((opt = enji_cmdline_find_option(p->cmd, "type", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        type = enji_parse_p_type(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "flags", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        flags = enji_parse_p_flags(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "offset", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        offset = enji_parse_offset(p->elf, opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "vaddr", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        vaddr = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "paddr", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        paddr = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "filesz", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        filesz = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "memsz", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        memsz = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(p->cmd, "align", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            goto fail;
        }

        align = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            goto fail;
        }
    }

    enj_elf_phdr* segment = enj_elf_new_phdr(p->elf, type, err);
    if (!segment)
    {
        enjp_error(err, "Unable to add new segment");
        goto fail;
    }

    ENJ_ELF_PHDR_SET(segment, p_type, type);
    ENJ_ELF_PHDR_SET(segment, p_flags, flags);
    ENJ_ELF_PHDR_SET(segment, p_offset, offset);
    ENJ_ELF_PHDR_SET(segment, p_vaddr, vaddr);
    ENJ_ELF_PHDR_SET(segment, p_paddr, paddr);
    ENJ_ELF_PHDR_SET(segment, p_filesz, filesz);
    ENJ_ELF_PHDR_SET(segment, p_memsz, memsz);
    ENJ_ELF_PHDR_SET(segment, p_align, align);

    if (enj_elf_update_sections(p->elf, err) < 0 ||
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
    "add",
    "Add program header",
    &enjp_phdr_add_help,
    &enjp_phdr_add_run
};

static __attribute__((constructor(201))) void _register()
{
    enj_error* err = 0;

    if (enjp_phdr_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register phdr command 'add'");
}
