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

#include "elfninja/core/note.h"
#include "elfninja/core/malloc.h"
#include "elfninja/core/blob.h"
#include "elfninja/core/note_gnu.h"

#include <string.h>
#include <dlfcn.h>

int enj_note_pull(enj_note* note, enj_error** err)
{
    if (!note || !note->nsect || !note->nsect->section || !note->nsect->section->elf ||
        !note->nsect->section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = note->nsect->section->elf;

    // Read note header
    if (elf->bits == 32)
    {
        if (enj_blob_read(elf->blob, note->header->pos, &note->nhdr32, sizeof(Elf32_Nhdr), err) < 0)
            return -1;
    }
    else if (elf->bits == 64)
    {
        if (enj_blob_read(elf->blob, note->header->pos, &note->nhdr64, sizeof(Elf64_Nhdr), err) < 0)
            return -1;
    }

    if (note->name)
    {
        if (enj_blob_remove_cursor(elf->blob, note->name, err) < 0)
            return -1;

        note->name = 0;
    }

    // Create name cursor (don't forget null byte)
    size_t name_off = ENJ_NOTE_SIZE(note);
    size_t name_size = ENJ_NOTE_GET(note, n_namesz) + 1;

    if (!(note->name = enj_blob_new_cursor(elf->blob, note->header->pos + name_off, name_size, err)))
        return -1;

    if (note->desc)
    {
        if (enj_blob_remove_cursor(elf->blob, note->desc, err) < 0)
            return -1;

        note->desc = 0;
    }

    // Create desc cursor (don't forget alignment)
    size_t desc_size = ENJ_NOTE_GET(note, n_descsz);
    size_t desc_off = ENJ_NOTE_ALIGN(note, name_off + name_size-1);

    if (!(note->desc = enj_blob_new_cursor(elf->blob, note->header->pos + desc_off, desc_size, err)))
        return -1;

    // Update the note entry
    if (enj_note_update(note, err) < 0)
        return -1;

    return 0;
}

int enj_note_update(enj_note* note, enj_error** err)
{
    if (!note || !note->nsect || !note->nsect->section || !note->nsect->section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = note->nsect->section->elf;

    // Clear cached symbol name
    if (note->cached_name)
    {
        enj_fstring_delete(note->cached_name);
        note->cached_name = 0;
    }

    // Cache symbol name (if available)
    if (note->name)
    {
        char buffer[note->name->length];
        if (enj_blob_read(elf->blob, note->name->start->pos, &buffer[0], note->name->length, err) < 0)
            return -1;

        note->cached_name = enj_fstring_create(&buffer[0], err);
        if (!note->cached_name)
            return -1;
    }

    size_t type = ENJ_NOTE_GET(note, n_type);

    // Update content view if the note type changed, or get one
    if (!note->content_view || (note->content_view && note->content_view->target_type != type))
    {
        if (note->content_view && note->content_view->o_delete &&
            (*note->content_view->o_delete)(note, err) < 0)
            return -1;

        note->content_view = 0;
        note->content = 0;

        if (enj_note__get_content_view(note, &note->content_view, err) < 0)
            return -1;

        if (note->content_view && note->content_view->o_pull &&
            (*note->content_view->o_pull)(note, err) < 0)
            return -1;
    }
    // Otherwise, just update the content
    else
    {
        if (note->content_view && note->content_view->o_update &&
            (*note->content_view->o_update)(note, err) < 0)
            return -1;
    }

    return 0;
}

int enj_note_push(enj_note* note, enj_error** err)
{
    if (!note || !note->nsect || !note->nsect->section || !note->nsect->section->elf ||
        !note->nsect->section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = note->nsect->section->elf;

    // Push eventual note content
    if (note->content_view && note->content_view->o_push &&
        (note->content_view->o_push)(note, err) < 0)
        return -1;

    // Update name size
    if (note->name)
    {
        size_t name_size = note->name->length ? note->name->length - 1 : 0;
        ENJ_NOTE_SET(note, n_namesz, name_size);
    }

    //TODO: alignment of descriptor ??

    // Update desc size
    if (note->desc)
    {
        ENJ_NOTE_SET(note, n_descsz, note->desc->length);
    }

    // Write note header
    if (elf->bits == 32)
    {
        if (enj_blob_write(elf->blob, note->header->pos, &note->nhdr32, sizeof(Elf32_Nhdr), err) < 0)
            return -1;
    }
    else if (elf->bits == 64)
    {
        if (enj_blob_write(elf->blob, note->header->pos, &note->nhdr64, sizeof(Elf64_Nhdr), err) < 0)
            return -1;
    }

    return 0;
}

int enj_note__delete(enj_note* note, enj_error** err)
{
    if (!note || !note->nsect || !note->nsect->section || !note->nsect->section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = note->nsect->section->elf;

    if (note->content)
    {
        if (note->content_view && note->content_view->o_delete &&
            (*note->content_view->o_delete)(note, err) < 0)
            return -1;
    }

    if ((note->header && enj_blob_remove_anchor(elf->blob, note->header, err) < 0) ||
        (note->name && enj_blob_remove_cursor(elf->blob, note->name, err) < 0) ||
        (note->desc && enj_blob_remove_cursor(elf->blob, note->desc, err) < 0))
    {
        return -1;
    }

    if (note->cached_name)
        enj_fstring_delete(note->cached_name);

    enj_free(note);

    return 0;
}

int enj_note__get_content_view(enj_note* note, enj_note_content_view** view, enj_error** err)
{
    if (!note || !view)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    int(*user)(enj_note*, enj_note_content_view**, enj_error**) = 0;
    while ((user = dlsym(RTLD_NEXT, "enj_note_get_user_content_view")))
    {
        if ((*user)(note, view, err) < 0)
            return -1;
        else if (*view)
            return 0;
    }

    static enj_note_content_view gnu_abi_tag =
    {
        ENJ_NOTE_GNU_ABI_TAG,
        NT_GNU_ABI_TAG,
        &enj_note_gnu_abi_tag__pull,
        &enj_note_gnu_abi_tag__update,
        &enj_note_gnu_abi_tag__push,
        &enj_note_gnu_abi_tag__delete
    };

    //TODO: add support for NT_GNU_HWCAP

    static enj_note_content_view gnu_build_id =
    {
        ENJ_NOTE_GNU_BUILD_ID,
        NT_GNU_BUILD_ID,
        &enj_note_gnu_build_id__pull,
        &enj_note_gnu_build_id__update,
        &enj_note_gnu_build_id__push,
        &enj_note_gnu_build_id__delete
    };

    size_t type = ENJ_NOTE_GET(note, n_type);

    switch (type)
    {
        case NT_GNU_ABI_TAG:
            *view = &gnu_abi_tag;
            return 0;

        case NT_GNU_BUILD_ID:
            *view = &gnu_build_id;
            return 0;
    }

    *view = 0;
    return 0;
}

int enj_nsect__pull(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->elf || !section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = section->elf;

    if (section->content && enj_nsect__delete(section, err) < 0)
        return -1;

    enj_nsect* nsect = enj_malloc(sizeof(enj_nsect));
    if (!nsect)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return -1;
    }

    section->content = nsect;
    nsect->section = section;
    nsect->notes = 0;
    nsect->last_note = 0;

    // Read section parameters
    size_t offset = ENJ_ELF_SHDR_GET(section, sh_offset);
    size_t size = ENJ_ELF_SHDR_GET(section, sh_size);

    // Read note entries
    for (size_t pos = 0; pos < size; )
    {
        enj_note* note = enj_malloc(sizeof(enj_note));
        if (!note)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            return -1;
        }

        note->nsect = nsect;

        if (!(note->header = enj_blob_new_anchor(elf->blob, offset + pos, err)) ||
            enj_note_pull(note, err) < 0)
        {
            enj_free(note);
            return -1;
        }

        // Goto next entry considering alignment
        size_t note_size = ENJ_NOTE_ALIGN(note, note->desc->end->pos - note->header->pos);
        pos += note_size;

        // Insert the descriptor into the linked list
        note->prev = nsect->last_note;
        note->next = 0;
        if (note->prev)
            note->prev->next = note;
        else
            nsect->notes = note;
        nsect->last_note = note;
    }

    return 0;
}

int enj_nsect__update(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_nsect* nsect = (enj_nsect*) section->content;

    for (enj_note* note = nsect->notes; note; note = note->next)
    {
        if (enj_note_update(note, err) < 0)
            return -1;
    }

    return 0;
}

int enj_nsect__push(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_nsect* nsect = (enj_nsect*) section->content;

    for (enj_note* note = nsect->notes; note; note = note->next)
    {
        if (enj_note_push(note, err) < 0)
            return -1;
    }

    return 0;
}

int enj_nsect__delete(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_nsect* nsect = (enj_nsect*) section->content;

    for (enj_note* note = nsect->notes; note; )
    {
        enj_note* next = note->next;

        if (enj_note__delete(note, err) < 0)
            return -1;

        note = next;
    }

    enj_free(nsect);

    return 0;
}
