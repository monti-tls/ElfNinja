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

#include "elfninja/core/elf.h"
#include "elfninja/core/malloc.h"

#include "elfninja/core/symtab.h"
#include "elfninja/core/note.h"
#include "elfninja/core/dynamic.h"

#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

enj_elf* enj_elf_create_fd(int fd, enj_error** err)
{
    if (fd <= 0)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_elf* elf = enj_malloc(sizeof(enj_elf));
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    if (!(elf->blob = enj_blob_create(err)))
    {
        enj_free(elf);
        return 0;
    }

    unsigned char* buffer[elf->blob->chunk_size];
    ssize_t count;
    while ((count = read(fd, &buffer[0], elf->blob->chunk_size)) > 0)
    {
        if (enj_blob_insert(elf->blob, elf->blob->buffer_size, &buffer[0], count, err) < 0)
        {
            enj_blob_delete(elf->blob);
            enj_free(elf);
            return 0;
        }
    }

    if (count < 0)
    {
        enj_error_put_posix_errno(err, ENJ_ERR_IO, count);
        enj_blob_delete(elf->blob);
        enj_free(elf);
        return 0;
    }

    if (enj_elf_pull(elf, err) < 0)
        return 0;

    return elf;
}

enj_elf* enj_elf_create_buffer(void const* buffer, size_t length, enj_error** err)
{
    if (!buffer || !length)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_elf* elf = enj_malloc(sizeof(enj_elf));
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    if (!(elf->blob = enj_blob_create(err)) ||
        enj_blob_insert(elf->blob, 0, buffer, length, err) < 0)
    {
        enj_free(elf);
        return 0;
    }

    if (enj_elf_pull(elf, err) < 0)
        return 0;

    return elf;
}

void enj_elf_delete(enj_elf* elf)
{
    if (!elf)
        return;

    for (enj_elf_shdr* section = elf->sections; section; )
    {
        enj_elf_shdr* next = section->next;
        enj_elf__shdr_delete(section, 0);
        section = next;
    }

    enj_blob_delete(elf->blob);
    enj_free(elf);
}

int enj_elf_pull(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (enj_elf_pull_header(elf, err) < 0)
        return -1;

    if (enj_elf_pull_sections(elf, err) < 0)
        return -1;

    if (enj_elf_pull_contents(elf, err) < 0)
        return -1;

    if (enj_elf_pull_segments(elf, err) < 0)
        return -1;

    return 0;
}

int enj_elf_update(enj_elf* elf, enj_error** err)
{

    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (enj_elf_update_header(elf, err) < 0)
        return -1;

    if (enj_elf_update_sections(elf, err) < 0)
        return -1;

    if (enj_elf_update_contents(elf, err) < 0)
        return -1;

    if (enj_elf_update_segments(elf, err) < 0)
        return -1;

    if (enj_elf_build_traits(elf, err) < 0)
        return -1;

    return 0;
}

int enj_elf_push(enj_elf* elf, enj_error** err)
{

    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (enj_elf_push_segments(elf, err) < 0)
        return -1;

    if (enj_elf_push_contents(elf, err) < 0)
        return -1;

    if (enj_elf_push_sections(elf, err) < 0)
        return -1;

    if (enj_elf_push_header(elf, err) < 0)
        return -1;

    return 0;
}


int enj_elf_pull_header(enj_elf* elf, enj_error** err)
{
    return enj_elf_update_header(elf, err);
}

int enj_elf_update_header(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    unsigned char e_ident[EI_NIDENT];
    if (enj_blob_read(elf->blob, 0, &e_ident[0], sizeof(e_ident), err) < 0)
    {
        enj_error_wrap(err, ENJ_ERR_BADHDR, *err);
        return -1;
    }

    if (e_ident[EI_MAG0] != ELFMAG0 ||
        e_ident[EI_MAG1] != ELFMAG1 ||
        e_ident[EI_MAG2] != ELFMAG2 ||
        e_ident[EI_MAG3] != ELFMAG3)
    {
        enj_error_put(err, ENJ_ERR_BADHDR);
        return -1;
    }

    if (e_ident[EI_CLASS] == ELFCLASS32)
    {
        elf->bits = 32;

        if (enj_blob_read(elf->blob, 0, &elf->ehdr32, sizeof(Elf32_Ehdr), err) < 0)
            return -1;
    }
    else if ((e_ident[EI_CLASS == ELFCLASS64]))
    {
        elf->bits = 64;

        if (enj_blob_read(elf->blob, 0, &elf->ehdr64, sizeof(Elf64_Ehdr), err) < 0)
            return -1;
    }
    else
    {
        enj_error_put(err, ENJ_ERR_BADHDR);
        return -1;
    }

    if (elf->sht)
    {
        if (enj_blob_remove_cursor(elf->blob, elf->sht, err) < 0)
            return -1;
    }

    if (elf->pht)
    {
        if (enj_blob_remove_cursor(elf->blob, elf->pht, err) < 0)
            return -1;
    }

    // Create section header table cursor
    size_t sht_pos = ENJ_ELF_EHDR_GET(elf, e_shoff);
    size_t sht_length = ENJ_ELF_EHDR_GET(elf, e_shnum) * ENJ_ELF_EHDR_GET(elf, e_shentsize);

    if (!(elf->sht = enj_blob_new_cursor(elf->blob, sht_pos, sht_length, err)))
        return -1;

    // Create program header table cursor
    size_t pht_pos = ENJ_ELF_EHDR_GET(elf, e_phoff);
    size_t pht_length = ENJ_ELF_EHDR_GET(elf, e_phnum) * ENJ_ELF_EHDR_GET(elf, e_phentsize);

    if (!(elf->pht = enj_blob_new_cursor(elf->blob, pht_pos, pht_length, err)))
        return -1;

    return 0;
}

