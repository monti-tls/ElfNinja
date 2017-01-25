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

#include "elfninja/dump/formatter.h"
#include "elfninja/core/malloc.h"

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>

enjd_formatter* enjd_formatter_create(enj_error** err)
{
    enjd_formatter* fmt = enj_malloc(sizeof(enjd_formatter));
    if (!fmt)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    fmt->escape0 = ENJ_DUMP_FORMAT_ESCAPE_CHAR;
    fmt->escape1 = ENJ_DUMP_FORMAT_ESCAPE_CHAR;
    fmt->modifier = ENJ_DUMP_FORMAT_MODIFIER_CHAR;
    fmt->elems = 0;
    fmt->last_elem = 0;

    return fmt;
}

void enjd_formatter_delete(enjd_formatter* fmt)
{
    if (!fmt)
        return;

    for (enjd_formatter_elem* elem = fmt->elems; elem; )
    {
        enjd_formatter_elem* next = elem->next;
        enj_fstring_delete(elem->name);
        enj_free(elem);
        elem = next;
    }

    enj_free(fmt);
}

enjd_formatter_elem* enjd_formatter_new_elem(enjd_formatter* fmt, const char* name, enjd_formatter_handler_t handler, void* arg0, enj_error** err)
{
    if (!fmt || !name || !handler)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enjd_formatter_elem* elem = enjd_formatter_find_elem(fmt, name, err);
    if (elem)
    {
        enj_error_put(err, ENJ_ERR_EXISTS);
        return 0;
    }
    else if (*err)
        return 0;

    elem = enj_malloc(sizeof(enjd_formatter_elem));
    if (!elem)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    elem->name = enj_fstring_create(name, err);
    if (!elem->name)
    {
        enj_free(elem);
        return 0;
    }

    elem->fmt = fmt;
    elem->handler = handler;
    elem->arg0 = arg0;
    elem->prev = fmt->last_elem;
    elem->next = 0;

    if (elem->prev)
        elem->prev->next = elem;
    else
        fmt->elems = elem;

    fmt->last_elem = elem;

    return elem;
}

int enjd_formatter_remove_elem(enjd_formatter* fmt, enjd_formatter_elem* elem, enj_error** err)
{
    if (!fmt || !elem || elem->fmt != fmt)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (elem->prev)
        elem->prev->next = elem->next;
    else
        fmt->elems = elem->next;

    if (elem->next)
        elem->next->prev = elem->prev;
    else
        fmt->last_elem = elem->prev;

    enj_fstring_delete(elem->name);
    enj_free(elem);

    return 0;
}

enjd_formatter_elem* enjd_formatter_find_elem(enjd_formatter* fmt, const char* name, enj_error** err)
{
    if (!fmt || !name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_fstring_hash_t hash = enj_fstring_hash(name);

    for (enjd_formatter_elem* elem = fmt->elems; elem; elem = elem->next)
    {
        if (!elem->name || elem->name->hash != hash)
            continue;

        if (!strncmp(elem->name->string, name, elem->name->length))
            return elem;
    }

    return 0;
}

int enjd_formatter_run(enjd_formatter* fmt, const char* string, void* arg1, int fd, enj_error** err)
{
    if (!fmt || !string || fd <= 0)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (const char* p = string; p && *p; )
    {
        if (*p != ENJ_DUMP_FORMAT_ESCAPE_CHAR)
        {
            int count;

        write_single_char:
            count = write(fd, p++, 1);

            if (count < 1)
            {
                if (count < 0)
                    enj_error_put_posix_errno(err, ENJ_ERR_IO, count);
                else
                    enj_error_put(err, ENJ_ERR_IO);
                return -1;
            }

            continue;
        }
        else
        {
            ++p;

            // If the escape was itself escaped, just print it
            if (*p == fmt->escape0)
                goto write_single_char;

            int mod_width = 0;
            char mod_pad = ' ';

            // Check for modifiers (width and pad character)
            if (*p == fmt->modifier)
            {
                ++p;

                if (*p)
                {
                    if (*p != '-' && !isdigit(*p))
                    {
                        enj_error_put(err, ENJ_ERR_BAD_FMT);
                        return -1;
                    }

                    // Get evnentual sign (for right-aligned field)
                    int sign = 1;
                    if (*p == '-')
                    {
                        sign = -1;
                        ++p;
                    }

                    // Get field width
                    while (isdigit(*p) && *p)
                        mod_width = mod_width * 10 + (*p++ - '0');

                    // Apply sign
                    mod_width = mod_width * sign;

                    // Get eventual pad character
                    if (*p == fmt->modifier && *p)
                    {
                        if (*++p)
                            mod_pad = *p++;
                    }
                }

                // If we reached the end already, the format was invalid
                if (!*p)
                {
                    enj_error_put(err, ENJ_ERR_BAD_FMT);
                    return -1;
                }
            }

            // Get the length of the element
            const char* s = p;
            size_t elem_len = 0;
            while (*s != fmt->escape1 && *s)
            {
                ++s;
                ++elem_len;
            }

            // If we reached the end already, the format was invalid
            if (*s != fmt->escape1)
            {
                enj_error_put(err, ENJ_ERR_BAD_FMT);
                return -1;
            }

            // Get a clean version of the name, null-terminated
            char elem_name[elem_len+1];
            memcpy(&elem_name[0], p, elem_len);
            elem_name[elem_len] = '\0';

            // Find the associated element
            enjd_formatter_elem* elem = enjd_formatter_find_elem(fmt, &elem_name[0], err);
            if (!elem)
            {
                enj_error_wrap(err, ENJ_ERR_NEXISTS, *err);
                enj_error_wrap(err, ENJ_ERR_BAD_FMT, *err);
                return -1;
            }

            // Invoke the element's handler
            if ((*elem->handler)(elem->arg0, arg1, fd, mod_width, mod_pad, err) < 0)
                return -1;

            p = ++s;
        }
    }

    return 0;
}

void enjd_padded_fprintf(FILE* f, int width, char pad, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    // Craft actual string
    char buffer[512];
    size_t len = vsnprintf(&buffer[0], sizeof(buffer), fmt, ap);
    buffer[len] = '\0';

    // Is the field right-aligned ?
    int right = 0;
    if (width < 0)
    {
        right = 1;
        width = -width;
    }

    // If no width was requested, or if the string exceeds the width
    //   anyway, just print it
    if (!width || len >= width)
    {
        fprintf(f, "%s", &buffer[0]);
    }
    // Otherwise,
    else
    {
        // Get the needed pad length
        int padlen = width - len;

        // Pad for right-aligned fields
        for (int i = 0; right && i < padlen; ++i)
            fprintf(f, "%c", pad);

        // Actual field contents
        fprintf(f, "%s", &buffer[0]);

        // Pad for left-aligned fields
        for (int i = 0; !right && i < padlen; ++i)
            fprintf(f, "%c", pad);
    }

    va_end(ap);
}
