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

#include "elfninja/input/pattern.h"
#include "elfninja/input/parse.h"

#include "elfninja/core/error.h"
#include "elfninja/core/malloc.h"

#include <string.h>
#include <ctype.h>
#include <fnmatch.h>

int enji_pattern_match(enj_elf_shdr* section, const char* pattern, enj_error** err)
{
    if (!section || !pattern)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enum
    {
        OP_EQ,
        OP_LT,
        OP_LTE,
        OP_GT,
        OP_GTE
    };

    const char* p = pattern;
    while (isspace(*p))
        ++p;

    if (*p == '#')
    {
        ++p;

        int op = OP_EQ;
        if (*p == '<')
        {
            ++p;
            if (*p == '=')
            {
                ++p;
                op = OP_LTE;
            }
            else
                op = OP_LT;
        }
        else if (*p == '>')
        {
            ++p;
            if (*p == '=')
            {
                ++p;
                op = OP_GTE;
            }
            else
                op = OP_GT;
        }

        size_t index = enji_parse_number(p, err);
        if (*err)
            return -1;

        switch (op)
        {
            case OP_EQ:
                return section->index == index ? 1 : 0;

            case OP_LT:
                return section->index < index ? 1 : 0;

            case OP_LTE:
                return section->index <= index ? 1 : 0;

            case OP_GT:
                return section->index > index ? 1 : 0;

            case OP_GTE:
                return section->index >= index ? 1 : 0;

            default:
                return 0;
        }
    }

    if (fnmatch(p, section->cached_name->string, FNM_EXTMATCH) == 0)
        return 1;

    return 0;
}

enj_elf_shdr* enji_pattern_match_unique(enj_elf* elf, const char* pattern, enj_error** err)
{
    if (!elf || !pattern)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_elf_shdr* found = 0;

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        int match = enji_pattern_match(section, pattern, err);
        if (match < 0)
            return 0;

        if (match)
        {
            if (!found)
                found = section;
            else
            {
                enj_error_put(err, ENJ_ERR_MULTIPLE_MATCH);
                return 0;
            }
        }
    }

    return found;
}

int enji_pattern_is_index(const char* pattern, enj_error** err)
{
    if (!pattern)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    const char* p = pattern;
    while (isspace(*p))
        ++p;

    return *p == '#';
}