int enj_elf_push_header(enj_elf* elf, enj_error** err)
{
    if (!elf || !elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Update program header table offset
    ENJ_ELF_EHDR_SET(elf, e_phoff, elf->pht->start->pos);

    // Update program header count
    size_t phentsize = ENJ_ELF_EHDR_GET(elf, e_phentsize);
    size_t num_segments = phentsize ? elf->pht->length / phentsize : 0;
    ENJ_ELF_EHDR_SET(elf, e_phnum, num_segments);

    // Update section header table offset
    ENJ_ELF_EHDR_SET(elf, e_shoff, elf->sht->start->pos);

    // Update section header count
    size_t shentsize = ENJ_ELF_EHDR_GET(elf, e_shentsize);
    size_t num_sections = shentsize ? elf->sht->length / shentsize : 0;
    ENJ_ELF_EHDR_SET(elf, e_shnum, num_sections);

    // Eventually update .shstrtab index
    if (elf->shstrtab)
    {
        size_t shoff = elf->sht->start->pos;
        size_t off = elf->shstrtab->header->pos;
        size_t index = shentsize ? (off - shoff) / shentsize : 0;
        ENJ_ELF_EHDR_SET(elf, e_shstrndx, index);
    }

    if (enj_elf_write_header(elf, err) < 0)
        return 0;

    return 0;
}

int enj_elf_write_header(enj_elf* elf, enj_error** err)
{
    if (!elf || !elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (elf->bits == 32)
    {
        if (enj_blob_write(elf->blob, 0, &elf->ehdr32, sizeof(Elf32_Ehdr), err) < 0)
            return -1;
    }
    else if (elf->bits == 64)
    {
        if (enj_blob_write(elf->blob, 0, &elf->ehdr64, sizeof(Elf64_Ehdr), err) < 0)
            return -1;
    }

    return 0;
}

int enj_elf_pull_sections(enj_elf* elf, enj_error** err)
{
    if (!elf || !elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Delete existing sections, remove anchors
    for (enj_elf_shdr* section = elf->sections; section; )
    {
        enj_elf_shdr* next = section->next;
        if (enj_elf__shdr_delete(section, err) < 0)
            return -1;
        section = next;
    }

    elf->sections = 0;
    elf->last_section = 0;
    elf->shstrtab = 0;

    // Get section headers
    size_t offset = ENJ_ELF_EHDR_GET(elf, e_shoff);
    size_t entsize = ENJ_ELF_EHDR_GET(elf, e_shentsize);
    size_t count = ENJ_ELF_EHDR_GET(elf, e_shnum);
    size_t shstrndx = ENJ_ELF_EHDR_GET(elf, e_shstrndx);

    if (count)
    {
        size_t i = shstrndx;
        int skip = 0;
        do
        {
            // Allocate descriptor
            enj_elf_shdr* section = enj_malloc(sizeof(enj_elf_shdr));
            if (!section)
            {
                enj_error_put(err, ENJ_ERR_MALLOC);
                return -1;
            }

            section->elf = elf;
            section->index = i;

            if (shstrndx && shstrndx == i)
                elf->shstrtab = section;

            // Setup header anchor
            section->header = enj_blob_new_anchor(elf->blob, offset + i * entsize, err);
            section->data = 0;

            // Pull section contents
            if (!section->header ||
                enj_elf_shdr_pull(section, err) < 0)
            {
                enj_blob_remove_anchor(elf->blob, section->header, 0);
                enj_free(section);
                return -1;
            }

            // Insert the descriptor into the linked list
            section->prev = elf->last_section;
            section->next = 0;
            if (section->prev)
                section->prev->next = section;
            else
                elf->sections = section;
            elf->last_section = section;

            if (shstrndx && shstrndx == i)
            {
                i = 0;
                skip = shstrndx;
                shstrndx = 0;
            }
            else
                ++i;

            if (skip && i == skip)
                ++i;
        } while (i < count);
    }

    return 0;
}

int enj_elf_update_sections(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        if (enj_elf_shdr_update(section, err) < 0)
            return -1;
    }

    return 0;
}

int enj_elf_push_sections(enj_elf* elf, enj_error** err)
{
    if (!elf || !elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        if (enj_elf_shdr_push(section, err) < 0)
            return -1;
    }

    return 0;
}

int enj_elf_pull_contents(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        if (section->content_view && section->content_view->o_pull &&
           (*section->content_view->o_pull)(section, err) < 0)
        {
            return -1;
        }
    }

    return 0;
}

int enj_elf_update_contents(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        if (section->content_view && section->content_view->o_update &&
            section->content &&
           (*section->content_view->o_update)(section, err) < 0)
        {
            return -1;
        }
    }

    return 0;
}

