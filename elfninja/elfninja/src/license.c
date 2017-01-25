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

const char* _license_msg =
"Copyright (C) 2017  Alexandre Monti\n"
"\n"
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
;

int enjp_license_run(enji_cmdline* cmd)
{
	printf("%s", _license_msg);

    return 0;
}

static enjp_tool _this_tool =
{
    "license",
    "Display licensing information and exit",
    0,
    &enjp_license_run
};

static __attribute__((constructor(102))) void _register()
{
    enj_error* err = 0;

    if (enjp_tool_register(&_this_tool, &err) < 0)
        enjp_fatal(&err, "Unable to register tool 'license'");
}