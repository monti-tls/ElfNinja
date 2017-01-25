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

#include "elfninja/core/symtab.h"
#include "elfninja/core/malloc.h"
#include "elfninja/core/blob.h"

#include <string.h>

enj_symbol* enj_symtab_find_symbol(enj_symtab* symtab, const char* name, enj_error** err)
{
    if (!symtab || !name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_fstring_hash_t hash = enj_fstring_hash(name);

    for (enj_symbol* sym = symtab->symbols; sym; sym = sym->next)
    {
        if (!sym->cached_name || sym->cached_name->hash != hash)
            continue;

        if (!strncmp(sym->cached_name->string, name, sym->cached_name->length))
            return sym;
    }

    return 0;
}

enj_symbol* enj_symtab_new_symbol(enj_symtab* symtab, const char* name, enj_error** err)
{
    if (!symtab || !symtab->section->data)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    enj_elf* elf = symtab->section->elf;

    // Allocate descriptor
    enj_symbol* sym = enj_malloc(sizeof(enj_symbol));
    if (!sym)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return 0;
    }

    sym->symtab = symtab;

    // Add symbol name if provided
    if (name)
    {
        if (!symtab->strtab)
        {
            enj_error_put(err, ENJ_ERR_NO_STRTAB);
            enj_free(sym);
            return 0;
        }

        if (!symtab->strtab->data)
        {
            enj_error_put(err, ENJ_ERR_BAD_STRTAB);
            enj_free(sym);
            return 0;
        }

        size_t name_len = strlen(name) + 1;
        size_t name_pos = symtab->strtab->data->end->pos;
        enj_blob_insert(elf->blob, name_pos, name, name_len, err);

        ENJ_SYMBOL_SET(sym, st_name, name_pos - symtab->strtab->data->start->pos);
    }

    // Setup name anchor, if applicable
    if (symtab->strtab)
    {
        if (!(sym->name = enj_blob_new_anchor(elf->blob, symtab->strtab->data->start->pos + ENJ_SYMBOL_GET(sym, st_name), err)))
        {
            enj_symbol__delete(sym, err);
            return 0;
        }
    }

    // No target for now
    sym->target = 0;

    // Header offset
    size_t hdr_pos = symtab->section->data->end->pos;

    // Setup symbol index
    sym->index = (hdr_pos - symtab->section->data->start->pos) / ENJ_SYMBOL_SIZE(sym);

    // Insert new symbol header
    if (elf->bits == 32)
    {
        if (enj_blob_insert(elf->blob, hdr_pos, &sym->sym32, sizeof(Elf32_Sym), err) < 0)
        {
            enj_symbol__delete(sym, err);
            return 0;
        }
    }
    else if (elf->bits == 64)
    {
        if (enj_blob_insert(elf->blob, hdr_pos, &sym->sym64, sizeof(Elf64_Sym), err) < 0)
        {
            enj_symbol__delete(sym, err);
            return 0;
        }
    }

    // Setup header anchor
    if (!(sym->header = enj_blob_new_anchor(elf->blob, hdr_pos, err)))
    {
        enj_symbol__delete(sym, err);
        return 0;
    }

    // Update symbol
    if (enj_symbol_update(sym, err) < 0)
    {
        enj_symbol__delete(sym, err);
        return 0;
    }

    // Insert the descriptor into the linked list
    sym->prev = symtab->last_symbol;
    sym->next = 0;
    if (sym->prev)
        sym->prev->next = sym;
    else
        symtab->symbols = sym;
    symtab->last_symbol = sym;

    return sym;
}

