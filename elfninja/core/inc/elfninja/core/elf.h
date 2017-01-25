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

#ifndef __ELFNINJA_CORE_ELF_H__
#define __ELFNINJA_CORE_ELF_H__

#include "elfninja/core/error.h"
#include "elfninja/core/blob.h"
#include "elfninja/core/fstring.h"

#include <elf.h>

struct enj_elf;
struct enj_elf_shdr;
struct enj_elf_content_view;
struct enj_elf_trait;
struct enj_elf_phdr;

typedef struct enj_elf
{
    enj_blob* blob;

    enj_blob_cursor* sht;
    enj_blob_cursor* pht;

    int bits;
    union
    {
        Elf32_Ehdr ehdr32;
        Elf64_Ehdr ehdr64;
    };

    struct enj_elf_shdr* sections;
    struct enj_elf_shdr* last_section;

    struct enj_elf_phdr* segments;
    struct enj_elf_phdr* last_segment;

    struct enj_elf_trait* traits;
    struct enj_elf_trait* last_trait;

    struct enj_elf_shdr* shstrtab;
} enj_elf;

typedef struct enj_elf_shdr
{
    enj_elf* elf;

    size_t index;
    enj_fstring* cached_name;
    struct enj_elf_content_view* content_view;
    void* content;

    enj_blob_anchor* header;
    enj_blob_anchor* name;
    enj_blob_cursor* data;

    union
    {
        Elf32_Shdr shdr32;
        Elf64_Shdr shdr64;
    };

    struct enj_elf_shdr* prev;
    struct enj_elf_shdr* next;
} enj_elf_shdr;

typedef struct enj_elf_phdr
{
    enj_elf* elf;

    size_t index;

    enj_blob_anchor* header;
    enj_blob_cursor* data;

    union
    {
        Elf32_Phdr phdr32;
        Elf64_Phdr phdr64;
    };

    struct enj_elf_phdr* prev;
    struct enj_elf_phdr* next;
} enj_elf_phdr;

enum
{
    ENJ_ELF_NONE = 0,

    #define DEF_TAG(name, id) ENJ_ELF_ ## name = (id),
    #include "elf_tags.def"

    ENJ_ELF_LOUSER
};

typedef struct enj_elf_content_view
{
    size_t tag;
    int target_shtype;
    int (*o_pull)(enj_elf_shdr* section, enj_error** err);
    int (*o_update)(enj_elf_shdr* section, enj_error** err);
    int (*o_push)(enj_elf_shdr* section, enj_error** err);
    int (*o_delete)(enj_elf_shdr* section, enj_error** err);
} enj_elf_content_view;

typedef struct enj_elf_trait
{
    enj_elf* elf;
    void* arg;
    int (*o_build)(void* arg, enj_error** err);

    struct enj_elf_trait* prev;
    struct enj_elf_trait* next;
} enj_elf_trait;

#define ENJ_ELF_EHDR_SIZE(elf) (elf->bits == 64 ? sizeof(Elf64_Shdr) : sizeof(Elf32_Ehdr))
#define ENJ_ELF_EHDR_GET(elf, field) (elf->bits == 64 ? elf->ehdr64.field : elf->ehdr32.field)
#define ENJ_ELF_EHDR_SET(elf, field, value) do {\
        if (elf->bits == 64) elf->ehdr64.field = (value); \
        else elf->ehdr32.field = (value); \
    } while (0);

#define ENJ_ELF_SHDR_SIZE(section) (section->elf->bits == 64 ? sizeof(Elf64_Shdr) : sizeof(Elf32_Shdr))
#define ENJ_ELF_SHDR_GET(section, field) (section->elf->bits == 64 ? section->shdr64.field : section->shdr32.field)
#define ENJ_ELF_SHDR_SET(section, field, value) do {\
        if (section->elf->bits == 64) section->shdr64.field = (value); \
        else section->shdr32.field = (value); \
    } while (0);

#define ENJ_ELF_PHDR_SIZE(segment) (segment->elf->bits == 64 ? sizeof(Elf64_Phdr) : sizeof(Elf32_Phdr))
#define ENJ_ELF_PHDR_GET(segment, field) (segment->elf->bits == 64 ? segment->phdr64.field : segment->phdr32.field)
#define ENJ_ELF_PHDR_SET(segment, field, value) do {\
        if (segment->elf->bits == 64) segment->phdr64.field = (value); \
        else segment->phdr32.field = (value); \
    } while (0);

