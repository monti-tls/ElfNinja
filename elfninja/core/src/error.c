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

#include "elfninja/core/error.h"
#include "elfninja/core/malloc.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static enj_error int_error =
{
    ENJ_ERR_INTERNAL,
    0
};

enj_error* enj_error_create(int code)
{
    enj_error* err = enj_malloc(sizeof(enj_error));
    if (!err)
        return &int_error;

    err->code = code;
    err->posix_errno = 0;
    err->wrap = 0;
    err->function = 0;
    err->line = 0;
    err->file = 0;

    return err;
}

enj_error* enj_error_create_posix_errno(int code, int posix_errno)
{
    enj_error* err = enj_malloc(sizeof(enj_error));
    if (!err)
        return &int_error;

    err->code = code;
    err->posix_errno = posix_errno;
    err->wrap = 0;
    err->function = 0;
    err->line = 0;
    err->file = 0;

    return err;
}

enj_error* enj_error_create_wrap(int code, enj_error* wrap)
{
    enj_error* err = enj_malloc(sizeof(enj_error));
    if (!err)
        return &int_error;

    err->code = code;
    err->posix_errno = 0;
    err->wrap = wrap;
    err->function = 0;
    err->line = 0;
    err->file = 0;

    return err;
}

/*enj_error* enj_error_create_message(int code, const char* fmt, ...)
{
    enj_error* err = enj_malloc(sizeof(enj_error));
    if (!err)
        return &int_error;

    err->code = code;
    err->message = 0;

    if (fmt)
    {
        va_list ap;
        va_start(ap, fmt);
        char buf[512];
        size_t len = vsnprintf(&buf[0], sizeof(buf), fmt, ap);
        buf[len] = '\0';

        err->message = enj_malloc(len + 1);
        if (!err->message)
        {
            enj_free(err);
            return &int_error;
        }

        strcpy(err->message, &buf[0]);
    }

    return err;
}*/

void enj_error_delete(enj_error* err)
{
    if (!err || err == &int_error)
        return;

    enj_free(err->wrap);
    enj_free(err);
}

void enj_error__put(enj_error** err, int code, const char* function, size_t line, const char* file)
{
    if (!err)
        return;

    if (*err)
        enj_error_delete(*err);

    *err = enj_error_create(code);
    if (*err)
    {
        (*err)->function = function;
        (*err)->line = line;
        (*err)->file = file;
    }
}

void enj_error__put_posix_errno(enj_error** err, int code, int posix_errno, const char* function, size_t line, const char* file)
{
    if (!err)
        return;

    if (*err)
        enj_error_delete(*err);

    *err = enj_error_create_posix_errno(code, posix_errno);
    if (*err)
    {
        (*err)->function = function;
        (*err)->line = line;
        (*err)->file = file;
    }
}

void enj_error__wrap(enj_error** err, int code, enj_error* wrap, const char* function, size_t line, const char* file)
{
    if (!err)
        return;

    if (*err && *err != wrap)
        enj_error_delete(*err);

    *err = enj_error_create_wrap(code, wrap);
    if (*err)
    {
        (*err)->function = function;
        (*err)->line = line;
        (*err)->file = file;
    }
}

const char* enj_error_string(int code)
{
    static struct
    {
        int code;
        const char* value;
    } strings[] =
    {
        #define DEF_ERRNO(no, desc) { (ENJ_ERR_ ## no), #no },
        #include "elfninja/core/errno.def"
    };

    for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i)
        if (strings[i].code == code)
            return strings[i].value;

    return 0;
}

const char* enj_error_description(int code)
{
    static struct
    {
        int code;
        const char* value;
    } strings[] =
    {
        #define DEF_ERRNO(no, desc) { (ENJ_ERR_ ## no), desc },
        #include "elfninja/core/errno.def"
    };

    for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i)
        if (strings[i].code == code)
            return strings[i].value;

    return 0;
}