int enj_symbol_pull(enj_symbol* sym, enj_error** err)
{
    if (!sym || !sym->symtab || !sym->symtab->section || !sym->symtab->section->elf ||
        !sym->symtab->section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = sym->symtab->section->elf;

    // Read symbol header
    if (elf->bits == 32)
    {
        if (enj_blob_read(elf->blob, sym->header->pos, &sym->sym32, sizeof(Elf32_Sym), err) < 0)
            return -1;
    }
    else if (elf->bits == 64)
    {
        if (enj_blob_read(elf->blob, sym->header->pos, &sym->sym64, sizeof(Elf64_Sym), err) < 0)
            return -1;
    }

    // Create name anchor
    if (sym->name)
    {
        if (enj_blob_remove_anchor(elf->blob, sym->name, err) < 0)
            return -1;

        sym->name = 0;
    }

    if (sym->symtab->strtab)
    {
        size_t name = ENJ_SYMBOL_GET(sym, st_name);
        size_t name_off = ENJ_ELF_SHDR_GET(sym->symtab->strtab, sh_offset) + name;

        if (!(sym->name = enj_blob_new_anchor(elf->blob, name_off, err)))
            return -1;
    }

    // Create target cursor
    size_t type = ENJ_SYMBOL_TYPE(sym);
    size_t addr = ENJ_SYMBOL_GET(sym, st_value);
    size_t size = ENJ_SYMBOL_GET(sym, st_size);
    size_t shndx = ENJ_SYMBOL_GET(sym, st_shndx);

    enj_elf_shdr* sh = enj_elf_find_shdr_by_index(elf, shndx, err);
    if (*err)
        return -1;

    if (sh && (type == STT_OBJECT || type == STT_FUNC) && addr)
    {
        size_t offset = ENJ_ELF_SHDR_GET(sh, sh_offset) + (addr - ENJ_ELF_SHDR_GET(sh, sh_addr));

        // Avoid error for symbols whose target do not actually reside in the file (think .bss)
        if (offset + size <= sh->elf->blob->buffer_size)
        {
            if (!(sym->target = enj_blob_new_cursor(elf->blob, offset, size, err)))
                return -1;
        }
    }

    if (enj_symbol_update(sym, err) < 0)
        return -1;

    return 0;
}

int enj_symbol_update(enj_symbol* sym, enj_error** err)
{
    if (!sym || !sym->symtab || !sym->symtab->section || !sym->symtab->section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = sym->symtab->section->elf;

    // Clear cached symbol name
    if (sym->cached_name)
    {
        enj_fstring_delete(sym->cached_name);
        sym->cached_name = 0;
    }

    // Cache symbol name (if available)
    if (sym->name)
    {
        int name_ok = 1;
        size_t name_len = 0;
        char c;
        do
        {
            if (enj_blob_read(elf->blob, sym->name->pos + name_len, &c, 1, err) < 0)
            {
                name_ok = 0;
                break;
            }

            ++name_len;
        } while (c != '\0');

        if (name_ok)
        {
            char buffer[name_len];
            if (enj_blob_read(elf->blob, sym->name->pos, &buffer[0], name_len, err) < 0)
                return -1;

            sym->cached_name = enj_fstring_create(&buffer[0], err);
            if (!sym->cached_name)
                return -1;
        }
    }

    return 0;
}

int enj_symbol_push(enj_symbol* sym, enj_error** err)
{
    if (!sym || !sym->symtab || !sym->symtab->section || !sym->symtab->section->elf ||
        !sym->symtab->section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = sym->symtab->section->elf;

    // Update name index
    if (sym->name && sym->symtab->strtab && sym->symtab->strtab->data)
        ENJ_SYMBOL_SET(sym, st_name, sym->name->pos - sym->symtab->strtab->data->start->pos);

    // Update target address if relevant
    if (sym->target)
    {
        // Get target section
        size_t shndx = ENJ_SYMBOL_GET(sym, st_shndx);
        enj_elf_shdr* sh = enj_elf_find_shdr_by_index(elf, shndx, err);
        if (*err)
            return -1;

        if (sh)
        {
            // Update symbol address and size
            size_t addr = ENJ_ELF_SHDR_GET(sh, sh_addr) + (sym->target->start->pos - ENJ_ELF_SHDR_GET(sh, sh_offset));
            ENJ_SYMBOL_SET(sym, st_value, addr);
            ENJ_SYMBOL_SET(sym, st_size, sym->target->length);
        }
    }

    // Write symbol header
    if (elf->bits == 32)
    {
        if (enj_blob_write(elf->blob, sym->header->pos, &sym->sym32, sizeof(Elf32_Sym), err) < 0)
            return -1;
    }
    else if (elf->bits == 64)
    {
        if (enj_blob_write(elf->blob, sym->header->pos, &sym->sym64, sizeof(Elf64_Sym), err) < 0)
            return -1;
    }

    return 0;
}

int enj_symbol_rename(enj_symbol* sym, const char* name, enj_error** err)
{
    if (!sym || !sym->symtab || !sym->symtab->section || !sym->symtab->section->elf ||
        !name)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = sym->symtab->section->elf;

    if (!sym->symtab->strtab)
    {
        enj_error_put(err, ENJ_ERR_NO_STRTAB);
        return -1;
    }

    if (!sym->symtab->strtab->data)
    {
        enj_error_put(err, ENJ_ERR_BAD_STRTAB);
        return -1;
    }

    // If the section had an empty name relocate it to the end of the .strtab
    //  to avoid making 0 a valid name index, as other sections may already use it
    if (sym->name->pos == sym->symtab->strtab->data->start->pos)
    {
        sym->name->pos = sym->symtab->strtab->data->end->pos;
    }

    size_t name_len = strlen(name) + 1;
    size_t old_name_len = sym->cached_name ? sym->cached_name->length : 0;

    if (enj_blob_insert(elf->blob, sym->name->pos, name, name_len, err) < 0 ||
        enj_blob_remove(elf->blob, sym->name->pos + name_len, old_name_len, err) < 0 ||
        enj_symbol_pull(sym, err) < 0)
        return -1;

    return 0;
}

int enj_symbol_remove(enj_symbol* sym, int flags, enj_error** err)
{
    if (!sym || !sym->symtab || !sym->symtab->section || !sym->symtab->section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = sym->symtab->section->elf;

    if (sym->name && sym->symtab->strtab)
    {
        if (!sym->symtab->strtab->data)
        {
            enj_error_put(err, ENJ_ERR_BAD_STRTAB);
            return -1;
        }

        size_t name_off = sym->symtab->strtab->data->start->pos;
        name_off += ENJ_SYMBOL_GET(sym, st_name);

        if (flags & ENJ_SYMBOL_CLEAR_NAME)
        {
            if (enj_blob_set(elf->blob, name_off, 0, sym->cached_name->length, err) < 0)
                return -1;
        }
        else if (flags & ENJ_SYMBOL_DISCARD_NAME)
        {
            if (enj_blob_remove(elf->blob, name_off, sym->cached_name->length, err) < 0)
                return -1;
        }
    }

    if (enj_blob_remove(elf->blob, sym->header->pos, ENJ_SYMBOL_SIZE(sym), err) < 0)
        return -1;

    // Remove the symbol descriptor from the list
    if (sym->prev)
        sym->prev->next = sym->next;
    else
        sym->symtab->symbols = sym->next;

    if (sym->next)
        sym->next->prev = sym->prev;
    else
        sym->symtab->last_symbol = sym->prev;

    // Release descriptor memory
    if (enj_symbol__delete(sym, err) < 0)
        return -1;

    return 0;
}

int enj_symbol__delete(enj_symbol* sym, enj_error** err)
{
    if (!sym || !sym->symtab || !sym->symtab->section || !sym->symtab->section->elf)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = sym->symtab->section->elf;

    if ((sym->header && enj_blob_remove_anchor(elf->blob, sym->header, err) < 0) ||
        (sym->name && enj_blob_remove_anchor(elf->blob, sym->name, err) < 0) ||
        (sym->target && enj_blob_remove_cursor(elf->blob, sym->target, err) < 0))
    {
        return -1;
    }

    if (sym->cached_name)
        enj_fstring_delete(sym->cached_name);

    enj_free(sym);

    return 0;
}

int enj_symtab__pull(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->elf || !section->elf->bits)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_elf* elf = section->elf;

    if (section->content && enj_symtab__delete(section, err) < 0)
        return -1;

    enj_symtab* symtab = enj_malloc(sizeof(enj_symtab));
    if (!symtab)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        return -1;
    }

    section->content = symtab;
    symtab->section = section;
    symtab->strtab = 0;
    symtab->symbols = 0;
    symtab->last_symbol = 0;

    // Get the linked strtab, if provided
    size_t strtabndx = ENJ_ELF_SHDR_GET(section, sh_link);
    symtab->strtab = enj_elf_find_shdr_by_index(elf, strtabndx, err);
    if (*err)
        return -1;

    // Read section parameters
    size_t offset = ENJ_ELF_SHDR_GET(section, sh_offset);
    size_t entsize = ENJ_ELF_SHDR_GET(section, sh_entsize);
    size_t size = ENJ_ELF_SHDR_GET(section, sh_size);
    size_t count = entsize ? size / entsize : 0;

    // Read symbols
    for (size_t i = 0; i < count; ++i)
    {
        enj_symbol* sym = enj_malloc(sizeof(enj_symbol));
        if (!sym)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            return -1;
        }

        sym->symtab = symtab;
        sym->index = i;

        if (!(sym->header = enj_blob_new_anchor(elf->blob, offset + i * entsize, err)) ||
            enj_symbol_pull(sym, err) < 0)
        {
            enj_free(sym);
            return -1;
        }

        // Insert the descriptor into the linked list
        sym->prev = symtab->last_symbol;
        sym->next = 0;
        if (sym->prev)
            sym->prev->next = sym;
        else
            symtab->symbols = sym;
        symtab->last_symbol = sym;
    }

    return 0;
}

int enj_symtab__update(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_symtab* symtab = (enj_symtab*) section->content;

    for (enj_symbol* sym = symtab->symbols; sym; sym = sym->next)
    {
        if (enj_symbol_update(sym, err) < 0)
            return -1;
    }

    return 0;
}

int enj_symtab__push(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_symtab* symtab = (enj_symtab*) section->content;

    // Just update the symbols, the section size will be updated automatically
    //  based on insertions / deletions
    for (enj_symbol* sym = symtab->symbols; sym; sym = sym->next)
    {
        if (enj_symbol_push(sym, err) < 0)
            return -1;
    }

    if (symtab->strtab)
    {
        ENJ_ELF_SHDR_SET(section, sh_link, symtab->strtab->index);
    }
    else
    {
        ENJ_ELF_SHDR_SET(section, sh_link, SHN_UNDEF);
    }

    return 0;
}

int enj_symtab__delete(enj_elf_shdr* section, enj_error** err)
{
    if (!section || !section->content)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    enj_symtab* symtab = (enj_symtab*) section->content;

    for (enj_symbol* sym = symtab->symbols; sym; )
    {
        enj_symbol* next = sym->next;

        if (enj_symbol__delete(sym, err) < 0)
            return -1;

        sym = next;
    }

    enj_free(symtab);

    section->content = 0;

    return 0;
}