enum
{
    ENJ_ELF_CLEAR_NAME   = 0x01,
    ENJ_ELF_DISCARD_NAME = 0x02,
    ENJ_ELF_CLEAR_DATA   = 0x04,
    ENJ_ELF_DISCARD_DATA = 0x08
};

enj_elf* enj_elf_create_fd(int fd, enj_error** err);
enj_elf* enj_elf_create_buffer(void const* buffer, size_t length, enj_error** err);
void enj_elf_delete(enj_elf* elf);

int enj_elf_pull(enj_elf* elf, enj_error** err);
int enj_elf_update(enj_elf* elf, enj_error** err);
int enj_elf_push(enj_elf* elf, enj_error** err);

int enj_elf_pull_header(enj_elf* elf, enj_error** err);
int enj_elf_update_header(enj_elf* elf, enj_error** err);
int enj_elf_push_header(enj_elf* elf, enj_error** err);
int enj_elf_write_header(enj_elf* elf, enj_error** err);

int enj_elf_pull_sections(enj_elf* elf, enj_error** err);
int enj_elf_update_sections(enj_elf* elf, enj_error** err);
int enj_elf_push_sections(enj_elf* elf, enj_error** err);

int enj_elf_pull_contents(enj_elf* elf, enj_error** err);
int enj_elf_update_contents(enj_elf* elf, enj_error** err);
int enj_elf_push_contents(enj_elf* elf, enj_error** err);

int enj_elf_pull_segments(enj_elf* elf, enj_error** err);
int enj_elf_update_segments(enj_elf* elf, enj_error** err);
int enj_elf_push_segments(enj_elf* elf, enj_error** err);

int enj_elf_build_traits(enj_elf* elf, enj_error** err);

enj_elf_shdr* enj_elf_find_shdr_by_index(enj_elf* elf, size_t index, enj_error** err);
enj_elf_shdr* enj_elf_find_shdr_by_name(enj_elf* elf, const char* name, enj_error** err);
enj_elf_shdr* enj_elf_new_shdr(enj_elf* elf, int type, const char* name, enj_error** err);

enj_elf_phdr* enj_elf_find_phdr_by_index(enj_elf* elf, size_t index, enj_error** err);
enj_elf_phdr* enj_elf_new_phdr(enj_elf* elf, int type, enj_error** err);

int enj_elf_shdr_pull(enj_elf_shdr* section, enj_error** err);
int enj_elf_shdr_update(enj_elf_shdr* section, enj_error** err);
int enj_elf_shdr_push(enj_elf_shdr* section, enj_error** err);
int enj_elf_shdr_write(enj_elf_shdr* section, enj_error** err);
int enj_elf_shdr_rename(enj_elf_shdr* section, const char* name, enj_error** err);
int enj_elf_shdr_remove(enj_elf_shdr* section, int mode, enj_error** err);
int enj_elf_shdr_swap(enj_elf_shdr* section, int index, enj_error** err);
int enj_elf_shdr_move(enj_elf_shdr* section, int index, enj_error** err);

int enj_elf_phdr_pull(enj_elf_phdr* segment, enj_error** err);
int enj_elf_phdr_update(enj_elf_phdr* segment, enj_error** err);
int enj_elf_phdr_push(enj_elf_phdr* segment, enj_error** err);
int enj_elf_phdr_write(enj_elf_phdr* segment, enj_error** err);
int enj_elf_phdr_remove(enj_elf_phdr* segment, enj_error** err);
int enj_elf_phdr_swap(enj_elf_phdr* segment, int index, enj_error** err);
int enj_elf_phdr_move(enj_elf_phdr* segment, int index, enj_error** err);

int enj_elf_add_trait(enj_elf* elf, enj_elf_trait* trait, enj_error** err);
int enj_elf_trait_remove(enj_elf_trait* trait, enj_error** err);

int enj_elf__shdr_delete(enj_elf_shdr* section, enj_error** err);
int enj_elf__phdr_delete(enj_elf_phdr* segment, enj_error** err);
int enj_elf__get_content_view(enj_elf_shdr* section, enj_elf_content_view** view, enj_error** err);

#endif // __ELFNINJA_CORE_ELF_H__
