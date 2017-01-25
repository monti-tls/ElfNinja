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

#include "elfninja/core/dynamic.h"
#include "elfninja/core/malloc.h"
#include "elfninja/core/blob.h"

#include <string.h>

int enj_dynamic_entry_pull(enj_dynamic_entry* dyn, enj_error** err)
{
    if (!dyn || !dyn->dynamic || !dyn->dynamic->section || !dyn->dynamic->section->elf ||
        !dyn->dynamic->section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = dyn->dynamic->section->elf;

    // Read entry
    if (elf->bits == 32)
    {
        if (enj_blob_read(elf->blob, dyn->header->pos, &dyn->dyn32, sizeof(Elf32_Dyn), err) < 0)
            return -1;
    }
    else if (elf->bits == 64)
    {
        if (enj_blob_read(elf->blob, dyn->header->pos, &dyn->dyn64, sizeof(Elf64_Dyn), err) < 0)
            return -1;
    }

    // Setup string value if applicable
    if (dyn->dynamic->strtab)
    {
        size_t tag = ENJ_DYNAMIC_ENTRY_GET(dyn, d_tag);

        if (tag == DT_NEEDED || tag == DT_SONAME || tag == DT_RPATH ||
            tag == DT_RUNPATH)
        {
            size_t string = ENJ_DYNAMIC_ENTRY_GET(dyn, d_un.d_val);
            size_t name_off = ENJ_ELF_SHDR_GET(dyn->dynamic->strtab, sh_offset) + string;

            if (!(dyn->string = enj_blob_new_anchor(elf->blob, name_off, err)))
                return -1;
        }
    }

    // Update the entry
    if (enj_dynamic_entry_update(dyn, err) < 0)
        return -1;

    return 0;
}

int enj_dynamic_entry_update(enj_dynamic_entry* dyn, enj_error** err)
{
    if (!dyn || !dyn->dynamic || !dyn->dynamic->section || !dyn->dynamic->section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = dyn->dynamic->section->elf;

    dyn->tag = ENJ_DYNAMIC_ENTRY_GET(dyn, d_tag);
    dyn->value = ENJ_DYNAMIC_ENTRY_GET(dyn, d_un.d_val);

    // Clear cached string value
    if (dyn->cached_string)
    {
        enj_fstring_delete(dyn->cached_string);
        dyn->cached_string = 0;
    }

    // Cache string value (if applicable)
    if (dyn->string)
    {
        int string_ok = 1;
        size_t string_len = 0;
        char c;
        do
        {
            if (enj_blob_read(elf->blob, dyn->string->pos + string_len, &c, 1, err) < 0)
            {
                string_ok = 0;
                break;
            }

            ++string_len;
        } while (c != '\0');

        if (string_ok)
        {
            char buffer[string_len];
            if (enj_blob_read(elf->blob, dyn->string->pos, &buffer[0], string_len, err) < 0)
                return -1;

            dyn->cached_string = enj_fstring_create(&buffer[0], err);
            if (!dyn->cached_string)
                return -1;
        }
    }

    return 0;
}

int enj_dynamic_entry_push(enj_dynamic_entry* dyn, enj_error** err)
{
    if (!dyn || !dyn->dynamic || !dyn->dynamic->section || !dyn->dynamic->section->elf ||
        !dyn->dynamic->section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = dyn->dynamic->section->elf;

    ENJ_DYNAMIC_ENTRY_SET(dyn, d_tag, dyn->tag);
    ENJ_DYNAMIC_ENTRY_SET(dyn, d_un.d_val, dyn->value);

    // Update name index
    if (dyn->string && dyn->dynamic->strtab && dyn->dynamic->strtab->data)
        ENJ_DYNAMIC_ENTRY_SET(dyn, d_un.d_val, dyn->string->pos - dyn->dynamic->strtab->data->start->pos);

    // Write entry
    if (elf->bits == 32)
    {
        if (enj_blob_write(elf->blob, dyn->header->pos, &dyn->dyn32, sizeof(Elf32_Dyn), err) < 0)
            return -1;
    }
    else if (elf->bits == 64)
    {
        if (enj_blob_write(elf->blob, dyn->header->pos, &dyn->dyn64, sizeof(Elf64_Dyn), err) < 0)
            return -1;
    }

    return 0;
}

int enj_dynamic_entry__delete(enj_dynamic_entry* dyn, enj_error** err)
{
    if (!dyn || !dyn->dynamic || !dyn->dynamic->section || !dyn->dynamic->section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = dyn->dynamic->section->elf;

    if ((dyn->header && enj_blob_remove_anchor(elf->blob, dyn->header, err) < 0) ||
        (dyn->string && enj_blob_remove_anchor(elf->blob, dyn->string, err) < 0))
    {
        return -1;
    }

    if (dyn->cached_string)
        enj_fstring_delete(dyn->cached_string);

    enj_free(dyn);

    return 0;
}

int enj_dynamic__pull(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->elf || !section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = section->elf;

    enj_dynamic* dynamic = enj_malloc(sizeof(enj_dynamic));
    if (!dynamic)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return -1;
    }

    section->content = dynamic;
    dynamic->section = section;
    dynamic->entries = 0;
    dynamic->last_entry = 0;

    size_t strtabndx = ENJ_ELF_SHDR_GET(section, sh_link);
    dynamic->strtab = enj_elf_find_shdr_by_index(elf, strtabndx, err);
    if (*err)
        return -1;

    // Read section parameters
    size_t offset = ENJ_ELF_SHDR_GET(section, sh_offset);
    size_t size = ENJ_ELF_SHDR_GET(section, sh_size);
    size_t entsize = ENJ_ELF_SHDR_GET(section, sh_entsize);

    // Read table entries
    for (size_t pos = 0; pos < size; pos += entsize)
    {
        enj_dynamic_entry* dyn = enj_malloc(sizeof(enj_dynamic_entry));
        if (!dyn)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            return -1;
        }

        dyn->dynamic = dynamic;

        if (!(dyn->header = enj_blob_new_anchor(elf->blob, offset + pos, err)) ||
            enj_dynamic_entry_pull(dyn, err) < 0)
        {
            enj_free(dyn);
            return -1;
        }

        // Insert the descriptor into the linked list
        dyn->prev = dynamic->last_entry;
        dyn->next = 0;
        if (dyn->prev)
            dyn->prev->next = dyn;
        else
            dynamic->entries = dyn;
        dynamic->last_entry = dyn;
    }

    return 0;
}

int enj_dynamic__update(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_dynamic* dynamic = (enj_dynamic*) section->content;

    for (enj_dynamic_entry* dyn = dynamic->entries; dyn; dyn = dyn->next)
    {
        if (enj_dynamic_entry_update(dyn, err) < 0)
            return -1;
    }

    return 0;
}

int enj_dynamic__push(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_dynamic* dynamic = (enj_dynamic*) section->content;

    for (enj_dynamic_entry* dyn = dynamic->entries; dyn; dyn = dyn->next)
    {
        if (enj_dynamic_entry_push(dyn, err) < 0)
            return -1;
    }

    if (dynamic->strtab)
    {
        ENJ_ELF_SHDR_SET(section, sh_link, dynamic->strtab->index);
    }
    else
    {
        ENJ_ELF_SHDR_SET(section, sh_link, SHN_UNDEF);
    }

    return 0;
}

int enj_dynamic__delete(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_dynamic* dynamic = (enj_dynamic*) section->content;

    for (enj_dynamic_entry* dyn = dynamic->entries; dyn; )
    {
        enj_dynamic_entry* next = dyn->next;

        if (enj_dynamic_entry__delete(dyn, err) < 0)
            return -1;

        dyn = next;
    }

    enj_free(dynamic);

    return 0;
}
