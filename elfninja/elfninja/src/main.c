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

#include <stdio.h>
#include <stdlib.h>

#include "elfninja/core/core.h"
#include "elfninja/dump/dump.h"

#include "log.h"
#include "tool.h"

int main(int argc, char** argv)
{
    enj_error* err = 0;

    // Parse command line
    enji_cmdline* cmd = enji_cmdline_create(argc, argv, &err);

    if (!cmd)
        enjp_fatal(0, "Unable to read command arguments");

    if (!cmd->tool)
        enjp_fatal(0, "No tool specified. Try 'elfninja help'.");

    // Process global options
    // ...

    // Resolve tool
    enjp_tool* t = enjp_tool_resolve(cmd->tool, &err);
    if (!t)
    {
        if (err)
            enjp_fatal(&err, "Unable to resolve tool.");

        enjp_fatal(0, "No tool named '%s'. Try 'elfninja help'.", cmd->tool);
    }

    // Run tool
    if (t->run)
        (*t->run)(cmd);

    // Cleanup and exit
    enji_cmdline_delete(cmd);
    return 0;
}
