#include "elfninja/core/elf_ehdr.h"
#include "elfninja/core/error.h"

#include <elf.h>

using namespace enj;

ElfEhdr::ElfEhdr(Elf* elf)
    : ElfHeader(elf)
{}

ElfEhdr::~ElfEhdr()
{}

void ElfEhdr::pull()
{
    /* First, read some header bytes to get the magic number
     * and the ELF file class */

    Record* stage1 = new Record();
    stage1->addField("ei_mag",   EI_MAG0,  SELFMAG);
    stage1->addField("ei_class", EI_CLASS, 1);

    Piece* p = new Piece(blob()->addCursor(0, stage1->size()), stage1);
    p->read();

    enj_assert(BadElfMag, stage1->get("ei_mag") != *((size_t*) ELFMAG));
    size_t ei_class = stage1->get("ei_class");

    delete p;

    /* Now, create the real header record depending on the file class */

    enj_assert(BadElfClass, ei_class == ELFCLASS32 || ei_class == ELFCLASS64);

    Record* r = new Record();
    if (ei_class == ELFCLASS32)
        M_initRecord<Elf32_Ehdr>(r);
    else
        M_initRecord<Elf64_Ehdr>(r);

    M_setHeader(blob()->addCursor(0, r->size()), r);

    ElfHeader::pull();
}

template <typename T>
void ElfEhdr::M_initRecord(Record* r)
{
    enj_internal_assert(BadArgument, r);

    r->addField("ei_mag",        EI_MAG0, SELFMAG);
    r->addField("ei_class",      EI_CLASS, 1);
    r->addField("ei_data",       EI_DATA, 1);
    r->addField("ei_version",    EI_VERSION, 1);
    r->addField("ei_osabi",      EI_OSABI, 1);
    r->addField("ei_abiversion", EI_ABIVERSION, 1);
    r->addField("e_type",        &T::e_type);
    r->addField("e_machine",     &T::e_machine);
    r->addField("e_version",     &T::e_version);
    r->addField("e_entry",       &T::e_entry);
    r->addField("e_phoff",       &T::e_phoff);
    r->addField("e_shoff",       &T::e_shoff);
    r->addField("e_flags",       &T::e_flags);
    r->addField("e_ehsize",      &T::e_ehsize);
    r->addField("e_phentsize",   &T::e_phentsize);
    r->addField("e_phnum",       &T::e_phnum);
    r->addField("e_shentsize",   &T::e_shentsize);
    r->addField("e_shnum",       &T::e_shnum);
    r->addField("e_shstrndx",    &T::e_shstrndx);
}
