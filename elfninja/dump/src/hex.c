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

#define _GNU_SOURCE

#include "elfninja/dump/hex.h"
#include "elfninja/core/malloc.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

enjd_hex_dumper* enjd_hex_dumper_create(enj_error** err)
{
    enjd_hex_dumper* hd = enj_malloc(sizeof(enjd_hex_dumper));
    if (!hd)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    hd->stride = 1;
    hd->grid = 16;
    hd->show_hex = 1;
    hd->show_ascii = 0;
    hd->unprintable_char = '.';
    hd->no_indices = 0;
    hd->base_address = 0;
    hd->from = -1;
    hd->to = -1;

    return hd;
}

void enjd_hex_dumper_delete(enjd_hex_dumper* hd)
{
    if (!hd)
        return;

    enj_free(hd);
}

int enjd_hex_dumper_run(enjd_hex_dumper* hd, void const* data, size_t length, int fd, enj_error** err)
{
    if (!hd || !data || fd <= 0)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (hd->stride < 1 || hd->stride > 8 || hd->grid < hd->stride ||
        (hd->from >= 0 && hd->from >= length) ||
        (hd->to >= 0 && (hd->to < hd->from || hd->to >= length)))
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    FILE* f = fdopen(dup(fd), "wb");
    if (!f)
    {
        enj_error_put_posix_errno(err, ENJ_ERR_IO, errno);
        return -1;
    }

    size_t start_pos = hd->from >= 0 ? hd->from : 0;
    size_t stop_len = hd->to >= 0 ? hd->to + 1 : length;

    for (size_t pos = start_pos; pos < stop_len; pos += hd->grid)
    {
        // Print index if allowed
        if (!hd->no_indices)
        {
            if (hd->base_address)
                fprintf(f, "%08lX: ", hd->base_address + pos);
            else
                fprintf(f, "%04lX: ", pos);
        }

        // Dump line contents as hex
        if (hd->show_hex)
        {
            for (size_t i = 0; i < hd->grid; i += hd->stride)
            {
                // Get field position and size
                size_t field_pos = pos + i;
                size_t field_len = hd->stride;

                // Stop if we're at the end
                if (field_pos < stop_len)
                {
                    // Adjust the size of the field to avoid reading past the end of the buffer
                    if (field_pos + field_len > stop_len)
                        field_len = stop_len - field_pos;

                    // Read the actual field contents
                    size_t field = 0;
                    memcpy(&field, data + field_pos, field_len);

                    // And display it
                    fprintf(f, "%0*lX%s", 2 * (int) hd->stride, field, i != hd->grid - 1 ? " " : "");
                }
                else
                {
                    if (hd->show_ascii)
                        fprintf(f, "%*s%s", 2 * (int) hd->stride, " ", i != hd->grid - 1 ? " " : "");
                    else
                        break;
                }
            }
        }

        // Dump ASCII
        if (hd->show_ascii)
        {
            if (hd->show_hex)
                fprintf(f, " | ");

            for (size_t i = 0; i < hd->grid && (pos + i) < stop_len; ++i)
            {
                char c = ((const char*) data)[pos + i];
                if (!isprint(c))
                    c = hd->unprintable_char;

                fprintf(f, "%c", c);
            }
        }

        fprintf(f, "\n");
    }

    fclose(f);
    return 0;
}
