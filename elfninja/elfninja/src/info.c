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

#include "info.h"
#include "tool.h"
#include "log.h"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fnmatch.h>
#include <errno.h>

static enjp_info_command* _commands = 0;
static enjp_info_command* _last_command = 0;

enjp_info_command* enjp_info_commands()
{
    return _commands;
}

enjp_info_command* enjp_info_resolve_command(const char* name, enj_error** err)
{
    if (!name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    for (enjp_info_command* cmd = _commands; cmd; cmd = cmd->next)
    {
        if (!strcmp(name, cmd->name))
            return cmd;
    }

    return 0;
}

int enjp_info_register_command(enjp_info_command* cmd, enj_error** err)
{
    if (!cmd || !cmd->name || !cmd->description || !cmd->run)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (enjp_info_resolve_command(cmd->name, 0))
    {
        enj_error_put(err, ENJ_ERR_EXISTS);
        return -1;
    }

    cmd->prev = _last_command;
    cmd->next = 0;
    if (cmd->prev)
        cmd->prev->next = cmd;
    else
        _commands = cmd;
    _last_command = cmd;

    return 0;
}

static const char* _help_msg =
"List of available commands :\n"
"%s"
;

int enjp_info_help(enji_cmdline* cmd)
{
    if (!cmd)
        return -1;

    enji_cmdline_argument* arg = cmd->arguments->next;

    if (!arg)
    {
        char buffer[4096];
        size_t pos = 0;
        for (enjp_info_command* cmd = _commands; cmd; cmd = cmd->next)
            pos += snprintf(&buffer[pos], sizeof(buffer) - pos, "  %-8s %s\n", cmd->name, cmd->description);
        buffer[pos] = '\0';

        printf(_help_msg, &buffer[0]);
    }
    else
    {
        enjp_info_command* cmd = enjp_info_resolve_command(arg->name->string, 0);
        if (!cmd)
        {
            enjp_error(0, "No info command '%s'", arg->name->string);
            return -1;
        }

        if (cmd->help)
        {
            (*cmd->help)();
        }
        else
        {
            printf("No help for command '%s'", cmd->name);
        }
    }

    return 0;
}

int enjp_info_run(enji_cmdline* cmd)
{
    if (!cmd)
        return -1;

    enj_error* err = 0;

    enjp_info_tool s;
    s.cmd = cmd;

    // Get file name from command line
    enji_cmdline_argument* file = enji_cmdline_find_argument_by_index(cmd, 0, 0);
    if (!file)
        enjp_fatal(0, "No file specified. Try 'elfninja section help'");

    // Rebase all options
    if (enji_cmdline_rebase_options(cmd, file, &err) < 0)
        enjp_fatal(&err, "Unable to rebase cmdline options");

    // Try to open the file
    int fd = open(file->name->string, O_RDONLY);
    if (fd <= 0)
        enjp_fatal(0, "Unable to open '%s'", file->name->string);

    // Create the ELF object
    s.elf = enj_elf_create_fd(fd, &err);
    if (!s.elf)
    {
        enjp_error(&err, "Unable to read file '%s' as ELF", file->name->string);
        close(fd);
        return -1;
    }

    // Check if there's a subsequent argument
    if (!file->next)
    {
        enjp_error(0, "No command specified. Try 'elfninja section help'");
        enj_elf_delete(s.elf);
        close(fd);
        return -1;
    }

    // Process all arguments sequentially as commands
    for (enji_cmdline_argument* arg = file->next; arg; arg = arg->next)
    {
        enjp_info_command* cmd = enjp_info_resolve_command(arg->name->string, 0);
        if (!cmd)
        {
            enjp_error(0, "No such command '%s'", arg->name->string);
            goto fail;
        }

        if ((*cmd->run)(&s, arg, &err) < 0)
        {
            enjp_error(&err, "Unable to run command '%s'", cmd->name);
            goto fail;
        }
    }

    enj_elf_delete(s.elf);
    close(fd);
    return 0;

fail:
    enj_elf_delete(s.elf);
    close(fd);
    return -1;
}

static enjp_tool _this_tool =
{
    "info",
    "Display high-level information about the ELF",
    &enjp_info_help,
    &enjp_info_run
};

static __attribute__((constructor(107))) void _register()
{
    enj_error* err = 0;

    if (enjp_tool_register(&_this_tool, &err) < 0)
        enjp_fatal(&err, "Unable to register tool 'info'");
}
