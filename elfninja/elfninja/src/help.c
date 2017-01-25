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

#include "elfninja/core/core.h"

#include "tool.h"
#include "log.h"

#include <stdio.h>

static const char* _help_msg =
".:: ElfNinja version " ENJ_VERSION " (" ENJ_BUILD_DATE ") ::.\n"
"\n"
"ElfNinja is a general purpose program for viewing and editing files in the\n"
"Executable and Linkable Format (ELF). It ca be used for debugging, reverse\n"
"engineering or learning purposes.\n"
"ElfNinja is licensed under the GNU GPLv3. Type 'elfninja license' for more.\n"
"\n"
".:: How to use this program ::.\n"
"\n"
"ElfNinja provides its functionnality via different tools. You can launch a\n"
"given tool using 'elfninja <tool>'.\n"
"\n"
"Here are the available tools :\n"
"%s"
"\n"
"You can see more help on a tool by typing 'elfninja help <tool>'.\n"
;

static const char* _help_help_msg =
"Use 'elfninja help' to display a general help message, list available tools\n"
"and exit.\n"
"Use 'elfninja help <tool>' to display the help page of the given tool.\n"
;

int enjp_help_help(enji_cmdline* cmd)
{
	if (!cmd)
		return -1;

	printf("%s", _help_help_msg);

	return 0;
}

int enjp_help_run(enji_cmdline* cmd)
{
	if (!cmd->arguments)
	{
		char buffer[4096];
		size_t pos = 0;
		for (enjp_tool* tool = enjp_tools(); tool; tool = tool->next)
			pos += snprintf(&buffer[pos], sizeof(buffer) - pos, "  %-8s %s\n", tool->name, tool->description);
		buffer[pos] = '\0';
		printf(_help_msg, &buffer[0]);
	}
	else
	{
		enji_cmdline_argument* arg = cmd->arguments;

		enjp_tool* t = enjp_tool_resolve(arg->name->string, 0);
		if (!t)
			enjp_fatal(0, "No tool named '%s'. Try 'elfninja help'.", arg->name->string);

		printf(".:: Help for the tool '%s' ::.\n\n", arg->name->string);

		if (!t->help)
		{
			printf("Sorry, this tool does not have any help.\n");
		}
		else
		{
			(*t->help)(cmd);
		}
	}

    return 0;
}

static enjp_tool _this_tool =
{
	"help",
	"View this page or a tool help page",
	&enjp_help_help,
	&enjp_help_run
};

static __attribute__((constructor(101))) void _register()
{
	enj_error* err = 0;

	if (enjp_tool_register(&_this_tool, &err) < 0)
		enjp_fatal(&err, "Unable to register tool 'help'");
}
