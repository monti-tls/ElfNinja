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

#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

static struct
{
    int fd[2];
    int channel_mask;
    int format_mask;
} _conf =
{
    { 1, 2 },
    ENJP_LOG_MESSAGE | ENJP_LOG_WARNING | ENJP_LOG_ERROR | ENJP_LOG_FATAL,
    0 // ENJP_LOG_CHANNEL | ENJP_LOG_LOC_FULL
};

static void _location(FILE* f, const char* function, size_t line, const char* file)
{
    if (_conf.format_mask & ENJP_LOG_LOC_FULL)
    {
        int add_space = 0;

        if (_conf.format_mask & ENJP_LOG_LOC_FUNC)
        {
            fprintf(f, "in '%s'", function);
            add_space = 1;
        }

        fprintf(f, "%sat %s:%ld", add_space ? " " : "", file, line);
    }
}

static void _trace_error(FILE* f, const char* prefix, enj_error** err, size_t indent)
{
    if (!err || !*err)
        return;

    // Print prefix
    if (prefix)
        fprintf(f, "%s", prefix);

    // Align to indentation level
    for (size_t i = 0; i < indent; ++i)
        fprintf(f, "   ");

    // Get error description
    const char* name = enj_error_string((*err)->code);
    const char* desc = enj_error_description((*err)->code);

    if (name)
        fprintf(f, "[%s] ", name);
    else
        fprintf(f, "[??? (%d)] ", (*err)->code);

    if ((*err)->function)
    {
        if ((_conf.format_mask & ENJP_LOG_LOC_FUNC) ||
            (_conf.format_mask & ENJP_LOG_LOC_FULL))
        {
            fprintf(f, "(");
            _location(f, (*err)->function, (*err)->line, (*err)->file);
            fprintf(f, ") ");
        }
    }

    if (desc)
        fprintf(f, "%s\n", desc);
    else
        fprintf(f, "No description available.\n");

    if ((*err)->wrap)
    {
        _trace_error(f, prefix, &(*err)->wrap, indent + 1);
    }
    else if ((*err)->posix_errno)
    {
        // Print prefix
        if (prefix)
            fprintf(f, "%s", prefix);

        // Align to indentation level
        for (size_t i = 0; i < indent + 1; ++i)
            fprintf(f, "   ");

        const char* desc = strerror((*err)->posix_errno);
        fprintf(f, "[Posix errno %d] : %s\n", (*err)->posix_errno, desc ? desc : "No description available.\n");
    }

    enj_error_delete(*err);
    *err = 0;
}

void enjp_log(int level, int fd, const char* function, size_t line, const char* file, enj_error** err, const char* fmt, ...)
{
    FILE* f = fdopen(dup(_conf.fd[fd]), "wb");
    if (!f)
    {
        fprintf(stderr, "Unable to open log file descriptor\n");
        exit(-1);
    }

    if (level & _conf.channel_mask)
    {
        const char* level_string;
        switch (level)
        {
            case ENJP_LOG_TRACE:   level_string = "ENJP_LOG_TRACE"; break;
            case ENJP_LOG_MESSAGE: level_string = "ENJP_LOG_MESSAGE"; break;
            case ENJP_LOG_WARNING: level_string = "ENJP_LOG_WARNING"; break;
            case ENJP_LOG_ERROR:   level_string = "ENJP_LOG_ERROR"; break;
            case ENJP_LOG_FATAL:   level_string = "ENJP_LOG_FATAL"; break;
            default: level_string = "????";
        }

        char prefix_buffer[128];
        const char* prefix = "";

        // Setup log level prefix
        if (_conf.format_mask & ENJP_LOG_CHANNEL)
        {
            size_t len = snprintf(&prefix_buffer[0], sizeof(prefix_buffer), "[%-16s] ", level_string);
            prefix_buffer[len] = '\0';
            fprintf(f, "%s", &prefix_buffer[0]);

            prefix = &prefix_buffer[0];
        }

        // Print location if not masked
        if ((_conf.format_mask & ENJP_LOG_LOC_FUNC) ||
            (_conf.format_mask & ENJP_LOG_LOC_FULL))
        {
            fprintf(f, "(");
            _location(f, function, line, file);
            fprintf(f, ") ");
        }

        // Get actual log message
        va_list ap;
        va_start(ap, fmt);
        char buffer[512];
        size_t len = vsnprintf(&buffer[0], sizeof(buffer), fmt, ap);
        buffer[len] = '\0';
        va_end(ap);

        fprintf(f, "%s\n", &buffer[0]);

        // Also print out error trace
        if (err)
            _trace_error(f, prefix, err, 1);
    }

    if (level == ENJP_LOG_FATAL)
        exit(-1);
}
