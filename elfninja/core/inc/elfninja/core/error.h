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

#ifndef __ELFNINJA_CORE_ERROR_H__
#define __ELFNINJA_CORE_ERROR_H__

#include <stddef.h>

enum
{
    ENJ_ERR_OK = 0,

    #define DEF_ERRNO(no, desc) ENJ_ERR_ ## no,
    #include "elfninja/core/errno.def"
};

typedef struct enj_error
{
    int code;
    int posix_errno;
    struct enj_error* wrap;

    const char* function;
    size_t line;
    const char* file;
} enj_error;

enj_error* enj_error_create(int code);
enj_error* enj_error_create_posix_errno(int code, int posix_errno);
enj_error* enj_error_create_wrap(int code, enj_error* wrap);
void enj_error_delete(enj_error* err);

void enj_error__put(enj_error** err, int code, const char* function, size_t line, const char* file);
void enj_error__put_posix_errno(enj_error** err, int code, int posix_errno, const char* function, size_t line, const char* file);
void enj_error__wrap(enj_error** err, int code, enj_error* wrap, const char* function, size_t line, const char* file);

#define enj_error_put(err, code) \
    enj_error__put((err), (code), __FUNCTION__, __LINE__, __FILE__)
#define enj_error_put_posix_errno(err, code, posix_errno) \
    enj_error__put_posix_errno((err), (code), (posix_errno), __FUNCTION__, __LINE__, __FILE__)
#define enj_error_wrap(err, code, wrap) \
    enj_error__wrap((err), (code), (wrap), __FUNCTION__, __LINE__, __FILE__)

const char* enj_error_string(int code);
const char* enj_error_description(int code);

#endif // __ELFNINJA_CORE_ERROR_H__