int enj_elf_push_contents(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        if (section->content_view && section->content_view->o_push &&
            section->content &&
           (*section->content_view->o_push)(section, err) < 0)
        {
            return -1;
        }
    }

    return 0;
}

int enj_elf_pull_segments(enj_elf* elf, enj_error** err)
{
    if (!elf || !elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Delete existing segments, remove anchors
    for (enj_elf_phdr* segment = elf->segments; segment; )
    {
        enj_elf_phdr* next = segment->next;
        if (enj_elf__phdr_delete(segment, err) < 0)
            return -1;
        segment = next;
    }

    elf->segments = 0;
    elf->last_segment = 0;

    // Get program headers
    size_t offset = ENJ_ELF_EHDR_GET(elf, e_phoff);
    size_t entsize = ENJ_ELF_EHDR_GET(elf, e_phentsize);
    size_t count = ENJ_ELF_EHDR_GET(elf, e_phnum);

    for (size_t i = 0; i < count; ++i)
    {
        enj_elf_phdr* segment = enj_malloc(sizeof(enj_elf_phdr));
        if (!segment)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            return -1;
        }

        segment->elf = elf;
        segment->index = i;

        segment->header = enj_blob_new_anchor(elf->blob, offset + i * entsize, err);
        segment->data = 0;

        if (!segment->header ||
            enj_elf_phdr_pull(segment, err) < 0)
        {
            enj_blob_remove_anchor(elf->blob, segment->header, 0);
            enj_free(segment);
            return -1;
        }

        segment->prev = elf->last_segment;
        segment->next = 0;
        if (segment->prev)
            segment->prev->next = segment;
        else
            elf->segments = segment;
        elf->last_segment = segment;
    }

    return 0;
}

int enj_elf_update_segments(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_elf_phdr* segment = elf->segments; segment; segment = segment->next)
    {
        if (enj_elf_phdr_update(segment, err) < 0)
            return -1;
    }

    return 0;
}

int enj_elf_push_segments(enj_elf* elf, enj_error** err)
{
    if (!elf || !elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_elf_phdr* segment = elf->segments; segment; segment = segment->next)
    {
        if (enj_elf_phdr_push(segment, err) < 0)
            return -1;
    }

    return 0;
}

int enj_elf_build_traits(enj_elf* elf, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    for (enj_elf_trait* trait = elf->traits; trait; trait = trait->next)
    {
        if (trait->o_build &&
           (*trait->o_build)(trait->arg, err) < 0)
        {
            return -1;
        }
    }

    return 0;
}

enj_elf_shdr* enj_elf_find_shdr_by_index(enj_elf* elf, size_t index, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        if (section->index == index)
            return section;
    }

    return 0;
}

enj_elf_shdr* enj_elf_find_shdr_by_name(enj_elf* elf, const char* name, enj_error** err)
{
    if (!elf || !name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_fstring_hash_t hash = enj_fstring_hash(name);

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        if (!section->cached_name || section->cached_name->hash != hash)
            continue;

        if (!strncmp(section->cached_name->string, name, section->cached_name->length))
            return section;
    }

    return 0;
}

