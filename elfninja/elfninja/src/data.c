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

#include "data.h"
#include "tool.h"
#include "log.h"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fnmatch.h>
#include <errno.h>

static enjp_data_command* _commands = 0;
static enjp_data_command* _last_command = 0;

enjp_data_command* enjp_data_commands()
{
    return _commands;
}

enjp_data_command* enjp_data_resolve_command(const char* name, enj_error** err)
{
    if (!name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    for (enjp_data_command* cmd = _commands; cmd; cmd = cmd->next)
    {
        if (!strcmp(name, cmd->name))
            return cmd;
    }

    return 0;
}

int enjp_data_register_command(enjp_data_command* cmd, enj_error** err)
{
    if (!cmd || !cmd->name || !cmd->description || !cmd->run)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (enjp_data_resolve_command(cmd->name, 0))
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

int enjp_data_read_input(enjp_data_tool* d, enji_cmdline_argument* arg, const char* command, enj_error** err)
{
    if (!d || !arg || !command)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!arg->value)
    {
        enjp_error(0, "No offset specified. Try 'elfninja help data %s'", command);
        return -1;
    }

    d->file_offset = enji_parse_offset(d->elf, arg->value, err);
    if (*err)
    {
        enjp_error(err, "Invalid start offset");
        return -1;
    }

    if (err || d->file_offset > d->elf->blob->buffer_size)
    {
        enjp_error(err, "Invalid start offset");
        return -1;
    }

    enji_cmdline_option* opt = 0;
    if ((opt = enji_cmdline_find_option(d->cmd, "hex", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            return -1;
        }

        d->free_bytes = 1;
        d->bytes = enji_parse_hex(opt->value, &d->length, err);
        if (!d->bytes)
        {
            enjp_error(err, "Invalid data");
            return -1;
        }
    }
    else if ((opt = enji_cmdline_find_option(d->cmd, "chars", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            return -1;
        }

        d->bytes = opt->value;
        d->length = strlen(opt->value);

        if (!d->length)
            enjp_warning(0, "Writing empty string");
    }
    else if ((opt = enji_cmdline_find_option(d->cmd, "string", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            return -1;
        }

        d->bytes = opt->value;
        d->length = strlen(opt->value) + 1;
    }
    else if ((opt = enji_cmdline_find_option(d->cmd, "file", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            return -1;
        }

        d->file = fopen(opt->value, "rb");
        if (!d->file)
        {
            enj_error_put_posix_errno(err, ENJ_ERR_IO, errno);
            enjp_error(err, "Unable to open file '%s'", opt->value);
            return -1;
        }

        size_t pos = ftell(d->file);
        fseek(d->file, 0, SEEK_END);
        d->length = ftell(d->file) - pos;
        rewind(d->file);
    }
    else
    {
        enjp_error(0, "No data specified. Try 'elfninja help data %s'", command);
        return -1;
    }

    if (!d->file && (opt = enji_cmdline_find_option(d->cmd, "count", ENJI_CMDLINE_TOOL, arg, 0)))
    {
        if (!opt->value)
        {
            enjp_error(0, "Option '%s' expects a value", opt->name->string);
            return -1;
        }

        d->count = enji_parse_number(opt->value, err);
        if (err || !d->count)
        {
            enjp_error(err, "Invalid value for option '%s'", opt->name->string);
            return -1;
        }
    }
    else
    {
        d->count = 1;
    }

    d->effective_length = d->count * d->length;

    if (d->file_offset + d->effective_length > d->elf->blob->buffer_size)
    {
        enjp_warning(0, "Data is too long, ignoring %ld bytes", (d->file_offset + d->length) - d->elf->blob->buffer_size);
        d->effective_length = d->elf->blob->buffer_size - d->file_offset;

        d->count = (d->effective_length + d->length - 1) / d->length;
    }

    if (d->file)
    {
        d->free_bytes = 1;
        d->bytes = enj_malloc(d->effective_length);
        if (!d->bytes)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            enjp_error(err, "Unable to read file contents");
            return -1;
        }

        size_t count = fread(d->bytes, d->effective_length, 1, d->file);
        if (count != d->effective_length)
        {
            enjp_error(0, "Unable to read file contents");
            return -1;
        }
    }

    return 0;
}

static const char* _help_msg =
"List of available commands :\n"
"%s"
;

int enjp_data_help(enji_cmdline* cmd)
{
    if (!cmd)
        return -1;

    enji_cmdline_argument* arg = cmd->arguments->next;

    if (!arg)
    {
        char buffer[4096];
        size_t pos = 0;
        for (enjp_data_command* cmd = _commands; cmd; cmd = cmd->next)
            pos += snprintf(&buffer[pos], sizeof(buffer) - pos, "  %-8s %s\n", cmd->name, cmd->description);
        buffer[pos] = '\0';

        printf(_help_msg, &buffer[0]);
    }
    else
    {
        enjp_data_command* cmd = enjp_data_resolve_command(arg->name->string, 0);
        if (!cmd)
        {
            enjp_error(0, "No data command '%s'", arg->name->string);
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

int enjp_data_run(enji_cmdline* cmd)
{
    if (!cmd)
        return -1;

    enj_error* err = 0;

    enjp_data_tool d;
    memset(&d, 0, sizeof(d));
    d.cmd = cmd;

    // Get file name from command line
    enji_cmdline_argument* file = enji_cmdline_find_argument_by_index(cmd, 0, 0);
    if (!file)
        enjp_fatal(0, "No file specified. Try 'elfninja data help'");

    // Rebase all options
    if (enji_cmdline_rebase_options(cmd, file, &err) < 0)
        enjp_fatal(&err, "Unable to rebase cmdline options");

    // Try to open the file
    int fd = open(file->name->string, O_RDWR);
    if (fd <= 0)
        enjp_fatal(0, "Unable to open '%s' for writing", file->name->string);

    // Create the ELF object
    d.elf = enj_elf_create_fd(fd, &err);
    if (!d.elf)
    {
        enjp_error(&err, "Unable to read file '%s' as ELF", file->name->string);
        close(fd);
        return -1;
    }

    // Check if there's a subsequent argument
    if (!file->next)
    {
        enjp_error(0, "No command specified. Try 'elfninja data help'");
        enj_elf_delete(d.elf);
        close(fd);
        return -1;
    }

    // Process all arguments sequentially as commands
    for (enji_cmdline_argument* arg = file->next; arg; arg = arg->next)
    {
        enjp_data_command* cmd = enjp_data_resolve_command(arg->name->string, 0);
        if (!cmd)
        {
            enjp_error(0, "No such command '%s'", arg->name->string);
            goto fail;
        }

        if ((*cmd->run)(&d, arg, &err) < 0)
        {
            enjp_error(&err, "Unable to run command '%s'", cmd->name);
            goto fail;
        }
    }

    off_t off = lseek(fd, 0, SEEK_SET);
    if (off < 0)
    {
        enj_error_put_posix_errno(&err, ENJ_ERR_IO, errno);
        enjp_error(&err, "Unable to write back changes to file");
        goto fail;
    }

    size_t count = write(fd, d.elf->blob->buffer, d.elf->blob->buffer_size);
    if (count != d.elf->blob->buffer_size)
    {
        if (count < 0)
            enj_error_put_posix_errno(&err, ENJ_ERR_IO, count);

        enjp_error(&err, "Unable to write back changes to file");
        goto fail;
    }

    if (d.free_bytes)
        enj_free(d.bytes);
    if (d.file)
        fclose(d.file);

    enj_elf_delete(d.elf);
    close(fd);
    return 0;

fail:
    if (d.free_bytes)
        enj_free(d.bytes);
    if (d.file)
        fclose(d.file);

    enj_elf_delete(d.elf);
    close(fd);
    return -1;
}

static enjp_tool _this_tool =
{
    "data",
    "Manipulate file data",
    &enjp_data_help,
    &enjp_data_run
};

static __attribute__((constructor(108))) void _register()
{
    enj_error* err = 0;

    if (enjp_tool_register(&_this_tool, &err) < 0)
        enjp_fatal(&err, "Unable to register tool 'data'");
}
