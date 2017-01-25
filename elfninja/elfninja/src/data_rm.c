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

#include "data.h"
#include "tool.h"
#include "log.h"

#include "elfninja/core/core.h"
#include "elfninja/input/input.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fnmatch.h>
#include <errno.h>

int enjp_data_rm_help()
{
    return 0;
}

int enjp_data_rm_run(enjp_data_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    if (!d || !d->cmd || !arg || !d->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    size_t file_offset = 0;
    size_t length = 0;

    if (!arg->value)
    {
        enjp_error(0, "No offset specified. Try 'elfninja help data rm'");
        return -1;
    }

    file_offset = enji_parse_offset(d->elf, arg->value, err);
    if (*err)
    {
        enjp_error(err, "Invalid start offset");
        return -1;
    }

    if (*err || file_offset > d->elf->blob->buffer_size)
    {
        enjp_error(err, "Invalid start offset");
        return -1;
    }

    enji_cmdline_option* opt = 0;
    if ((opt = enji_cmdline_find_option(d->cmd, "size", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            return -1;
        }

        length = enji_parse_number(opt->value, err);
        if (*err)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            return -1;
        }
    }

    if (file_offset + length > d->elf->blob->buffer_size)
    {
        enjp_error(0, "Removing past the end of the file");
        return -1;
    }

    enjp_message("Removing %ld bytes from file offset 0x%08lX", length, file_offset);

    if (enj_blob_remove(d->elf->blob, file_offset, length, err) < 0)
    {
        enjp_error(err, "Unable to remove data from blob");
        return -1;
    }

    if (!enji_cmdline_find_option(d->cmd, "no-update", ENJI_CMDLINE_TOOL, arg, 0))
    {
        if (enj_elf_push(d->elf, err) < 0)
        {
            enjp_error(err, "Unable to push changes to ELF");
            return -1;
        }
    }

    return 0;
}

static enjp_data_command _this_cmd =
{
    "rm",
    "Remove data from the ELF file",
    &enjp_data_rm_help,
    &enjp_data_rm_run
};

static __attribute__((constructor(203))) void _register()
{
    enj_error* err = 0;

    if (enjp_data_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register data command 'rm'");
}
