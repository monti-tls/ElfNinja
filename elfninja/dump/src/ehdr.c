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

#include "elfninja/dump/ehdr.h"
#include "elfninja/dump/shdr.h"
#include "elfninja/core/elf.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum
{
    #define DEF_FIELD(name, ident, elf_field, attrs, descr) EHDR_ ## ident,
    #include "elfninja/dump/ehdr.def"
};

static int _ehdr_field(void* arg0, void* arg1, int fd, int width, char pad, enj_error** err)
{
    if (!arg1)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    FILE* f = fdopen(dup(fd), "ab");
    if (!f)
    {
        enj_error_put(err, ENJ_ERR_ARGUMENT);
        return -1;
    }

    size_t field = (size_t) arg0;
    enj_elf* elf = (enj_elf*) arg1;
    unsigned char* e_ident = (unsigned char*) ENJ_ELF_EHDR_GET(elf, e_ident);

    switch (field)
    {
        case EHDR_CLASS:
        {
            unsigned char byte = e_ident[EI_CLASS];

            switch (byte)
            {
                case ELFCLASS32: enjd_padded_fprintf(f, width, pad, "ELFCLASS32"); break;
                case ELFCLASS64: enjd_padded_fprintf(f, width, pad, "ELFCLASS64"); break;
                default: enjd_padded_fprintf(f, width, pad, "ELFCLASSNONE(0x%02X)", byte);
            }

            break;
        }

        case EHDR_DATA:
        {
            unsigned char byte = e_ident[EI_DATA];

            switch (byte)
            {
                case ELFDATA2LSB: enjd_padded_fprintf(f, width, pad, "ELFDATA2LSB"); break;
                case ELFDATA2MSB: enjd_padded_fprintf(f, width, pad, "ELFDATA2MSB"); break;
                default: enjd_padded_fprintf(f, width, pad, "ELFDATANONE(0x%02X)", byte);
            }

            break;
        }

        case EHDR_VERSION:
        {
            unsigned char byte = e_ident[EI_VERSION];

            switch (byte)
            {
                case EV_CURRENT: enjd_padded_fprintf(f, width, pad, "EV_CURRENT"); break;
                default: enjd_padded_fprintf(f, width, pad, "EV_NONE(0x%02X)", byte);
            }

            break;
        }

        case EHDR_OSABI:
        {
            unsigned char byte = e_ident[EI_OSABI];

            switch (byte)
            {
                case ELFOSABI_SYSV:       enjd_padded_fprintf(f, width, pad, "ELFOSABI_SYSV");       break;
                case ELFOSABI_HPUX:       enjd_padded_fprintf(f, width, pad, "ELFOSABI_HPUX");       break;
                case ELFOSABI_NETBSD:     enjd_padded_fprintf(f, width, pad, "ELFOSABI_NETBSD");     break;
                case ELFOSABI_GNU:        enjd_padded_fprintf(f, width, pad, "ELFOSABI_GNU");        break;
                case ELFOSABI_SOLARIS:    enjd_padded_fprintf(f, width, pad, "ELFOSABI_SOLARIS");    break;
                case ELFOSABI_AIX:        enjd_padded_fprintf(f, width, pad, "ELFOSABI_AIX");        break;
                case ELFOSABI_IRIX:       enjd_padded_fprintf(f, width, pad, "ELFOSABI_IRIX");       break;
                case ELFOSABI_FREEBSD:    enjd_padded_fprintf(f, width, pad, "ELFOSABI_FREEBSD");    break;
                case ELFOSABI_TRU64:      enjd_padded_fprintf(f, width, pad, "ELFOSABI_TRU64");      break;
                case ELFOSABI_MODESTO:    enjd_padded_fprintf(f, width, pad, "ELFOSABI_MODESTO");    break;
                case ELFOSABI_OPENBSD:    enjd_padded_fprintf(f, width, pad, "ELFOSABI_OPENBSD");    break;
                case ELFOSABI_ARM_AEABI:  enjd_padded_fprintf(f, width, pad, "ELFOSABI_ARM_AEABI");  break;
                case ELFOSABI_ARM:        enjd_padded_fprintf(f, width, pad, "ELFOSABI_ARM");        break;
                case ELFOSABI_STANDALONE: enjd_padded_fprintf(f, width, pad, "ELFOSABI_STANDALONE"); break;
                default: enjd_padded_fprintf(f, width, pad, "ELFOSABI_NONE(0x%02X)", byte);
            }

            break;
        }

        case EHDR_ABIVERSION:
        {
            fprintf(f, "%02X", e_ident[EI_ABIVERSION]);
            break;
        }

        case EHDR_TYPE:
        {
            size_t word = ENJ_ELF_EHDR_GET(elf, e_type);

            switch (word)
            {
                case ET_REL:  enjd_padded_fprintf(f, width, pad, "ET_REL");  break;
                case ET_EXEC: enjd_padded_fprintf(f, width, pad, "ET_EXEC"); break;
                case ET_DYN:  enjd_padded_fprintf(f, width, pad, "ET_DYN");  break;
                case ET_CORE: enjd_padded_fprintf(f, width, pad, "ET_CORE"); break;
                case ET_NUM:  enjd_padded_fprintf(f, width, pad, "ET_NUM");  break;
                default: enjd_padded_fprintf(f, width, pad, "ET_NONE(0x%0*X)", (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_type)), ENJ_ELF_EHDR_GET(elf, e_type));
            }

            break;
        }

        case EHDR_MACHINE:
        {
            size_t word = ENJ_ELF_EHDR_GET(elf, e_machine);

            switch (word)
            {
                case EM_M32:         enjd_padded_fprintf(f, width, pad, "EM_M32");         break;
                case EM_SPARC:       enjd_padded_fprintf(f, width, pad, "EM_SPARC");       break;
                case EM_386:         enjd_padded_fprintf(f, width, pad, "EM_386");         break;
                case EM_68K:         enjd_padded_fprintf(f, width, pad, "EM_68K");         break;
                case EM_88K:         enjd_padded_fprintf(f, width, pad, "EM_88K");         break;
                case EM_860:         enjd_padded_fprintf(f, width, pad, "EM_860");         break;
                case EM_MIPS:        enjd_padded_fprintf(f, width, pad, "EM_MIPS");        break;
                case EM_S370:        enjd_padded_fprintf(f, width, pad, "EM_S370");        break;
                case EM_MIPS_RS3_LE: enjd_padded_fprintf(f, width, pad, "EM_MIPS_RS3_LE"); break;
                case EM_PARISC:      enjd_padded_fprintf(f, width, pad, "EM_PARISC");      break;
                case EM_VPP500:      enjd_padded_fprintf(f, width, pad, "EM_VPP500");      break;
                case EM_SPARC32PLUS: enjd_padded_fprintf(f, width, pad, "EM_SPARC32PLUS"); break;
                case EM_960:         enjd_padded_fprintf(f, width, pad, "EM_960");         break;
                case EM_PPC:         enjd_padded_fprintf(f, width, pad, "EM_PPC");         break;
                case EM_PPC64:       enjd_padded_fprintf(f, width, pad, "EM_PPC64");       break;
                case EM_S390:        enjd_padded_fprintf(f, width, pad, "EM_S390");        break;
                case EM_V800:        enjd_padded_fprintf(f, width, pad, "EM_V800");        break;
                case EM_FR20:        enjd_padded_fprintf(f, width, pad, "EM_FR20");        break;
                case EM_RH32:        enjd_padded_fprintf(f, width, pad, "EM_RH32");        break;
                case EM_RCE:         enjd_padded_fprintf(f, width, pad, "EM_RCE");         break;
                case EM_ARM:         enjd_padded_fprintf(f, width, pad, "EM_ARM");         break;
                case EM_FAKE_ALPHA:  enjd_padded_fprintf(f, width, pad, "EM_FAKE_ALPHA");  break;
                case EM_SH:          enjd_padded_fprintf(f, width, pad, "EM_SH");          break;
                case EM_SPARCV9:     enjd_padded_fprintf(f, width, pad, "EM_SPARCV9");     break;
                case EM_TRICORE:     enjd_padded_fprintf(f, width, pad, "EM_TRICORE");     break;
                case EM_ARC:         enjd_padded_fprintf(f, width, pad, "EM_ARC");         break;
                case EM_H8_300:      enjd_padded_fprintf(f, width, pad, "EM_H8_300");      break;
                case EM_H8_300H:     enjd_padded_fprintf(f, width, pad, "EM_H8_300H");     break;
                case EM_H8S:         enjd_padded_fprintf(f, width, pad, "EM_H8S");         break;
                case EM_H8_500:      enjd_padded_fprintf(f, width, pad, "EM_H8_500");      break;
                case EM_IA_64:       enjd_padded_fprintf(f, width, pad, "EM_IA_64");       break;
                case EM_MIPS_X:      enjd_padded_fprintf(f, width, pad, "EM_MIPS_X");      break;
                case EM_COLDFIRE:    enjd_padded_fprintf(f, width, pad, "EM_COLDFIRE");    break;
                case EM_68HC12:      enjd_padded_fprintf(f, width, pad, "EM_68HC12");      break;
                case EM_MMA:         enjd_padded_fprintf(f, width, pad, "EM_MMA");         break;
                case EM_PCP:         enjd_padded_fprintf(f, width, pad, "EM_PCP");         break;
                case EM_NCPU:        enjd_padded_fprintf(f, width, pad, "EM_NCPU");        break;
                case EM_NDR1:        enjd_padded_fprintf(f, width, pad, "EM_NDR1");        break;
                case EM_STARCORE:    enjd_padded_fprintf(f, width, pad, "EM_STARCORE");    break;
                case EM_ME16:        enjd_padded_fprintf(f, width, pad, "EM_ME16");        break;
                case EM_ST100:       enjd_padded_fprintf(f, width, pad, "EM_ST100");       break;
                case EM_TINYJ:       enjd_padded_fprintf(f, width, pad, "EM_TINYJ");       break;
                case EM_X86_64:      enjd_padded_fprintf(f, width, pad, "EM_X86_64");      break;
                case EM_PDSP:        enjd_padded_fprintf(f, width, pad, "EM_PDSP");        break;
                case EM_FX66:        enjd_padded_fprintf(f, width, pad, "EM_FX66");        break;
                case EM_ST9PLUS:     enjd_padded_fprintf(f, width, pad, "EM_ST9PLUS");     break;
                case EM_ST7:         enjd_padded_fprintf(f, width, pad, "EM_ST7");         break;
                case EM_68HC16:      enjd_padded_fprintf(f, width, pad, "EM_68HC16");      break;
                case EM_68HC11:      enjd_padded_fprintf(f, width, pad, "EM_68HC11");      break;
                case EM_68HC08:      enjd_padded_fprintf(f, width, pad, "EM_68HC08");      break;
                case EM_68HC05:      enjd_padded_fprintf(f, width, pad, "EM_68HC05");      break;
                case EM_SVX:         enjd_padded_fprintf(f, width, pad, "EM_SVX");         break;
                case EM_ST19:        enjd_padded_fprintf(f, width, pad, "EM_ST19");        break;
                case EM_VAX:         enjd_padded_fprintf(f, width, pad, "EM_VAX");         break;
                case EM_CRIS:        enjd_padded_fprintf(f, width, pad, "EM_CRIS");        break;
                case EM_JAVELIN:     enjd_padded_fprintf(f, width, pad, "EM_JAVELIN");     break;
                case EM_FIREPATH:    enjd_padded_fprintf(f, width, pad, "EM_FIREPATH");    break;
                case EM_ZSP:         enjd_padded_fprintf(f, width, pad, "EM_ZSP");         break;
                case EM_MMIX:        enjd_padded_fprintf(f, width, pad, "EM_MMIX");        break;
                case EM_HUANY:       enjd_padded_fprintf(f, width, pad, "EM_HUANY");       break;
                case EM_PRISM:       enjd_padded_fprintf(f, width, pad, "EM_PRISM");       break;
                case EM_AVR:         enjd_padded_fprintf(f, width, pad, "EM_AVR");         break;
                case EM_FR30:        enjd_padded_fprintf(f, width, pad, "EM_FR30");        break;
                case EM_D10V:        enjd_padded_fprintf(f, width, pad, "EM_D10V");        break;
                case EM_D30V:        enjd_padded_fprintf(f, width, pad, "EM_D30V");        break;
                case EM_V850:        enjd_padded_fprintf(f, width, pad, "EM_V850");        break;
                case EM_M32R:        enjd_padded_fprintf(f, width, pad, "EM_M32R");        break;
                case EM_MN10300:     enjd_padded_fprintf(f, width, pad, "EM_MN10300");     break;
                case EM_MN10200:     enjd_padded_fprintf(f, width, pad, "EM_MN10200");     break;
                case EM_PJ:          enjd_padded_fprintf(f, width, pad, "EM_PJ");          break;
                case EM_OPENRISC:    enjd_padded_fprintf(f, width, pad, "EM_OPENRISC");    break;
                case EM_ARC_A5:      enjd_padded_fprintf(f, width, pad, "EM_ARC_A5");      break;
                case EM_XTENSA:      enjd_padded_fprintf(f, width, pad, "EM_XTENSA");      break;
                case EM_AARCH64:     enjd_padded_fprintf(f, width, pad, "EM_AARCH64");     break;
                case EM_TILEPRO:     enjd_padded_fprintf(f, width, pad, "EM_TILEPRO");     break;
                case EM_TILEGX:      enjd_padded_fprintf(f, width, pad, "EM_TILEGX");      break;
                case EM_ALPHA:       enjd_padded_fprintf(f, width, pad, "EM_ALPHA");       break;
                default: enjd_padded_fprintf(f, width, pad, "ELFOSABI_NONE(0x%0*X)", (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_type)), ENJ_ELF_EHDR_GET(elf, e_type));
            }

            break;
        }

        case EHDR_ENTRY:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_entry)), ENJ_ELF_EHDR_GET(elf, e_entry));
            break;

        case EHDR_PHOFF:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_phoff)), ENJ_ELF_EHDR_GET(elf, e_phoff));
            break;

        case EHDR_SHOFF:
            fprintf(f, "%0*lX", 2 * (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_shoff)), ENJ_ELF_EHDR_GET(elf, e_shoff));
            break;

        case EHDR_FLAGS:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_flags)), ENJ_ELF_EHDR_GET(elf, e_flags));
            break;

        case EHDR_EHSIZE:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_ehsize)), ENJ_ELF_EHDR_GET(elf, e_ehsize));
            break;

        case EHDR_PHENTSIZE:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_phentsize)), ENJ_ELF_EHDR_GET(elf, e_phentsize));
            break;

        case EHDR_PHNUM:
            enjd_padded_fprintf(f, width, pad, "%d", ENJ_ELF_EHDR_GET(elf, e_phnum));
            break;

        case EHDR_SHENTSIZE:
            fprintf(f, "%0*X", 2 * (int) sizeof(ENJ_ELF_EHDR_GET(elf, e_shentsize)), ENJ_ELF_EHDR_GET(elf, e_shentsize));
            break;

        case EHDR_SHNUM:
            enjd_padded_fprintf(f, width, pad, "%d", ENJ_ELF_EHDR_GET(elf, e_shnum));
            break;

        case EHDR_SHSTRNDX:
            enjd_padded_fprintf(f, width, pad, "%d", ENJ_ELF_EHDR_GET(elf, e_shstrndx));
            break;

        case EHDR_SHSTR_NAME:
        {
            enj_elf_shdr* shstrtab = enj_elf_find_shdr_by_index(elf, ENJ_ELF_EHDR_GET(elf, e_shstrndx), 0);
            if (shstrtab && shstrtab->cached_name)
                enjd_padded_fprintf(f, width, pad, "%s", shstrtab->cached_name->string);
            else if (!shstrtab)
                enjd_padded_fprintf(f, width, pad, "<corrupt>");
            break;
        }
    }

    fclose(f);
    return 0;
}

enjd_formatter* enjd_ehdr_formatter_create(enj_error** err)
{
    enjd_formatter* fmt = enjd_formatter_create(err);
    if (!fmt)
        return 0;

    #define DEF_FIELD(name, ident, elf_field, attr, sdescr) \
        || !enjd_formatter_new_elem(fmt, name, &_ehdr_field, (void*) EHDR_ ## ident, err)

    if (0
        #include "elfninja/dump/ehdr.def"
        )
    {
        enjd_formatter_delete(fmt);
        return 0;
    }

    return fmt;
}
