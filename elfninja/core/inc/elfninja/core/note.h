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

#ifndef __ELFNINJA_CORE_NOTE_H__
#define __ELFNINJA_CORE_NOTE_H__

#include "elfninja/core/error.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/fstring.h"

#include <elf.h>

struct enj_nsect;
struct enj_note;
struct enj_note_content_view;

typedef struct enj_nsect
{
    enj_elf_shdr* section;

    struct enj_note* notes;
    struct enj_note* last_note;
} enj_nsect;

typedef struct enj_note
{
    enj_nsect* nsect;

    enj_fstring* cached_name;
    struct enj_note_content_view* content_view;
    void* content;

    enj_blob_anchor* header;
    enj_blob_cursor* name;
    enj_blob_cursor* desc;

    union
    {
        Elf32_Nhdr nhdr32;
        Elf64_Nhdr nhdr64;
    };

    struct enj_note* next;
    struct enj_note* prev;
} enj_note;

enum
{
    ENJ_NOTE_NONE = 0,

    #define DEF_TAG(tag, id) ENJ_NOTE_ ## tag = (id),
    #include "elfninja/core/note_tags.def"

    ENJ_NOTE_LOUSER
};

typedef struct enj_note_content_view
{
    size_t tag;
    int target_type;
    int (*o_pull)(enj_note* note, enj_error** err);
    int (*o_update)(enj_note* note, enj_error** err);
    int (*o_push)(enj_note* note, enj_error** err);
    int (*o_delete)(enj_note* note, enj_error** err);
} enj_note_content_view;

#define ENJ_NOTE_SIZE(note) (note->nsect->section->elf->bits == 64 ? sizeof(note->nhdr64) : sizeof(note->nhdr32))
#define ENJ_NOTE_GET(note, field) (note->nsect->section->elf->bits == 64 ? note->nhdr64.field : note->nhdr32.field)
#define ENJ_NOTE_SET(note, field, value) do {\
        if (note->nsect->section->elf->bits == 64) note->nhdr64.field = (value); \
        else note->nhdr32.field = (value); \
    } while (0);
#define ENJ_NOTE_ALIGNMENT(note) (note->nsect->section->elf->bits == 64 ? sizeof(Elf64_Xword) : sizeof(Elf32_Word))
#define ENJ_NOTE_ALIGN(note, value) (((value) + (ENJ_NOTE_ALIGNMENT(note) - 1)) & ~(ENJ_NOTE_ALIGNMENT(note) - 1))

int enj_note_pull(enj_note* note, enj_error** err);
int enj_note_update(enj_note* note, enj_error** err);
int enj_note_push(enj_note* note, enj_error** err);

int enj_note__delete(enj_note* note, enj_error** err);
int enj_note__get_content_view(enj_note* note, enj_note_content_view** view, enj_error** err);

int enj_nsect__pull(enj_elf_shdr* section, enj_error** err);
int enj_nsect__update(enj_elf_shdr* section, enj_error** err);
int enj_nsect__push(enj_elf_shdr* section, enj_error** err);
int enj_nsect__delete(enj_elf_shdr* section, enj_error** err);

#endif // __ELFNINJA_CORE_NOTE_H__
