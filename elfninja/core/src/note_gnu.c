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

#include "elfninja/core/note_gnu.h"
#include "elfninja/core/malloc.h"
#include "elfninja/core/blob.h"

int enj_note_gnu_abi_tag__pull(enj_note* note, enj_error** err)
{
    enj_note_gnu_abi_tag* tag = enj_malloc(sizeof(enj_note_gnu_abi_tag));
    if (!tag)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return -1;
    }

    note->content = tag;

    if (enj_note_gnu_abi_tag__update(note, err) < 0)
        return -1;

    return 0;
}

int enj_note_gnu_abi_tag__update(enj_note* note, enj_error** err)
{
    if (!note || !note->desc || !note->nsect || !note->nsect->section ||
        !note->nsect->section->elf || !note->nsect->section->elf->bits ||
        !note->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_note_gnu_abi_tag* tag = (enj_note_gnu_abi_tag*) note->content;

    // Note content is arranged in 4 fields
    size_t* fields[] =
    {
        &tag->os,
        &tag->abi_major,
        &tag->abi_minor,
        &tag->abi_subminor
    };

    size_t num_fields = sizeof(fields) / sizeof(size_t*);

    // Get each field's size
    enj_elf* elf = note->nsect->section->elf;
    size_t word_size = elf->bits == 64 ? sizeof(Elf64_Word) : sizeof(Elf32_Word);

    // Check note descriptor size
    if (note->desc->length != num_fields * word_size)
    {
        enj_error_put(err, ENJ_ERR_BAD_SIZE);
        enj_free(tag);
        return -1;
    }

    Elf32_Word word32;
    Elf64_Word word64;

    // Read in the fields
    for (size_t i = 0; i < num_fields; ++i)
    {
        if (elf->bits == 64)
        {
            if (enj_blob_read(elf->blob, note->desc->start->pos + i * word_size, &word64, word_size, err) < 0)
            {
                enj_free(tag);
                return -1;
            }

            *fields[i] = word64;
        }
        else if (elf->bits == 32)
        {
            if (enj_blob_read(elf->blob, note->desc->start->pos + i * word_size, &word32, word_size, err) < 0)
            {
                enj_free(tag);
                return -1;
            }

            *fields[i] = word32;
        }
    }

    return 0;
}

int enj_note_gnu_abi_tag__push(enj_note* note, enj_error** err)
{
    if (!note || !note->desc || !note->nsect || !note->nsect->section ||
        !note->nsect->section->elf || !note->nsect->section->elf->bits ||
        !note->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_note_gnu_abi_tag* tag = note->content;

    // Note content is arranged in 4 fields
    size_t* fields[] =
    {
        &tag->os,
        &tag->abi_major,
        &tag->abi_minor,
        &tag->abi_subminor
    };

    size_t num_fields = sizeof(fields) / sizeof(size_t*);

    // Get each field's size
    enj_elf* elf = note->nsect->section->elf;
    size_t word_size = elf->bits == 64 ? sizeof(Elf64_Word) : sizeof(Elf32_Word);

    // Check note descriptor size
    if (note->desc->length != num_fields * word_size)
    {
        enj_error_put(err, ENJ_ERR_BAD_SIZE);
        enj_free(tag);
        return -1;
    }

    Elf32_Word word32;
    Elf64_Word word64;

    // Write out the fields
    for (size_t i = 0; i < num_fields; ++i)
    {
        if (elf->bits == 64)
        {
            word64 = *fields[i];

            if (enj_blob_write(elf->blob, note->desc->start->pos + i * word_size, &word64, word_size, err) < 0)
            {
                enj_free(tag);
                return -1;
            }
        }
        else if (elf->bits == 32)
        {
            word32 = *fields[i];

            if (enj_blob_write(elf->blob, note->desc->start->pos + i * word_size, &word32, word_size, err) < 0)
            {
                enj_free(tag);
                return -1;
            }
        }
    }

    return 0;
}

int enj_note_gnu_abi_tag__delete(enj_note* note, enj_error** err)
{
    if (!note)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_free(note->content);

    return 0;
}

int enj_note_gnu_build_id__pull(enj_note* note, enj_error** err)
{
    enj_note_gnu_abi_tag* tag = enj_malloc(sizeof(enj_note_gnu_abi_tag));
    if (!tag)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return -1;
    }

    note->content = tag;

    if (enj_note_gnu_build_id__update(note, err) < 0)
        return -1;

    return 0;
}

int enj_note_gnu_build_id__update(enj_note* note, enj_error** err)
{
    if (!note || !note->desc || !note->nsect || !note->nsect->section ||
        !note->nsect->section->elf || !note->nsect->section->elf->bits ||
        !note->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = note->nsect->section->elf;
    enj_note_gnu_build_id* build_id = note->content;

    if (build_id->bytes)
        enj_free(build_id->bytes);

    build_id->length = note->desc->length;
    if (!(build_id->bytes = enj_malloc(build_id->length)))
        return -1;

    if (enj_blob_read(elf->blob, note->desc->start->pos, build_id->bytes, note->desc->length, err) < 0)
        return -1;

    return 0;
}

int enj_note_gnu_build_id__push(enj_note* note, enj_error** err)
{
    if (!note || !note->desc || !note->nsect || !note->nsect->section ||
        !note->nsect->section->elf || !note->nsect->section->elf->bits ||
        !note->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = note->nsect->section->elf;
    enj_note_gnu_build_id* build_id = note->content;

    if (build_id->bytes)
    {
        size_t old_length = note->desc->length;

        if (enj_blob_insert(elf->blob, note->desc->start->pos, build_id->bytes, build_id->length, err) < 0 ||
            enj_blob_remove(elf->blob, note->desc->start->pos + build_id->length, old_length, err) < 0)
        {
            return -1;
        }
    }

    return 0;
}

int enj_note_gnu_build_id__delete(enj_note* note, enj_error** err)
{
    if (!note)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!note->content)
        return 0;

    enj_note_gnu_build_id* build_id = note->content;


    enj_free(build_id->bytes);
    enj_free(build_id);

    return 0;
}
