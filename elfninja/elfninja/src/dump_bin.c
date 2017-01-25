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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int enjp_dump_bin_help()
{
    return 0;
}

int enjp_dump_bin_run(enjp_dump_tool* d, enji_cmdline_argument* arg, enj_error** err)
{
    const char* pattern = arg->value;

    int file_fd = 0;
    ssize_t from = -1;
    ssize_t to = -1;

    // Process options for this command
    enji_cmdline_option* opt;
    if ((opt = enji_cmdline_find_option(d->cmd, "file", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'file' expects a value");
            goto fail;
        }

        file_fd = open(opt->value, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (file_fd <= 0)
        {
            if (file_fd < 0)
                enj_error_put_posix_errno(err, ENJ_ERR_IO, errno);

            enjp_error(err, "Unable to open file '%s'", opt->value);
            goto fail;
        }
    }

    if (file_fd == 0)
    {
        file_fd = dup(1);
    }

    if ((opt = enji_cmdline_find_option(d->cmd, "from", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'from' expects a value");
            goto fail;
        }

        if (pattern)
            from = enji_parse_number(opt->value, err);
        else
            from = enji_parse_offset(d->elf, opt->value, err);

        if (*err)
        {
            enjp_error(err, "Invalid value for option 'from'");
            goto fail;
        }
    }

    if ((opt = enji_cmdline_find_option(d->cmd, "to", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option 'to' expects a value");
            goto fail;
        }

        if (pattern)
            to = enji_parse_number(opt->value, err);
        else
            to = enji_parse_offset(d->elf, opt->value, err);

        if (*err)
        {
            enjp_error(err, "Invalid value for option 'to'");
            goto fail;
        }
    }

    void* buffer = 0;
    size_t buffer_size = 0;

    if (!pattern)
    {
        buffer = d->elf->blob->buffer;
        buffer_size = d->elf->blob->buffer_size;

        if (to >= 0)
        {
            if (to >= buffer_size)
                enj_error_put(err, ENJ_ERR_BAD_OFFSET);
            else
                buffer_size = to + 1;
        }

        if (from >= 0)
        {
            if (from >= buffer_size || (to >= 0 && from >= to))
                enj_error_put(err, ENJ_ERR_BAD_OFFSET);
            else
            {
                buffer += from;
                buffer_size -= from;
            }
        }

        if (*err)
        {
            enjp_error(err, "Invalid parameters");
            goto fail;
        }

        enjp_message("Dumping %ld bytes from file", buffer_size);
        size_t count = write(file_fd, buffer, buffer_size);
        if (count != buffer_size)
        {
            if (count < 0)
                enj_error_put_posix_errno(err, ENJ_ERR_IO, count);

            enjp_error(err, "Failed to write to file");
            goto fail;
        }
    }
    else
    {
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
                        goto fail;
                    }

                    continue;
                }
            }

            // If the section does not have any content, it may be malformed
            if (!section->data)
            {
                enjp_warning(0, "Section #%ld (%s) has no content, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

                // And ignore this section
                continue;
            }

            // Ignore empty sections
            if (!section->data->length)
            {
                if (pattern)
                    enjp_warning(0, "Section #%ld (%s) is empty, ignoring", section->index, section->cached_name ? section->cached_name->string : "");

                continue;
            }

            buffer =  section->elf->blob->buffer + section->data->start->pos;
            buffer_size = section->data->length;

            if (to >= 0)
            {
                if (to >= buffer_size)
                    enj_error_put(err, ENJ_ERR_BAD_OFFSET);
                else
                    buffer_size = to + 1;
            }

            if (from >= 0)
            {
                if (from >= buffer_size || (to >= 0 && from >= to))
                    enj_error_put(err, ENJ_ERR_BAD_OFFSET);
                else
                {
                    buffer += from;
                    buffer_size -= from;
                }
            }

            if (*err)
            {
                enjp_error(err, "Invalid parameters");
                goto fail;
            }

            enjp_message("Dumping %ld bytes from section #%ld (%s)", buffer_size, section->index, section->cached_name ? section->cached_name->string : "");

            size_t count = write(file_fd, buffer, buffer_size);
            if (count != buffer_size)
            {
                if (count < 0)
                    enj_error_put_posix_errno(err, ENJ_ERR_IO, count);

                enjp_error(err, "write failed");
                goto fail;
            }
        }
    }


    close(file_fd);
    return 0;

fail:
    close(file_fd);
    return -1;
}

static enjp_dump_command _this_cmd =
{
    "bin",
    "Extract raw section content",
    &enjp_dump_bin_help,
    &enjp_dump_bin_run
};

static __attribute__((constructor(206))) void _register()
{
    enj_error* err = 0;

    if (enjp_dump_register_command(&_this_cmd, &err) < 0)
        enjp_fatal(&err, "Unable to register dump command 'bin'");
}
