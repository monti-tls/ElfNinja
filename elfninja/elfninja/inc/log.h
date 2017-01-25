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

#ifndef __ELFNINJA_LOG_H__
#define __ELFNINJA_LOG_H__

#include "elfninja/core/error.h"

#define ENJP_LOG_TRACE   (0x01 << 0)
#define ENJP_LOG_MESSAGE (0x01 << 1)
#define ENJP_LOG_WARNING (0x01 << 2)
#define ENJP_LOG_ERROR   (0x01 << 3)
#define ENJP_LOG_FATAL   (0x01 << 4)

#define ENJP_LOG_LOC_FUNC (0x01 << 0)
#define ENJP_LOG_LOC_FULL (0x01 << 1)
#define ENJP_LOG_CHANNEL  (0x01 << 2)

void enjp_log(int level, int fd, const char* function, size_t line, const char* file, enj_error** err, const char* fmt, ...) __attribute__ ((format(printf, 7, 8)));

#define enjp_trace(fmt, ...) \
    enjp_log(ENJP_LOG_TRACE,   0, __FUNCTION__, __LINE__, __FILE__, 0, (fmt), ## __VA_ARGS__)
#define enjp_message(fmt, ...) \
    enjp_log(ENJP_LOG_MESSAGE, 0, __FUNCTION__, __LINE__, __FILE__, 0, (fmt), ## __VA_ARGS__)
#define enjp_warning(err, fmt, ...) \
    enjp_log(ENJP_LOG_WARNING, 1, __FUNCTION__, __LINE__, __FILE__, (err), (fmt), ## __VA_ARGS__)
#define enjp_error(err, fmt, ...) \
    enjp_log(ENJP_LOG_ERROR,   1, __FUNCTION__, __LINE__, __FILE__, (err), (fmt), ## __VA_ARGS__)
#define enjp_fatal(err, fmt, ...) \
    enjp_log(ENJP_LOG_FATAL,   1, __FUNCTION__, __LINE__, __FILE__, (err), (fmt), ## __VA_ARGS__)

#endif // __ELFNINJA_LOG_H__
