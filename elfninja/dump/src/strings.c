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

#include "elfninja/dump/strings.h"
#include "elfninja/core/malloc.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

enjd_strings_dumper* enjd_strings_dumper_create(enj_error** err)
{
    enjd_strings_dumper* sd = enj_malloc(sizeof(enjd_strings_dumper));
    if (!sd)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    sd->no_indices = 0;
    sd->use_quotes = 0;
    sd->show_empty = 0;
    sd->unprintable_char = '.';

    return sd;
}

void enjd_strings_dumper_delete(enjd_strings_dumper* sd)
{
    if (!sd)
        return;

    enj_free(sd);
}

int enjd_strings_dumper_run(enjd_strings_dumper* sd, void const* data, size_t length, int fd, enj_error** err)
{
    if (!sd || !data || fd <= 0)
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

    const char* str_data = (const char*) data;

    for (size_t index = 0; index < length;)
    {
        const char* str = str_data + index;
        size_t string_len = 0;
        for (size_t i = index; str_data[i] && i < length; ++i)
            ++string_len;

        if (!sd->show_empty && !string_len)
        {
            ++index;
            continue;
        }

        if (!sd->no_indices)
            fprintf(f, "%04lX: ", index);

        if (sd->use_quotes)
            fprintf(f, "\"");

        for (size_t i = 0; i < string_len; ++i)
        {
            char c = str[i];
            fprintf(f, "%c", isprint(c) ? c : sd->unprintable_char);
        }

        index += string_len + 1;

        fprintf(f, "%s\n", sd->use_quotes ? "\"" : "");
    }

    fclose(f);
    return 0;
}
