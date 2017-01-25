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

#include "elfninja/core/trait/layout.h"
#include "elfninja/core/malloc.h"

enj_trait_layout* enj_trait_layout_create(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_trait_layout* lay = enj_malloc(sizeof(enj_trait_layout));
    if (!lay)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    lay->trait.arg = lay;
    lay->trait.o_build = &enj_trait_layout__build;

    if (enj_elf_add_trait(elf, &lay->trait, err) < 0 ||
        enj_trait_layout__build(lay, err) < 0)
    {
        enj_free(lay);
        return 0;
    }

    return lay;
}

void enj_trait_layout_delete(enj_trait_layout* lay)
{
    if (!lay)
        return;

    for (enj_trait_layout_chunk* chunk = lay->chunks; chunk; )
    {
        enj_trait_layout_chunk* next = chunk->next;
        enj_free(chunk);
        chunk = next;
    }

    enj_elf_trait_remove(&lay->trait, 0);
    enj_free(lay);
}

int enj_trait_layout__link_chunk(enj_trait_layout* lay, enj_trait_layout_chunk* chunk, enj_error** err)
{
    if (!lay || !chunk)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    chunk->prev = lay->last_chunk;
    chunk->next = 0;
    if (chunk->prev)
        chunk->prev->next = chunk;
    else
        lay->chunks = chunk;
    lay->last_chunk = chunk;

    return 0;
}

enj_trait_layout_chunk* enj_trait_layout__add_chunk(enj_trait_layout* lay, size_t offset, size_t size, size_t type, enj_error** err)
{
    if (!lay)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_trait_layout_chunk* chunk = enj_malloc(sizeof(enj_trait_layout_chunk));
    if (!chunk)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    chunk->offset = offset;
    chunk->size = size;
    chunk->type = type;

    if (enj_trait_layout__link_chunk(lay, chunk, err) < 0)
        return 0;

    return chunk;
}

int enj_trait_layout__sort(enj_trait_layout* lay, enj_error** err)
{
    if (!lay)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

again:
    for (enj_trait_layout_chunk* chunk = lay->chunks; chunk && chunk->next; chunk = chunk->next)
    {
        enj_trait_layout_chunk* other = chunk->next;

        if (chunk->offset > other->offset)
        {
            enj_trait_layout_chunk* pred = chunk->prev;
            enj_trait_layout_chunk* succ = other->next;

            chunk->prev = other;
            chunk->next = succ;

            other->next = chunk;
            other->prev = pred;

            if (pred)
                pred->next = other;
            else
                lay->chunks = other;

            if (succ)
                succ->prev = chunk;
            else
                lay->last_chunk = chunk;

            goto again;
        }
    }

    return 0;
}

int enj_trait_layout__build(void* arg, enj_error** err)
{
    enj_trait_layout* lay = (enj_trait_layout*) arg;

    if (!lay || !lay->trait.elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Remove all chunks
    for (enj_trait_layout_chunk* chunk = lay->chunks; chunk; )
    {
        enj_trait_layout_chunk* next = chunk->next;
        enj_free(chunk);
        chunk = next;
    }

    lay->chunks = 0;
    lay->last_chunk = 0;

    enj_trait_layout_chunk* chunk;

    // Add chunk for ELF header
    size_t ehsize = ENJ_ELF_EHDR_GET(lay->trait.elf, e_ehsize);

    if (!(chunk = enj_trait_layout__add_chunk(lay, 0, ehsize, ENJ_TRAIT_LAYOUT_EHDR, err)))
        return -1;

    // Then for program headers table
    size_t phoff = ENJ_ELF_EHDR_GET(lay->trait.elf, e_phoff);
    size_t phentsize = ENJ_ELF_EHDR_GET(lay->trait.elf, e_phentsize);
    size_t phnum = ENJ_ELF_EHDR_GET(lay->trait.elf, e_phnum);

    if (phnum && !(chunk = enj_trait_layout__add_chunk(lay, phoff, phentsize * phnum, ENJ_TRAIT_LAYOUT_PHDRS, err)))
        return -1;

    // Then for section headers table
    size_t shoff = ENJ_ELF_EHDR_GET(lay->trait.elf, e_shoff);
    size_t shentsize = ENJ_ELF_EHDR_GET(lay->trait.elf, e_shentsize);
    size_t shnum = ENJ_ELF_EHDR_GET(lay->trait.elf, e_shnum);

    if (shnum && !(chunk = enj_trait_layout__add_chunk(lay, shoff, shentsize * shnum, ENJ_TRAIT_LAYOUT_SHDRS, err)))
        return -1;

    // Process sections
    for (enj_elf_shdr* section = lay->trait.elf->sections; section && lay->chunks; section = section->next)
    {
        if (ENJ_ELF_SHDR_GET(section, sh_type) == SHT_NOBITS)
            continue;

        size_t offset = ENJ_ELF_SHDR_GET(section, sh_offset);
        size_t size = ENJ_ELF_SHDR_GET(section, sh_size);

        if (!size)
            continue;

        if (!(chunk = enj_trait_layout__add_chunk(lay, offset, size, ENJ_TRAIT_LAYOUT_SECTION, err)))
            return -1;

        chunk->section = section;
    }

    if (enj_trait_layout__sort(lay, err) < 0)
        return -1;

    enj_trait_layout_chunk* last_chunk = lay->last_chunk;

    for (enj_trait_layout_chunk* chunk = lay->chunks; chunk && chunk->next && chunk != last_chunk; chunk = chunk->next)
    {
        size_t end = chunk->offset + chunk->size;

        if (end == chunk->next->offset)
        {
            continue;
        }
        else if (end < chunk->next->offset)
        {
            size_t gap_size = chunk->next->offset - end;

            if (!enj_trait_layout__add_chunk(lay, end, gap_size, ENJ_TRAIT_LAYOUT_FREE, err))
                return -1;
        }
    }

    if (enj_trait_layout__sort(lay, err) < 0)
        return -1;

    if (lay->last_chunk)
    {
        enj_trait_layout_chunk* chunk = lay->last_chunk;
        size_t end = chunk->offset + chunk->size;
        if (end < lay->trait.elf->blob->buffer_size)
        {
            size_t gap_size = lay->trait.elf->blob->buffer_size - end;

            if (!enj_trait_layout__add_chunk(lay, end, gap_size, ENJ_TRAIT_LAYOUT_FREE, err))
                return -1;
        }
    }

    if (enj_trait_layout__sort(lay, err) < 0)
        return -1;

    return 0;
}
