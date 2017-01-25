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

#include "tool.h"

#include <string.h>

static enjp_tool* _tools = 0;
static enjp_tool* _last_tool = 0;

enjp_tool* enjp_tools()
{
    return _tools;
}

enjp_tool* enjp_tool_resolve(const char* name, enj_error** err)
{
    if (!name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    for (enjp_tool* tool = _tools; tool; tool = tool->next)
    {
        if (!strcmp(name, tool->name))
            return tool;
    }

    return 0;
}

int enjp_tool_register(enjp_tool* tool, enj_error** err)
{
    if (!tool || !tool->name || !tool->description)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (enjp_tool_resolve(tool->name, 0))
    {
        enj_error_put(err, ENJ_ERR_EXISTS);
        return -1;
    }

    tool->prev = _last_tool;
    tool->next = 0;
    if (tool->prev)
        tool->prev->next = tool;
    else
        _tools = tool;
    _last_tool = tool;

    return 0;
}