enj_elf_shdr* enj_elf_new_shdr(enj_elf* elf, int type, const char* name, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    // Get new section index
    size_t index = 0;
    for (enj_elf_shdr* section = elf->sections; section/* && section->next*/; section = section->next)
        ++index;

    // Allocate descriptor
    enj_elf_shdr* section = enj_malloc(sizeof(enj_elf_shdr));
    if (!section)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    section->elf = elf;
    section->index = index;

    // Setup section type
    ENJ_ELF_SHDR_SET(section, sh_type, type);

    // Insert section name if provided
    if (name)
    {
        if (!elf->shstrtab)
        {
            enj_error_put(err, ENJ_ERR_NO_STRTAB);
            enj_free(section);
            return 0;
        }

        if (!elf->shstrtab->data)
        {
            enj_error_put(err, ENJ_ERR_BAD_STRTAB);
            enj_free(section);
            return 0;
        }

        size_t name_len = strlen(name) + 1;
        size_t name_pos = elf->shstrtab->data->end->pos;
        enj_blob_insert(elf->blob, name_pos, name, name_len, err);

        ENJ_ELF_SHDR_SET(section, sh_name, name_pos - elf->shstrtab->data->start->pos);
    }

    // Setup name anchor, if applicable
    if (elf->shstrtab)
    {
        if (!(section->name = enj_blob_new_anchor(elf->blob, elf->shstrtab->data->start->pos + ENJ_ELF_SHDR_GET(section, sh_name), err)))
        {
            enj_elf__shdr_delete(section, err);
            return 0;
        }
    }

    // No data for now
    section->data = 0;

    // Section header offset
    size_t shdr_pos = elf->sht->end->pos;

    // Insert new section header
    if (elf->bits == 64)
    {
        if (enj_blob_insert(elf->blob, shdr_pos-1, &section->shdr64, sizeof(Elf64_Shdr), err) < 0)
        {
            enj_elf__shdr_delete(section, err);
            return 0;
        }
    }
    else if (elf->bits == 32)
    {
        if (enj_blob_insert(elf->blob, shdr_pos-1, &section->shdr32, sizeof(Elf32_Shdr), err) < 0)
        {
            enj_elf__shdr_delete(section, err);
            return 0;
        }
    }

    // Setup header anchor
    if (!(section->header = enj_blob_new_anchor(elf->blob, shdr_pos, err)))
    {
        enj_elf__shdr_delete(section, err);
        return 0;
    }

    // Update section contents
    if (enj_elf_shdr_update(section, err) < 0)
    {
        enj_elf__shdr_delete(section, err);
        return 0;
    }

    // Insert the descriptor into the linked list
    section->prev = elf->last_section;
    section->next = 0;
    if (section->prev)
        section->prev->next = section;
    else
        elf->sections = section;
    elf->last_section = section;

    return section;
}

enj_elf_phdr* enj_elf_find_phdr_by_index(enj_elf* elf, size_t index, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    for (enj_elf_phdr* segment = elf->segments; segment; segment = segment->next)
    {
        if (segment->index == index)
            return segment;
    }

    return 0;
}

