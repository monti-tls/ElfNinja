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

#include "elfninja/input/parse.h"

#include "elfninja/core/error.h"
#include "elfninja/core/malloc.h"

#include <string.h>
#include <ctype.h>

size_t enji_parse_number(const char* string, enj_error** err)
{
    if (!string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    const char* p = string;
    int base = 10;

    while (isspace(*p))
        ++p;

    if (!*p)
    {
        enj_error_put(err, ENJ_ERR_BAD_NUMBER);
        return 0;
    }

    if (*p == '0')
    {
        ++p;

        if (!*p)
            return 0;

        if (*p == 'x' || *p == 'X')
        {
            base = 16;
            ++p;
        }
        else if (*p == 'b' || *p == 'B')
        {
            base = 2;
            ++p;
        }
    }

    if (base != 10 && !*p)
    {
        enj_error_put(err, ENJ_ERR_BAD_NUMBER);
        return 0;
    }

    size_t value = 0;

    char c;
    while ((c = *p++))
    {
        if ((base == 10 && !isdigit(c)) ||
            (base == 16 && !isxdigit(c)) ||
            (base == 2 && (c != '0' && c != '1')))
        {
            enj_error_put(err, ENJ_ERR_BAD_NUMBER);
            return 0;
        }

        value *= base;

        if (base == 10)
        {
            value += c - '0';
        }
        else if (base == 16)
        {
            if (c <= '9')
                value += c - '0';
            else if (c <= 'X')
                value += 10 + c - 'A';
            else if (c <= 'x')
                value += 10 + c - 'a';
        }
        else if (base == 2)
        {
            value += c - '0';
        }
    }

    return value;
}

size_t enji_parse_offset(enj_elf* elf, const char* string, enj_error** err)
{
    if (!elf || !string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    const char* p = string;
    size_t file_offset = 0;

    while (isspace(*p))
        ++p;

    if (*p == '@')
    {
        ++p;
        file_offset = enji_parse_number(p, err);
        if (*err)
        {
            enj_error_wrap(err, ENJ_ERR_BAD_OFFSET, *err);
            return 0;
        }

        size_t found_addr = 0;

        for (enj_elf_shdr* section = elf->sections; section; section = section->next)
        {
            size_t flags = ENJ_ELF_SHDR_GET(section, sh_flags);
            size_t type = ENJ_ELF_SHDR_GET(section, sh_type);

            if (!(flags & SHF_ALLOC) || type == SHT_NOBITS)
                continue;

            size_t addr = ENJ_ELF_SHDR_GET(section, sh_addr);
            size_t size = ENJ_ELF_SHDR_GET(section, sh_size);
            size_t offset = ENJ_ELF_SHDR_GET(section, sh_offset);

            if (file_offset >= addr && file_offset < addr + size)
            {
                file_offset = (file_offset - addr) + offset;
                found_addr = 1;
                break;
            }
        }

        if (!found_addr)
        {
            enj_error_put(err, ENJ_ERR_BAD_OFFSET);
            return 0;
        }
    }
    else if (*p == '+')
    {
        ++p;
        file_offset = enji_parse_number(p, err);
        if (*err || file_offset >= elf->blob->buffer_size)
        {
            enj_error_wrap(err, ENJ_ERR_BAD_OFFSET, *err);
            return 0;
        }
    }
    else
    {
        size_t name_len = 0;
        const char* s = p;
        for (; *s != '+' && *s != '-' && *s; ++s)
            ++name_len;

        if (!*s)
        {
            enj_error_wrap(err, ENJ_ERR_BAD_OFFSET, *err);
            return 0;
        }

        char sign = *s++;
        file_offset = enji_parse_number(s, err);
        if (*err)
        {
            enj_error_wrap(err, ENJ_ERR_BAD_OFFSET, *err);
            return 0;
        }

        char* name = enj_malloc(name_len + 1);
        if (!name)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            return 0;
        }

        memcpy(name, p, name_len);
        name[name_len] = '\0';

        enj_elf_shdr* section = enj_elf_find_shdr_by_name(elf, name, 0);
        enj_free(name);

        if (!section)
        {
            enj_error_put(err, section ? ENJ_ERR_BAD_OFFSET : ENJ_ERR_NEXISTS);
            return 0;
        }

        size_t sh_offset = ENJ_ELF_SHDR_GET(section, sh_offset);

        if (sign == '+')
        {
            file_offset += sh_offset;
        }
        else if (sign == '-')
        {
            if (file_offset > sh_offset)
            {
                enj_error_put(err, ENJ_ERR_BAD_OFFSET);
                return 0;
            }

            file_offset = sh_offset - file_offset;
        }

        if (file_offset >= elf->blob->buffer_size)
        {
            enj_error_put(err, section ? ENJ_ERR_BAD_OFFSET : ENJ_ERR_NEXISTS);
            return 0;
        }
    }

    return file_offset;
}

size_t enji_parse_address(enj_elf* elf, const char* string, enj_error** err)
{
    if (!elf || !string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    const char* p = string;
    size_t file_offset = 0;

    while (isspace(*p))
        ++p;

    if (isdigit(*p))
    {
        return enji_parse_number(p, err);
    }
    else if (*p == '+')
    {
        ++p;
        file_offset = enji_parse_number(p, err);
        if (*err || file_offset >= elf->blob->buffer_size)
        {
            enj_error_wrap(err, ENJ_ERR_BAD_OFFSET, *err);
            return 0;
        }
    }
    else
    {
        size_t name_len = 0;
        const char* s = p;
        for (; *s != '+' && *s; ++s)
            ++name_len;

        if (!*s)
        {
            enj_error_wrap(err, ENJ_ERR_BAD_OFFSET, *err);
            return 0;
        }

        ++s;
        file_offset = enji_parse_number(s, err);
        if (*err)
        {
            enj_error_wrap(err, ENJ_ERR_BAD_OFFSET, *err);
            return 0;
        }

        char* name = enj_malloc(name_len + 1);
        if (!name)
        {
            enj_error_put(err, ENJ_ERR_MALLOC);
            return 0;
        }

        memcpy(name, p, name_len);
        name[name_len] = '\0';

        enj_elf_shdr* section = enj_elf_find_shdr_by_name(elf, name, 0);
        enj_free(name);

        if (!section)
        {
            enj_error_put(err, section ? ENJ_ERR_BAD_OFFSET : ENJ_ERR_NEXISTS);
            return 0;
        }

        file_offset += ENJ_ELF_SHDR_GET(section, sh_offset);

        if (file_offset >= elf->blob->buffer_size)
        {
            enj_error_put(err, section ? ENJ_ERR_BAD_OFFSET : ENJ_ERR_NEXISTS);
            return 0;
        }
    }

    size_t found_offset = 0;

    for (enj_elf_shdr* section = elf->sections; section; section = section->next)
    {
        size_t flags = ENJ_ELF_SHDR_GET(section, sh_flags);
        size_t type = ENJ_ELF_SHDR_GET(section, sh_type);

        if (!(flags & SHF_ALLOC) || type == SHT_NOBITS)
            continue;

        size_t addr = ENJ_ELF_SHDR_GET(section, sh_addr);
        size_t size = ENJ_ELF_SHDR_GET(section, sh_size);
        size_t offset = ENJ_ELF_SHDR_GET(section, sh_offset);

        if (file_offset >= offset && file_offset < offset + size)
        {
            file_offset = (file_offset - offset) + addr;
            found_offset = 1;
            break;
        }
    }

    if (!found_offset)
    {
        enj_error_put(err, ENJ_ERR_BAD_OFFSET);
        return 0;
    }

    return file_offset;
}

char* enji_parse_hex(const char* string, size_t* length, enj_error** err)
{
    if (!string || !length)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    char* nibbles = 0;
    size_t nibble_count = 0;
    size_t nibble = 0;

    for (int counting = 1; counting >= 0; --counting)
    {
        for (const char* p = string; *p; ++p)
        {
            if (isspace(*p))
                continue;

            if (!isxdigit(*p))
            {
                enj_error_put(err, ENJ_ERR_BAD_NUMBER);
                return 0;
            }

            if (counting)
            {
                ++nibble_count;
            }
            else
            {
                if (!nibbles)
                {
                    if (nibble_count & 1)
                    {
                        enj_error_put(err, ENJ_ERR_BAD_NUMBER);
                        return 0;
                    }

                    nibbles = enj_malloc(nibble_count);
                    if (!nibbles)
                    {
                        enj_error_put(err, ENJ_ERR_MALLOC);
                        return 0;
                    }
                }

                nibbles[nibble++] = *p;
            }
        }
    }

    char* bytes = enj_malloc(nibble_count / 2);
    if (!bytes)
    {
        enj_error_put(err, ENJ_ERR_MALLOC);
        enj_free(nibbles);
        return 0;
    }

    for (size_t byte = 0; byte < nibble_count / 2; ++byte)
    {
        char nib1 = nibbles[2 * byte];
        char nib2 = nibbles[2 * byte + 1];

        size_t nib1v = nib1 <= '9' ? nib1 - '0' : (10 + (nib1 <= 'X' ? nib1 - 'A' : nib1 - 'a'));
        size_t nib2v = nib2 <= '9' ? nib2 - '0' : (10 + (nib2 <= 'X' ? nib2 - 'A' : nib2 - 'a'));

        bytes[byte] = nib1v * 16 + nib2v;
    }

    *length = nibble_count / 2;

    enj_free(nibbles);
    return bytes;
}

size_t enji_parse_field(enji_parse_dict* dict, const char* string, enj_error** err)
{
    if (!dict || !string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    if (isdigit(*string))
    {
        return enji_parse_number(string, err);
    }
    else
    {
        for (size_t i = 0; dict[i].name; ++i)
        {
            if (!strcmp(dict[i].name, string))
                return dict[i].value;
        }
    }

    enj_error_put(err, ENJ_ERR_NEXISTS);
    return 0;
}

size_t enji_parse_flags(enji_parse_dict* dict, const char* string, enj_error** err)
{
    if (!dict || !string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    size_t flags = 0;

    for (const char* p = string; *p; )
    {
        size_t len = 0;
        const char* s;
        for (s = p; *s != '|' && *s; ++s)
            ++len;

        if (!len)
        {
            enj_error_put(err, ENJ_ERR_BAD_FIELD);
            return 0;
        }

        char cpy[len + 1];
        memcpy(&cpy[0], p, len);
        cpy[len] = '\0';

        size_t mask = enji_parse_field(&dict[0], &cpy[0], err);
        if (*err)
            return 0;

        flags |= mask;

        if (*s == '|')
            p += (len + 1);
        else
            break;
    }

    return flags;
}

size_t enji_parse_sh_type(const char* string, enj_error** err)
{
    if (!string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    static enji_parse_dict dict[] =
    {
        { "SHT_NULL",           SHT_NULL },
        { "SHT_PROGBITS",       SHT_PROGBITS },
        { "SHT_SYMTAB",         SHT_SYMTAB },
        { "SHT_STRTAB",         SHT_STRTAB },
        { "SHT_RELA",           SHT_RELA },
        { "SHT_HASH",           SHT_HASH },
        { "SHT_DYNAMIC",        SHT_DYNAMIC },
        { "SHT_NOTE",           SHT_NOTE },
        { "SHT_NOBITS",         SHT_NOBITS },
        { "SHT_REL",            SHT_REL },
        { "SHT_SHLIB",          SHT_SHLIB },
        { "SHT_DYNSYM",         SHT_DYNSYM },
        { "SHT_INIT_ARRAY",     SHT_INIT_ARRAY },
        { "SHT_FINI_ARRAY",     SHT_FINI_ARRAY },
        { "SHT_PREINIT_ARRAY",  SHT_PREINIT_ARRAY },
        { "SHT_GROUP",          SHT_GROUP },
        { "SHT_SYMTAB_SHNDX",   SHT_SYMTAB_SHNDX },
        { "SHT_GNU_ATTRIBUTES", SHT_GNU_ATTRIBUTES },
        { "SHT_GNU_HASH",       SHT_GNU_HASH },
        { "SHT_GNU_LIBLIST",    SHT_GNU_LIBLIST },
        { "SHT_CHECKSUM",       SHT_CHECKSUM },
        { "SHT_SUNW_move",      SHT_SUNW_move },
        { "SHT_SUNW_COMDAT",    SHT_SUNW_COMDAT },
        { "SHT_SUNW_syminfo",   SHT_SUNW_syminfo },
        { "SHT_GNU_verdef",     SHT_GNU_verdef },
        { "SHT_GNU_verneed",    SHT_GNU_verneed },
        { "SHT_GNU_versym",     SHT_GNU_versym },
        { 0, 0 }
    };

    return enji_parse_field(&dict[0], string, err);
}

size_t enji_parse_sh_flags(const char* string, enj_error** err)
{
    if (!string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    static enji_parse_dict dict[] =
    {
        { "SHF_WRITE",            SHF_WRITE },
        { "SHF_ALLOC",            SHF_ALLOC },
        { "SHF_EXECINSTR",        SHF_EXECINSTR },
        { "SHF_MERGE",            SHF_MERGE },
        { "SHF_STRINGS",          SHF_STRINGS },
        { "SHF_INFO_LINK",        SHF_INFO_LINK },
        { "SHF_LINK_ORDER",       SHF_LINK_ORDER },
        { "SHF_OS_NONCONFORMING", SHF_OS_NONCONFORMING },
        { "SHF_GROUP",            SHF_GROUP },
        { "SHF_TLS",              SHF_TLS },
        { "SHF_ORDERED",          SHF_ORDERED },
        { "SHF_EXCLUDE",          SHF_EXCLUDE },
        // Some aliases
        { "W",                    SHF_WRITE },
        { "A",                    SHF_ALLOC },
        { "X",                    SHF_EXECINSTR },
        { "M",                    SHF_MERGE },
        { "S",                    SHF_STRINGS },
        { "I",                    SHF_INFO_LINK }
    };

    return enji_parse_flags(&dict[0], string, err);
}


size_t enji_parse_p_type(const char* string, enj_error** err)
{
    if (!string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    static enji_parse_dict dict[] =
    {
        { "PT_NULL",         PT_NULL },
        { "PT_LOAD",         PT_LOAD },
        { "PT_DYNAMIC",      PT_DYNAMIC },
        { "PT_INTERP",       PT_INTERP },
        { "PT_NOTE",         PT_NOTE },
        { "PT_SHLIB",        PT_SHLIB },
        { "PT_PHDR",         PT_PHDR },
        { "PT_TLS",          PT_TLS },
        { "PT_GNU_EH_FRAME", PT_GNU_EH_FRAME },
        { "PT_GNU_STACK",    PT_GNU_STACK },
        { "PT_GNU_RELRO",    PT_GNU_RELRO },
        { "PT_SUNWBSS",      PT_SUNWBSS },
        { "PT_SUNWSTACK",    PT_SUNWSTACK },
        { 0, 0 }
    };

    return enji_parse_field(&dict[0], string, err);
}

size_t enji_parse_p_flags(const char* string, enj_error** err)
{
    if (!string)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return 0;
    }

    static enji_parse_dict dict[] =
    {
        { "PF_X", PF_X },
        { "PF_W", PF_W },
        { "PF_R", PF_R },
        // Some aliases
        { "X",    PF_X },
        { "W",    PF_W },
        { "R",    PF_R },
    };

    return enji_parse_flags(&dict[0], string, err);
}
