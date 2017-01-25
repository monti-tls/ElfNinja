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

#include "elfninja/input/cmdline.h"

#include "elfninja/core/error.h"
#include "elfninja/core/malloc.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static enji_cmdline_option* _parse_option(enji_cmdline* cmd, const char* str, int is_global, enj_error** err)
{
    if (!cmd || !str)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enji_cmdline_option* opt = enj_malloc(sizeof(enji_cmdline_option));
    if (!opt)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    opt->is_global = is_global;
    opt->base_arg = cmd->last_argument;

    while (*str == '-')
        ++str;

    const char* name = str;
    size_t name_length = 0;
    for (; *str && *str != '='; ++str)
        ++name_length;

    opt->name = enj_fstring_create_n(name, name_length, err);
    if (!opt->name)
    {
        enj_free(opt);
        return 0;
    }

    if (*str == '=')
    {
        ++str;
        size_t value_length = 0;
        for (const char* p = str; *p; ++p)
            ++value_length;

        opt->value = enj_malloc(value_length + 1);
        if (!opt->value)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            enj_fstring_delete(opt->name);
            enj_free(opt);
            return 0;
        }

        memcpy(opt->value, str, value_length);
        opt->value[value_length] = '\0';
    }

    if (is_global)
    {
        enji_cmdline_option* other = enji_cmdline_find_option(cmd, opt->name->string, ENJI_CMDLINE_GLOBAL, 0, 0);
        if (other)
        {
            enj_fstring_delete(other->name);
            enj_free(other->value);

            other->is_global = opt->is_global;
            other->name = opt->name;
            other->value = opt->value;

            enj_free(opt);
            return 0;
        }
    }

    opt->prev = cmd->last_option;
    opt->next = 0;
    if (opt->prev)
        opt->prev->next = opt;
    else
        cmd->options = opt;
    cmd->last_option = opt;

    return 0;
}

static int _parse_argument(enji_cmdline* cmd, const char* str, enj_error** err)
{
    if (!cmd || !str)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enji_cmdline_argument* arg = enj_malloc(sizeof(enji_cmdline_argument));
    if (!arg)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    while (*str == '-')
        ++str;

    const char* name = str;
    size_t name_length = 0;
    for (; *str && *str != '='; ++str)
        ++name_length;

    arg->name = enj_fstring_create_n(name, name_length, err);
    if (!arg->name)
    {
        enj_free(arg);
        return 0;
    }

    if (*str == '=')
    {
        ++str;
        size_t value_length = 0;
        for (const char* p = str; *p; ++p)
            ++value_length;

        arg->value = enj_malloc(value_length + 1);
        if (!arg->value)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            enj_fstring_delete(arg->name);
            enj_free(arg);
            return 0;
        }

        memcpy(arg->value, str, value_length);
        arg->value[value_length] = '\0';
    }

    arg->index = cmd->argument_count++;

    arg->prev = cmd->last_argument;
    arg->next = 0;
    if (arg->prev)
        arg->prev->next = arg;
    else
        cmd->arguments = arg;
    cmd->last_argument = arg;

    return 0;
}

enji_cmdline* enji_cmdline_create(int argc, char** argv, enj_error** err)
{
    if (argc < 1 || !argv)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enji_cmdline* cmd = enj_malloc(sizeof(enji_cmdline));
    if (!cmd)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        enj_free(cmd);
        return 0;
    }

    cmd->argc = argc;
    cmd->argv = argv;

    for (size_t i = 1; i < argc; ++i)
    {
        const char* str = argv[i];

        if (*str == '-')
        {
            if (_parse_option(cmd, str, !cmd->tool, err) < 0)
            {
                enji_cmdline_delete(cmd);
                return 0;
            }
        }
        else
        {
            if (!cmd->tool)
            {
                size_t length = strlen(str);
                cmd->tool = enj_malloc(length + 1);
                if (!cmd->tool)
                {
                    enj_error_put(err, ENJ_ERR_MALLOC);
                    enji_cmdline_delete(cmd);
                    return 0;
                }
                memcpy(cmd->tool, str, length);
                cmd->tool[length] = '\0';
            }
            else
            {
                if (_parse_argument(cmd, str, err) < 0)
                {
                    enji_cmdline_delete(cmd);
                    return 0;
                }
            }
        }
    }

    return cmd;
}

void enji_cmdline_delete(enji_cmdline* cmd)
{
    if (!cmd)
        return;

    for (enji_cmdline_argument* arg = cmd->arguments; arg; )
    {
        enji_cmdline_argument* next = arg->next;
        enj_free(arg->value);
        enj_fstring_delete(arg->name);
        enj_free(arg);
        arg = next;
    }

    for (enji_cmdline_option* opt = cmd->options; opt; )
    {
        enji_cmdline_option* next = opt->next;
        enj_free(opt->value);
        enj_fstring_delete(opt->name);
        enj_free(opt);
        opt = next;
    }

    enj_free(cmd->tool);
    enj_free(cmd);
}

int enji_cmdline_rebase_options(enji_cmdline* cmd, enji_cmdline_argument* base, enj_error** err)
{
    if (!cmd || !base)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    int _is_successor(enji_cmdline_argument* arg, enji_cmdline_argument* of)
    {
        if (!arg || !of)
            return 0;

        if (arg == of)
            return 1;

        return _is_successor(arg->prev, of);
    }

    for (enji_cmdline_option* opt = cmd->options; opt; opt = opt->next)
    {
        if (!opt->base_arg)
            continue;

        if (_is_successor(base, opt->base_arg))
        {
            opt->base_arg = 0;
        }
    }

    return 0;
}

enji_cmdline_option* enji_cmdline_find_option(enji_cmdline* cmd, const char* name, int flags, enji_cmdline_argument* base_arg, enj_error** err)
{
    if (!cmd || !name || !flags)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_fstring_hash_t hash = enj_fstring_hash(name);

    for (enji_cmdline_option* opt = cmd->options; opt; opt = opt->next)
    {
        if (hash != opt->name->hash)
            continue;

        if (!strncmp(name, opt->name->string, opt->name->length))
        {
            if (opt->is_global && (flags & ENJI_CMDLINE_GLOBAL))
                return opt;

            if (!opt->is_global && (flags & ENJI_CMDLINE_TOOL))
            {
                if (!opt->base_arg && (flags & ENJI_CMDLINE_ORPHAN))
                    return opt;

                if (!base_arg || opt->base_arg == base_arg)
                    return opt;
            }
        }
    }

    return 0;
}

enji_cmdline_argument* enji_cmdline_find_argument_by_index(enji_cmdline* cmd, size_t index, enj_error** err)
{
    if (!cmd)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    for (enji_cmdline_argument* arg = cmd->arguments; arg; arg = arg->next)
    {
        if (arg->index == index)
            return arg;
    }

    return 0;
}

enji_cmdline_argument* enji_cmdline_find_argument_by_name(enji_cmdline* cmd, const char* name, enj_error** err)
{
    if (!cmd || !name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_fstring_hash_t hash = enj_fstring_hash(name);

    for (enji_cmdline_argument* arg = cmd->arguments; arg; arg = arg->next)
    {
        if (hash != arg->name->hash)
            continue;

        if (!strncmp(name, arg->name->string, arg->name->length))
            return arg;
    }

    return 0;
}