enj_elf_phdr* enj_elf_new_phdr(enj_elf* elf, int type, enj_error** err)
{
    if (!elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    // Get new segment index
    size_t index = 0;
    for (enj_elf_phdr* segment = elf->segments; segment/* && segment->next*/; segment = segment->next)
        ++index;

    // Allocate descriptor
    enj_elf_phdr* segment = enj_malloc(sizeof(enj_elf_phdr));
    if (!segment)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    segment->elf = elf;
    segment->index = index;

    // Setup segment type
    ENJ_ELF_PHDR_SET(segment, p_type, type);

    // No data for now
    segment->data = 0;

    // Program header offset
    size_t phdr_pos = elf->pht->end->pos;

    // Insert new program header
    if (elf->bits == 64)
    {
        if (enj_blob_insert(elf->blob, phdr_pos-1, &segment->phdr64, sizeof(Elf64_Phdr), err) < 0)
        {
            enj_elf__phdr_delete(segment, err);
            return 0;
        }
    }
    else if (elf->bits == 32)
    {
        if (enj_blob_insert(elf->blob, phdr_pos-1, &segment->phdr32, sizeof(Elf32_Phdr), err) < 0)
        {
            enj_elf__phdr_delete(segment, err);
            return 0;
        }
    }

    // Setup header anchor
    if (!(segment->header = enj_blob_new_anchor(elf->blob, phdr_pos, err)))
    {
        enj_elf__phdr_delete(segment, err);
        return 0;
    }

    // Update segment
    if (enj_elf_phdr_update(segment, err) < 0)
    {
        enj_elf__phdr_delete(segment, err);
        return 0;
    }

    // Insert the descriptor into the linked list
    segment->prev = elf->last_segment;
    segment->next = 0;
    if (segment->prev)
        segment->prev->next = segment;
    else
        elf->segments = segment;
    elf->last_segment = segment;

    return segment;
}

int enj_elf_shdr_pull(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->elf || !section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Read section header
    if (section->elf->bits == 32)
    {
        if (enj_blob_read(section->elf->blob, section->header->pos, &section->shdr32, sizeof(Elf32_Shdr), err) < 0)
            return -1;
    }
    else if (section->elf->bits == 64)
    {
        if (enj_blob_read(section->elf->blob, section->header->pos, &section->shdr64, sizeof(Elf64_Shdr), err) < 0)
            return -1;
    }

    if (section->data)
    {
        if (enj_blob_remove_cursor(section->elf->blob, section->data, err) < 0)
            return -1;
    }

    size_t shtype = ENJ_ELF_SHDR_GET(section, sh_type);

    if (shtype != SHT_NOBITS)
    {
        size_t offset = ENJ_ELF_SHDR_GET(section, sh_offset);
        size_t size = ENJ_ELF_SHDR_GET(section, sh_size);

        if (!(section->data = enj_blob_new_cursor(section->elf->blob, offset, size, err)))
            return -1;
    }

    if (section->name)
    {
        if (enj_blob_remove_anchor(section->elf->blob, section->name, err) < 0)
            return -1;
    }

    size_t name = ENJ_ELF_SHDR_GET(section, sh_name);
    size_t name_off = section->elf->shstrtab->data->start->pos + name;

    if (!(section->name = enj_blob_new_anchor(section->elf->blob, name_off, err)))
        return -1;

    if (enj_elf_shdr_update(section, err) < 0)
        return -1;

    return 0;
}

int enj_elf_shdr_update(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Update section name
    if (section->elf->shstrtab)
    {
        size_t name_off = section->name->pos;

        if (section->cached_name)
        {
            enj_fstring_delete(section->cached_name);
            section->cached_name = 0;
        }

        int name_ok = 1;
        size_t name_len = 0;
        char c;
        do
        {
            if (enj_blob_read(section->elf->blob, name_off + name_len, &c, 1, err) < 0)
            {
                name_ok = 0;
                break;
            }

            ++name_len;
        } while (c != '\0');

        if (name_ok)
        {
            char buffer[name_len];
            if (enj_blob_read(section->elf->blob, name_off, &buffer[0], name_len, err) < 0)
                return -1;

            section->cached_name = enj_fstring_create(&buffer[0], err);
            if (!section->cached_name)
                return -1;
        }
    }
    else
        section->name = 0;

    size_t shtype = ENJ_ELF_SHDR_GET(section, sh_type);

    // Update content view if the section type changed
    if (!section->content_view || (section->content_view && section->content_view->target_shtype != shtype))
    {
        if (section->content_view && section->content_view->o_delete &&
            (*section->content_view->o_delete)(section, err) < 0)
            return -1;

        section->content_view = 0;
        section->content = 0;

        if (enj_elf__get_content_view(section, &section->content_view, err) < 0)
            return -1;
    }

    return 0;
}

int enj_elf_shdr_push(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->elf || !section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Update data pointers
    if (section->data)
    {
        ENJ_ELF_SHDR_SET(section, sh_offset, section->data->start->pos);
        ENJ_ELF_SHDR_SET(section, sh_size, section->data->length);
    }

    // Update name pointer
    if (section->name && section->elf->shstrtab && section->elf->shstrtab->data)
        ENJ_ELF_SHDR_SET(section, sh_name, section->name->pos - section->elf->shstrtab->data->start->pos);

    if (enj_elf_shdr_write(section, err) < 0)
        return -1;

    return 0;
}

int enj_elf_shdr_write(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->elf || !section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (section->elf->bits == 32)
    {
        if (enj_blob_write(section->elf->blob, section->header->pos, &section->shdr32, sizeof(Elf32_Shdr), err) < 0)
            return -1;
    }
    else if (section->elf->bits == 64)
    {
        if (enj_blob_write(section->elf->blob, section->header->pos, &section->shdr64, sizeof(Elf64_Shdr), err) < 0)
            return -1;
    }

    return 0;
}

int enj_elf_shdr_rename(enj_elf_shdr* section, const char* name, enj_error** err)
{
    if (!section || !section->elf || !name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!section->elf->shstrtab)
    {
        enj_error_put(err, ENJ_ERR_NO_STRTAB);
        return -1;
    }

    if (!section->elf->shstrtab->data)
    {
        enj_error_put(err, ENJ_ERR_BAD_STRTAB);
        return -1;
    }

    // If the section had an empty name, relocate it to the end of the .shstrtab
    //  to avoid making 0 a valid name index, as other sections may already use it
    if (section->name->pos == section->elf->shstrtab->data->start->pos)
    {
        section->name->pos = section->elf->shstrtab->data->end->pos;
    }

    size_t name_len = strlen(name) + 1;
    size_t old_name_len = section->cached_name ? section->cached_name->length : 0;

    if (enj_blob_insert(section->elf->blob, section->name->pos, name, name_len, err) < 0 ||
        enj_blob_remove(section->elf->blob, section->name->pos + name_len, old_name_len, err) < 0 ||
        enj_elf_shdr_pull(section, err) < 0)
        return -1;

    return 0;
}

int enj_elf_shdr_remove(enj_elf_shdr* section, int mode, enj_error** err)
{
    if (!section || !section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // First of all, take care of the section name
    if (section->elf->shstrtab && section->cached_name)
    {
        size_t name_len = section->cached_name->length;

        if (mode & ENJ_ELF_CLEAR_NAME)
        {
            if (enj_blob_set(section->elf->blob, section->name->pos, 0, name_len, err) < 0)
                return -1;
        }
        else if (mode & ENJ_ELF_DISCARD_NAME)
        {
            if (enj_blob_remove(section->elf->blob, section->name->pos, name_len, err) < 0)
                return -1;
        }
    }

    // Then of the section contents
    if (mode & ENJ_ELF_CLEAR_DATA)
    {
        if (enj_blob_set(section->elf->blob, section->data->start->pos, 0, section->data->length, err) < 0)
            return -1;
    }
    else if (mode & ENJ_ELF_DISCARD_DATA)
    {
        if (enj_blob_remove(section->elf->blob, section->data->start->pos, section->data->length, err) < 0)
            return -1;
    }

    // Shift other section indices, as well as eventual links
    for (enj_elf_shdr* other = section->elf->sections; other; other = other->next)
    {
        if (other->index <= section->index)
            continue;

        if (other->index)
            --other->index;

        size_t link = ENJ_ELF_SHDR_GET(other, sh_link);

        if (link == section->index)
            link = 0;
        else if (link && link > section->index)
            --link;

        ENJ_ELF_SHDR_SET(other, sh_link, link);

        // Also update sh_info if it contains a section index
        size_t flags = ENJ_ELF_SHDR_GET(other, sh_flags);
        if (flags & SHF_INFO_LINK)
        {
            size_t info = ENJ_ELF_SHDR_GET(other, sh_info);

            if (info == section->index)
                info = 0;
            else if (info && info > section->index)
                --info;

            ENJ_ELF_SHDR_SET(other, sh_info, info);
        }
    }

    // Eventually shift ElfXX_Ehdr.e_shstrndx
    size_t shstrndx = ENJ_ELF_EHDR_GET(section->elf, e_shstrndx);
    if (shstrndx == section->index)
        shstrndx = 0;
    else if (shstrndx && shstrndx > section->index)
        --shstrndx;
    ENJ_ELF_EHDR_SET(section->elf, e_shstrndx, shstrndx);

    // Now, remove the section header
    if (enj_blob_remove(section->elf->blob, section->header->pos, ENJ_ELF_EHDR_GET(section->elf, e_shentsize), err) < 0)
        return -1;

    // Remove the section descriptor from the list
    if (section->prev)
        section->prev->next = section->next;
    else
        section->elf->sections = section->next;

    if (section->next)
        section->next->prev = section->prev;
    else
        section->elf->last_section = section->prev;

    if (section == section->elf->shstrtab)
    {
        section->elf->shstrtab = 0;
    }

    // Release descriptor memory
    if (enj_elf__shdr_delete(section, err) < 0)
        return -1;

    return 0;
}

int enj_elf_shdr_swap(enj_elf_shdr* section, int index, enj_error** err)
{
    if (!section || !section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf_shdr* other = enj_elf_find_shdr_by_index(section->elf, index, err);
    if (!other || other == section)
    {
        if (!*err)
            enj_error_put(err, ENJ_ERR_BAD_INDEX);
        return -1;
    }

    // Swap section header anchors
    enj_blob_anchor* header = section->header;
    section->header = other->header;
    other->header = header;

    // Update section links
    for (enj_elf_shdr* other2 = section->elf->sections; other2; other2 = other2->next)
    {
        size_t link = ENJ_ELF_SHDR_GET(other2, sh_link);

        if (link == other->index)
            link = section->index;
        else if (link == section->index)
            link = other->index;

        ENJ_ELF_SHDR_SET(other2, sh_link, link);

        // Also update sh_info if it contains a section index
        size_t flags = ENJ_ELF_SHDR_GET(other2, sh_flags);
        if (flags & SHF_INFO_LINK)
        {
            size_t info = ENJ_ELF_SHDR_GET(other2, sh_info);

            if (info == other->index)
                info = section->index;
            else if (info == section->index)
                info = other->index;

            ENJ_ELF_SHDR_SET(other2, sh_info, info);
        }
    }

    // Swap section indices
    other->index = section->index;
    section->index = index;

    return 0;
}

int enj_elf_shdr_move(enj_elf_shdr* section, int index, enj_error** err)
{
    if (!section || !section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!enj_elf_find_shdr_by_index(section->elf, index, err) || index == section->index)
    {
        if (!*err)
            enj_error_put(err, ENJ_ERR_BAD_INDEX);
        return -1;
    }

    size_t start_index = section->index;

    enj_elf_shdr_swap(section, index, err);

    if (index < start_index)
    {
        for (size_t i = start_index; i > (index + 1); --i)
        {
            enj_elf_shdr* other = enj_elf_find_shdr_by_index(section->elf, i, err);
            enj_elf_shdr_swap(other, i - 1, err);
        }
    }
    else if (index > start_index)
    {

        for (size_t i = start_index; i < (index - 1); ++i)
        {
            enj_elf_shdr* other = enj_elf_find_shdr_by_index(section->elf, i, err);
            enj_elf_shdr_swap(other, i + 1, err);
        }
    }

    return 0;
}

int enj_elf_phdr_pull(enj_elf_phdr* segment, enj_error** err)
{
    if (!segment || !segment->elf || !segment->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Read segment header
    if (segment->elf->bits == 32)
    {
        if (enj_blob_read(segment->elf->blob, segment->header->pos, &segment->phdr32, sizeof(Elf32_Phdr), err) < 0)
            return -1;
    }
    else if (segment->elf->bits == 64)
    {
        if (enj_blob_read(segment->elf->blob, segment->header->pos, &segment->phdr64, sizeof(Elf64_Phdr), err) < 0)
            return -1;
    }

    if (segment->data)
    {
        if (enj_blob_remove_cursor(segment->elf->blob, segment->data, err) < 0)
            return -1;
    }

    size_t offset = ENJ_ELF_PHDR_GET(segment, p_offset);
    size_t size = ENJ_ELF_PHDR_GET(segment, p_filesz);

    if (!(segment->data = enj_blob_new_cursor(segment->elf->blob, offset, size, err)))
        return -1;

    return 0;
}

int enj_elf_phdr_update(enj_elf_phdr* segment, enj_error** err)
{
    if (!segment || !segment->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    return 0;
}

int enj_elf_phdr_push(enj_elf_phdr* segment, enj_error** err)
{
    if (!segment || !segment->elf || !segment->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Update data pointers
    if (segment->data)
    {
        size_t old_filesz = ENJ_ELF_PHDR_GET(segment, p_filesz);
        size_t old_memsz = ENJ_ELF_PHDR_GET(segment, p_memsz);
        size_t new_filesz = segment->data->length;
        size_t new_memsz = old_memsz;

        if (new_filesz < old_filesz)
            new_memsz = old_memsz - (old_filesz - new_filesz);
        else if (new_filesz > old_filesz)
            new_memsz = old_memsz + (new_filesz - old_filesz);

        ENJ_ELF_PHDR_SET(segment, p_offset, segment->data->start->pos);
        ENJ_ELF_PHDR_SET(segment, p_filesz, new_filesz);
        ENJ_ELF_PHDR_SET(segment, p_memsz, new_memsz);
    }

    // Then, write back the updated header
    if (segment->elf->bits == 32)
    {
        if (enj_blob_write(segment->elf->blob, segment->header->pos, &segment->phdr32, sizeof(Elf32_Phdr), err) < 0)
            return -1;
    }
    else if (segment->elf->bits == 64)
    {
        if (enj_blob_write(segment->elf->blob, segment->header->pos, &segment->phdr64, sizeof(Elf64_Phdr), err) < 0)
            return -1;
    }

    if (enj_elf_phdr_write(segment, err) < 0)
        return -1;

    return 0;
}

int enj_elf_phdr_write(enj_elf_phdr* segment, enj_error** err)
{
    if (!segment || !segment->elf || !segment->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (segment->elf->bits == 32)
    {
        if (enj_blob_write(segment->elf->blob, segment->header->pos, &segment->phdr32, sizeof(Elf32_Phdr), err) < 0)
            return -1;
    }
    else if (segment->elf->bits == 64)
    {
        if (enj_blob_write(segment->elf->blob, segment->header->pos, &segment->phdr64, sizeof(Elf64_Phdr), err) < 0)
            return -1;
    }

    return 0;
}

int enj_elf_phdr_remove(enj_elf_phdr* segment, enj_error** err)
{
    if (!segment || !segment->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    // Shift other segment indices
    for (enj_elf_phdr* other = segment->elf->segments; other; other = other->next)
    {
        if (other->index <= segment->index)
            continue;

        if (other->index)
            --other->index;
    }

    // Now, remove the program header
    if (enj_blob_remove(segment->elf->blob, segment->header->pos, ENJ_ELF_EHDR_GET(segment->elf, e_phentsize), err) < 0)
        return -1;

    // Remove the segment descriptor from the list
    if (segment->prev)
        segment->prev->next = segment->next;
    else
        segment->elf->segments = segment->next;

    if (segment->next)
        segment->next->prev = segment->prev;
    else
        segment->elf->last_segment = segment->prev;

    // Release descriptor memory
    if (enj_elf__phdr_delete(segment, err) < 0)
        return -1;

    return 0;
}

int enj_elf_phdr_swap(enj_elf_phdr* segment, int index, enj_error** err)
{
    if (!segment || !segment->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf_phdr* other = enj_elf_find_phdr_by_index(segment->elf, index, err);
    if (!other || other == segment)
    {
        if (!*err)
            enj_error_put(err, ENJ_ERR_BAD_INDEX);
        return -1;
    }

    // Swap phdr header anchors
    enj_blob_anchor* header = segment->header;
    segment->header = other->header;
    other->header = header;

    // Swap phdr indices
    other->index = segment->index;
    segment->index = index;

    return 0;
}

int enj_elf_phdr_move(enj_elf_phdr* segment, int index, enj_error** err)
{
    if (!segment || !segment->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (!enj_elf_find_shdr_by_index(segment->elf, index, err) || index == segment->index)
    {
        if (!*err)
            enj_error_put(err, ENJ_ERR_BAD_INDEX);
        return -1;
    }

    size_t start_index = segment->index;

    enj_elf_phdr_swap(segment, index, err);

    if (index < start_index)
    {
        for (size_t i = start_index; i > (index + 1); --i)
        {
            enj_elf_phdr* other = enj_elf_find_phdr_by_index(segment->elf, i, err);
            enj_elf_phdr_swap(other, i - 1, err);
        }
    }
    else if (index > start_index)
    {

        for (size_t i = start_index; i < (index - 1); ++i)
        {
            enj_elf_phdr* other = enj_elf_find_phdr_by_index(segment->elf, i, err);
            enj_elf_phdr_swap(other, i + 1, err);
        }
    }

    return 0;
}

int enj_elf_add_trait(enj_elf* elf, enj_elf_trait* trait, enj_error** err)
{
    if (!elf || !trait)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    trait->elf = elf;

    trait->prev = elf->last_trait;
    trait->next = 0;
    if (trait->prev)
        trait->prev->next = trait;
    else
        elf->traits = trait;
    elf->last_trait = trait;

    return 0;
}

int enj_elf_trait_remove(enj_elf_trait* trait, enj_error** err)
{
    if (!trait || !trait->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (trait->prev)
        trait->prev->next = trait->next;
    else
        trait->elf->traits = trait->next;

    if (trait->next)
        trait->next->prev = trait->prev;
    else
        trait->elf->last_trait = trait->prev;

    trait->elf = 0;

    return 0;
}

int enj_elf__shdr_delete(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if (section->content_view && section->content_view->o_delete &&
        section->content &&
        (*section->content_view->o_delete)(section, err) < 0)
        return -1;

    if ((section->header && enj_blob_remove_anchor(section->elf->blob, section->header, err) < 0) ||
        (section->name && enj_blob_remove_anchor(section->elf->blob, section->name, err) < 0) ||
        (section->data && enj_blob_remove_cursor(section->elf->blob, section->data, err) < 0))
    {
        return -1;
    }

    if (section->cached_name)
        enj_fstring_delete(section->cached_name);

    enj_free(section);

    return 0;
}

int enj_elf__phdr_delete(enj_elf_phdr* segment, enj_error** err)
{
    if (!segment || !segment->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    if ((segment->header && enj_blob_remove_anchor(segment->elf->blob, segment->header, err) < 0) ||
        (segment->data && enj_blob_remove_cursor(segment->elf->blob, segment->data, err) < 0))
    {
        return -1;
    }

    enj_free(segment);

    return 0;
}

int enj_elf__get_content_view(enj_elf_shdr* section, enj_elf_content_view** view, enj_error** err)
{
    if (!section || !view)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    int(*user)(enj_elf_shdr*, enj_elf_content_view**, enj_error**) = 0;
    while ((user = dlsym(RTLD_NEXT, "enj_elf_get_user_content_view")))
    {
        if ((*user)(section, view, err) < 0)
            return -1;
        else if (*view)
            return 0;
    }

    static enj_elf_content_view symtab =
    {
        ENJ_ELF_SYMTAB,
        SHT_SYMTAB,
        &enj_symtab__pull,
        &enj_symtab__update,
        &enj_symtab__push,
        &enj_symtab__delete
    };

    static enj_elf_content_view dynsym =
    {
        ENJ_ELF_SYMTAB,
        SHT_DYNSYM,
        &enj_symtab__pull,
        &enj_symtab__update,
        &enj_symtab__push,
        &enj_symtab__delete
    };

    static enj_elf_content_view nsect =
    {
        ENJ_ELF_NOTE,
        SHT_NOTE,
        &enj_nsect__pull,
        &enj_nsect__update,
        &enj_nsect__push,
        &enj_nsect__delete
    };

    static enj_elf_content_view dynamic =
    {
        ENJ_ELF_DYNAMIC,
        SHT_DYNAMIC,
        &enj_dynamic__pull,
        &enj_dynamic__update,
        &enj_dynamic__push,
        &enj_dynamic__delete
    };

    size_t sh_type = ENJ_ELF_SHDR_GET(section, sh_type);

    switch (sh_type)
    {
        case SHT_SYMTAB:
            *view = &symtab;
            return 0;

        case SHT_DYNSYM:
            *view = &dynsym;
            return 0;

        case SHT_NOTE:
            *view = &nsect;
            return 0;

        case SHT_DYNAMIC:
            *view = &dynamic;
            return 0;
    }

    *view = 0;
    return 0;
}
