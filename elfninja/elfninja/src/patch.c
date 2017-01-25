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

#include "tool.h"
#include "log.h"

#include "elfninja/core/core.h"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static const char* _help_msg =
"The 'patch' tool allows small modifications to ELF files. Patches are\n"
"processed in a sequential fashion, each patch consisting of a file offset\n"
"and a patch value. Values can be null-terminated strings, array of characters\n"
"or hex-specified byffers.\n"
"Offsets can either be provided as absolute file offsets, relative to sections\n"
"or computed using loading addresses.\n"
;

int enjp_patch_help(enji_cmdline* cmd)
{
    if (!cmd)
        return -1;

    printf("%s", _help_msg);

    return 0;
}

int enjp_patch_run(enji_cmdline* cmd)
{
    if (!cmd)
        return -1;

    enj_error* err = 0;

    // Get file name from command line
    enji_cmdline_argument* file = enji_cmdline_find_argument_by_index(cmd, 0, 0);
    if (!file)
        enjp_fatal(0, "No file specified. Try 'elfninja patch help'");

    // Rebase all options
    if (enji_cmdline_rebase_options(cmd, file, &err) < 0)
        enjp_fatal(&err, "Unable to rebase cmdline options");

    // Try to open the file
    int fd = open(file->name->string, O_RDWR);
    if (fd <= 0)
        enjp_fatal(0, "Unable to open '%s' for writing", file->name->string);

    // Create the ELF object
    enj_elf* elf = enj_elf_create_fd(fd, &err);
    if (!elf)
    {
        enjp_error(&err, "Unable to read file '%s' as ELF", file->name->string);
        close(fd);
        return -1;
    }

    // Check if an addressing mode was specified
    if (!file->next)
    {
        enjp_error(0, "No offset specified. Try 'elfninja patch help'");
        goto fail;
    }

    for (enji_cmdline_argument* offset = file->next; offset; offset = offset->next)
    {
        size_t file_offset = enji_parse_offset(elf, offset->name->string, &err);
        if (err || file_offset > elf->blob->buffer_size)
        {
            enjp_error(&err, "Invalid offset");
            goto fail;
        }

        char* bytes = 0;
        size_t length = 0;
        size_t count = 1;
        int free_bytes = 0;

        enji_cmdline_option* opt = 0;
        if ((opt = enji_cmdline_find_option(cmd, "hex", ENJI_CMDLINE_TOOL, offset, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            bytes = enji_parse_hex(opt->value, &length, &err);
            if (!bytes)
            {
                enjp_error(&err, "Invalid data");
                goto fail;
            }
            free_bytes = 1;
        }
        else if ((opt = enji_cmdline_find_option(cmd, "chars", ENJI_CMDLINE_TOOL, offset, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            bytes = opt->value;
            length = strlen(opt->value);

            if (!length)
                enjp_warning(0, "Writing empty string");
        }
        else if ((opt = enji_cmdline_find_option(cmd, "string", ENJI_CMDLINE_TOOL, offset, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            bytes = opt->value;
            length = strlen(opt->value) + 1;
        }
        else
        {
            enjp_error(0, "No data specified. Try 'elfninja help patch'");
            goto fail;
        }

        if ((opt = enji_cmdline_find_option(cmd, "count", ENJI_CMDLINE_TOOL, offset, 0)))
        {
            if (!opt->value)
            {
                enjp_error(0, "Option '%s' expects a value", opt->name->string);
                goto fail;
            }

            count = enji_parse_number(opt->value, &err);
            if (err || !count)
            {
                enjp_error(&err, "Invalid value for option '%s'", opt->name->string);
                goto fail;
            }
        }

        size_t effective_length = count * length;

        if (file_offset + effective_length > elf->blob->buffer_size)
        {
            enjp_warning(0, "Data is too long, ignoring %ld bytes", (file_offset + length) - elf->blob->buffer_size);
            effective_length = elf->blob->buffer_size - file_offset;

            count = (effective_length + length - 1) / length;
        }

        enjp_message("Patching %ld bytes at file offset 0x%08lX", effective_length, file_offset);

        for (size_t i = 0; i < count; ++i)
        {
            size_t off = file_offset + i * length;

            if ((off + length) > elf->blob->buffer_size)
                length = elf->blob->buffer_size - off;

            memcpy(elf->blob->buffer + off, bytes, length);
        }

        if (free_bytes)
            enj_free(bytes);
    }

    off_t off = lseek(fd, 0, SEEK_SET);
    if (off < 0)
    {
        enj_error_put_posix_errno(&err, ENJ_ERR_IO, errno);
        enjp_error(&err, "Unable to write back changes to file");
        goto fail;
    }

    size_t count = write(fd, elf->blob->buffer, elf->blob->buffer_size);
    if (count != elf->blob->buffer_size)
    {
        if (count < 0)
            enj_error_put_posix_errno(&err, ENJ_ERR_IO, count);

        enjp_error(&err, "Unable to write back changes to file");
        goto fail;
    }

    enj_elf_delete(elf);
    close(fd);
    return 0;

fail:
    enj_elf_delete(elf);
    close(fd);
    return -1;
}

static enjp_tool _this_tool =
{
    "patch",
    "Patch file content",
    &enjp_patch_help,
    &enjp_patch_run
};

static __attribute__((constructor(108))) void _register()
{
    enj_error* err = 0;

    if (enjp_tool_register(&_this_tool, &err) < 0)
        enjp_fatal(&err, "Unable to register tool 'patch'");
}
