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

#include "elfninja/dump/note.h"
#include "elfninja/dump/note_gnu.h"
#include "elfninja/core/note.h"

#include <dlfcn.h>

enjd_formatter* enjd_note_formatter_create_by_tag(size_t content_tag, enj_error** err)
{
    typedef enjd_formatter*(*handler_t)(enj_error**);
    handler_t handler = 0;

    int(*user)(size_t, handler_t*, enj_error** err);
    while ((user = dlsym(RTLD_NEXT, "enj_dump_note_get_user_register_handler")))
    {
        if ((*user)(content_tag, &handler, err) < 0)
            return 0;
        else if (*handler)
            return (**handler)(err);
    }

    switch (content_tag)
    {
        case ENJ_NOTE_GNU_ABI_TAG:
            return enjd_note_gnu_abi_tag_formatter_create(err);

        case ENJ_NOTE_GNU_BUILD_ID:
            return enjd_note_gnu_build_id_formatter_create(err);
    }

    enj_error_put(err, ENJ_ERR_NO_HNDL);
    return 0;
}
