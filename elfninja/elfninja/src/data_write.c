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

int enjp_data_write_help()
{
    return 0;
}

int enjp_data_write_run(enjp_data_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    if (!d || !d->cmd || !arg || !d->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (enjp_data_read_input(d, arg, "write", err) < 0)
    {
        enjp_error(err, "Bad input parameters");
        return -1;
    }

    enjp_message("Writing %ld bytes at file offset 0x%08lX", d->effective_length, d->file_offset);

    for (size_t i = 0; i < d->count; ++i)
    {
        size_t off = d->file_offset + i * d->length;

        size_t length = d->length;
        if ((off + length) > d->elf->blob->buffer_size)
            length = d->elf->blob->buffer_size - off;

        if (enj_blob_write(d->elf->blob, off, d->bytes, length, err) < 0)
        {
            enjp_error(err, "Unable to write data to blob");
            return -1;
        }
    }

    return 0;
}

static enjp_data_command _this_cmd =
{
    "write",
    "Write data to the ELF file",
    &enjp_data_write_help,
    &enjp_data_write_run
};

static __attribute__((constructor(201))) void _register()
{
    enj_error* err = 0;

    if (enjp_data_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register data command 'write'");
}
