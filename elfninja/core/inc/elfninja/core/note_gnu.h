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

#ifndef __ELFNINJA_CORE_NOTE_GNU_H__
#define __ELFNINJA_CORE_NOTE_GNU_H__

#include "elfninja/core/error.h"
#include "elfninja/core/note.h"

#include <elf.h>

typedef struct enj_note_gnu_abi_tag
{
    enj_note* note;

    size_t os;
    size_t abi_major;
    size_t abi_minor;
    size_t abi_subminor;
} enj_note_gnu_abi_tag;

int enj_note_gnu_abi_tag__pull(enj_note* note, enj_error** err);
int enj_note_gnu_abi_tag__update(enj_note* note, enj_error** err);
int enj_note_gnu_abi_tag__push(enj_note* note, enj_error** err);
int enj_note_gnu_abi_tag__delete(enj_note* note, enj_error** err);

typedef struct enj_note_gnu_build_id
{
    enj_note* note;

    size_t length;
    unsigned char* bytes;
} enj_note_gnu_build_id;

int enj_note_gnu_build_id__pull(enj_note* note, enj_error** err);
int enj_note_gnu_build_id__update(enj_note* note, enj_error** err);
int enj_note_gnu_build_id__push(enj_note* note, enj_error** err);
int enj_note_gnu_build_id__delete(enj_note* note, enj_error** err);

#endif // __ELFNINJA_CORE_NOTE_GNU_H__
