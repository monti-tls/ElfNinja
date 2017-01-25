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

#include "dump.h"
#include "tool.h"
#include "log.h"

#include "elfninja/dump/dump.h"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static enjp_dump_command* _commands = 0;
static enjp_dump_command* _last_command = 0;

enjp_dump_command* enjp_dump_commands()
{
    return _commands;
}

enjp_dump_command* enjp_dump_resolve_command(const char* name, enj_error** err)
{
    if (!name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    for (enjp_dump_command* cmd = _commands; cmd; cmd = cmd->next)
    {
        if (!strcmp(name, cmd->name))
            return cmd;
    }

    return 0;
}

int enjp_dump_register_command(enjp_dump_command* cmd, enj_error** err)
{
    if (!cmd || !cmd->name || !cmd->description || !cmd->run)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (enjp_dump_resolve_command(cmd->name, 0))
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
"The 'dump' tool allows viewing the content of an ELF file in a formatted\n"
"fashion. Commands can be sequentially specified in order to select which\n"
"parts of the file to dump using 'elfninja dump <file> <cmd0> ... <cmdN>',\n"
"where <file> is the path to the ELF file to examine and the cmdi's are all\n"
"the commands. Each command can accept options, some than can be set globally.\n"
"For example, in 'elfninja dump <file> --opt <cmd0> --opt2', the '--opt'\n"
"option is passed to all commands whereas the '--opt2 'option will only refer\n"
"to the command 'cmd0'.\n"
"\n"
"Here are the available commands :\n"
"%s"
"\n"
"You can type in 'elfninja help dump <cmd>' to read more information on a\n"
"particular command.\n"
"\n"
".:: Common options cheat sheet ::.\n"
"\n"
"For lazy users who do not want to read sections below, here is a quick list\n"
"of all available common options :\n"
"\n"
"Option              Description\n"
"-------------       ------------------------------------------------------\n"
"--no-flourish       Do not output flourish.\n"
"--no-spacers        Do not output spacers.\n"
"--no-headers        Do not use headers for table entries listing.\n"
"\n"
".:: Flourish and spacers ::.\n"
"\n"
"The 'dump' tool will execute sequentially all the provided commands and\n"
"send their output to stdout. To allow better visibility, flourish and\n"
"spacers are added. Flourish are short titles that separate groups of dumps,\n"
"for example in a symbol dump flourish will be added when the section changes.\n"
"Spacers are simply new lines inserted in between consecutive comments. For\n"
"example, the command 'elfninja dump /bin/ls ehdr ehdr' will output :\n"
"> .:: ELF File Header ::.      <--+-- flourish\n"
">                              <--+\n"
"> class      = ELFCLASS64\n"
"> [...]\n"
"> shstrndx   = 28 (.shstrtab)\n"
">                              <----- spacer\n"
"> .:: ELF File Header ::.\n"
"> \n"
"> class      = ELFCLASS64\n"
"> [...]\n"
"> shstrndx   = 28 (.shstrtab)\n"
"Flourish can be disabled globally or per-command using the option\n"
"--no-flourish. Spacers can also be disabled using --no-spacers.\n"
"Flourish and spacers are both kept enabled by default when a user format\n"
"is provided (see below for more information on formats).\n"
"\n"
".:: Headers ::.\n"
"\n"
"For commands outputting table entries, such as section headers or symbols,\n"
"a header is printed to allow easier identification of the fields and shorter\n"
"output. Headers can be disabled either globally or per-command using the\n"
"--no-headers option.\n"
"Please note that headers will be disabled when a user format is provided.\n"
"\n"
".:: Format strings ::.\n"
"\n"
"Internally, the 'dump' tool uses libelfninja_dump, which is a library that\n"
"dumps ELF structures based on format strings. The output of each command is\n"
"obtained by using a default format string, that can be overriden by the user\n"
"using the --format=<string> option. Note that specifying the string using\n"
"ANSI C quoting ($'fmt') is recommended.\n"
"Format string works in the same way as printf-format strings, except that\n"
"fields are named and specified using the escape character '`'.\n"
"For example `field` will be expanded by the actual field value. The modifier\n"
"character '!' can be used to enforce a fixed width of the field expansion.\n"
"For example, `!10field` will be at least 10 characters long and left-aligned.\n"
"To obtain a right-aligned field the width must be preceded by a '-' character.\n"
"The pad character is a space ' ' by default but can be overriden using the\n"
"'!' character again, such as in `!-10!#field' where '#' will be used for\n"
"padding. Please note that specifying the field width will not work for fields\n"
"marked as 'fixed' (see command help pages for lists of available fields).\n"
;

int enjp_dump_help(enji_cmdline* cmd)
{
    if (!cmd)
        return -1;

    enji_cmdline_argument* arg = cmd->arguments->next;

    if (!arg)
    {
        char buffer[4096];
        size_t pos = 0;
        for (enjp_dump_command* cmd = _commands; cmd; cmd = cmd->next)
            pos += snprintf(&buffer[pos], sizeof(buffer) - pos, "  %-8s %s\n", cmd->name, cmd->description);
        buffer[pos] = '\0';

        printf(_help_msg, &buffer[0]);
    }
    else
    {
        enjp_dump_command* cmd = enjp_dump_resolve_command(arg->name->string, 0);
        if (!cmd)
        {
            enjp_error(0, "No dump command '%s'", arg->name->string);
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

int enjp_dump_run(enji_cmdline* cmd)
{
    if (!cmd)
        return -1;

    enj_error* err = 0;

    enjp_dump_tool d;
    d.cmd = cmd;

    // Get file name from command line
    enji_cmdline_argument* file = enji_cmdline_find_argument_by_index(cmd, 0, 0);
    if (!file)
        enjp_fatal(0, "No file specified. Try 'elfninja dump help'");

    // Rebase all options
    if (enji_cmdline_rebase_options(cmd, file, &err) < 0)
        enjp_fatal(&err, "Unable to rebase cmdline options");

    // Try to open the file
    int fd = open(file->name->string, O_RDONLY);
    if (fd <= 0)
        enjp_fatal(0, "Unable to open '%s'", file->name->string);

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
        enjp_error(0, "No command specified. Try 'elfninja dump help'");
        enj_elf_delete(d.elf);
        close(fd);
        return -1;
    }

    // Process all arguments sequentially as commands
    for (enji_cmdline_argument* arg = file->next; arg; arg = arg->next)
    {
        // Get common options
        d.allow_flourish = 1;
        if (enji_cmdline_find_option(cmd, "no-flourish", ENJI_CMDLINE_TOOL | ENJI_CMDLINE_ORPHAN, arg, 0))
            d.allow_flourish = 0;

        d.allow_spacers = 1;
        if (enji_cmdline_find_option(cmd, "no-spacers", ENJI_CMDLINE_TOOL | ENJI_CMDLINE_ORPHAN, arg, 0))
            d.allow_spacers = 0;

        enjp_dump_command* cmd = enjp_dump_resolve_command(arg->name->string, 0);
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

        if (d.allow_spacers && arg->next)
        {
            printf("\n");
            fflush(stdout);
        }
    }

    enj_elf_delete(d.elf);
    close(fd);
    return 0;

fail:
    enj_elf_delete(d.elf);
    close(fd);
    return -1;
}

static enjp_tool _this_tool =
{
    "dump",
    "Dump file contents as formatted text",
    &enjp_dump_help,
    &enjp_dump_run
};

static __attribute__((constructor(103))) void _register()
{
    enj_error* err = 0;

    if (enjp_tool_register(&_this_tool, &err) < 0)
        enjp_fatal(&err, "Unable to register tool 'dump'");
}
