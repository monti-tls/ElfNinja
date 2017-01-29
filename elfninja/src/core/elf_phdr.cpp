#include "elfninja/core/elf_phdr.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/error.h"

#include "elfninja/core/elf_table_impl.hpp"
#include "elfninja/core/elf_table_item_impl.hpp"

#include <elf.h>

using namespace enj;

ElfPhdr::ElfPhdr(size_t index, Elf* elf)
    : ElfTableItem<ElfHeader, ElfPhdr>(index, elf)
{}

ElfPhdr::~ElfPhdr()
{}

void ElfPhdr::pull()
{
    Record* r = new Record();
    if (is32bit())
        M_initRecord<Elf32_Phdr>(r);
    else
        M_initRecord<Elf64_Phdr>(r);

    M_setHeader(blob()->addCursor(offset(), r->size()), r);

    ElfHeader::pull();
}

template <typename T>
void ElfPhdr::M_initRecord(Record* r)
{
    enj_internal_assert(BadArgument, r);

    r->addField("p_type",   &T::p_type);
    r->addField("p_offset", &T::p_offset);
    r->addField("p_vaddr",  &T::p_vaddr);
    r->addField("p_paddr",  &T::p_paddr);
    r->addField("p_filesz", &T::p_filesz);
    r->addField("p_memsz",  &T::p_memsz);
    r->addField("p_flags",  &T::p_flags);
    r->addField("p_align",  &T::p_align);
}
