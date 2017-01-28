#include <iostream>
#include <elf.h>

#include "elfninja/core/error.h"
#include "elfninja/core/contiguous_blob.h"
#include "elfninja/core/record.h"

/*
uint16_t      e_type;
uint16_t      e_machine;
uint32_t      e_version;
ElfN_Addr     e_entry;
ElfN_Off      e_phoff;
ElfN_Off      e_shoff;
uint32_t      e_flags;
uint16_t      e_ehsize;
uint16_t      e_phentsize;
uint16_t      e_phnum;
uint16_t      e_shentsize;
uint16_t      e_shnum;
uint16_t      e_shstrndx;
*/

int main(int argc, char** argv)
{
    try
    {
        enj::Record r;

        r.addField("ei_mag",        EI_MAG0, SELFMAG);
        r.addField("ei_class",      EI_CLASS, 1);
        r.addField("ei_data",       EI_DATA, 1);
        r.addField("ei_version",    EI_VERSION, 1);
        r.addField("ei_osabi",      EI_OSABI, 1);
        r.addField("ei_abiversion", EI_ABIVERSION, 1);
        r.addField("e_type",        &Elf64_Ehdr::e_type);
        r.addField("e_machine",     &Elf64_Ehdr::e_machine);
        r.addField("e_version",     &Elf64_Ehdr::e_version);
        r.addField("e_entry",       &Elf64_Ehdr::e_entry);
        r.addField("e_phoff",       &Elf64_Ehdr::e_phoff);
        r.addField("e_shoff",       &Elf64_Ehdr::e_shoff);
        r.addField("e_flags",       &Elf64_Ehdr::e_flags);
        r.addField("e_ehsize",      &Elf64_Ehdr::e_ehsize);
        r.addField("e_phentsize",   &Elf64_Ehdr::e_phentsize);
        r.addField("e_phnum",       &Elf64_Ehdr::e_phnum);
        r.addField("e_shentsize",   &Elf64_Ehdr::e_shentsize);
        r.addField("e_shnum",       &Elf64_Ehdr::e_shnum);
        r.addField("e_shstrndx",    &Elf64_Ehdr::e_shstrndx);

        Elf64_Ehdr* ehdr = (Elf64_Ehdr*) 0x400000;

        r.readFrom((uint8_t const*) ehdr);

        printf("e_entry = 0x%08lX\n", r.get("e_entry"));
    }
    catch (enj::Error const& e)
    {
        std::cerr << "[" << e.location() << "]: " << e.description() << std::endl;
        return -1;
    }

    return 0;
}

int main_blob(int argc, char** argv)
{
    try
    {
        enj::Blob* b = new enj::ContiguousBlob(4);

        enj::Blob::Cursor* c = b->addCursor(0, 0);

        b->insertAfterStart(c, (uint8_t const*) "3456", 4);

        b->insertBeforeStart(c, (uint8_t const*) "AB", 2);
        b->insertAfterEnd(c, (uint8_t const*) "CD", 2);
        b->insertBeforeEnd(c, (uint8_t const*) "789", 3);
        b->insertAfterStart(c, (uint8_t const*) "012", 3);

        b->insert(0, (uint8_t const*) "__", 2);

        // Now should be __AB0123456789CD

        b->removeAfterEnd(c, 1);
        b->removeAfterStart(c, 2);

        char as_str[256];
        b->read(0, (uint8_t*) &as_str[0], b->size());
        for (size_t i = 0; i < b->size(); ++i)
            if (!as_str[i])
                as_str[i] = '*';

        std::cout << "Anchor = (" << c->start()->pos();
        std::cout << "=" << as_str[c->start()->pos()] << " -> ";
        std::cout << c->end()->pos();
        std::cout << "=" << as_str[c->end()->pos()] << ")" << std::endl;

        as_str[b->size()] = '\0';
        std::cout << "'" << &as_str[0] << "'" << std::endl;

        delete b;
    }
    catch (enj::Error const& e)
    {
        std::cerr << "[" << e.location() << "]: " << e.description() << std::endl;
        return -1;
    }

    return 0;
}
