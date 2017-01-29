#include "elfninja/core/elf_shdr.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/error.h"

#include "elfninja/core/elf_table_impl.hpp"
#include "elfninja/core/elf_table_item_impl.hpp"

#include <elf.h>

using namespace enj;

ElfShdr::ElfShdr(size_t index, Elf* elf)
    : ElfTableItem<ElfHeader, ElfShdr>(index, elf)
{}

ElfShdr::~ElfShdr()
{}

void ElfShdr::pull()
{
    Record* r = new Record();
    if (is32bit())
        M_initRecord<Elf32_Shdr>(r);
    else
        M_initRecord<Elf64_Shdr>(r);

    M_setHeader(blob()->addCursor(offset(), r->size()), r);

    ElfHeader::pull();
}

void ElfShdr::update()
{
    ElfHeader::update();

    ElfShdr* shstrtab = elf()->shstrtab();
    if (shstrtab)
    {
        size_t name = get("sh_name");
        if (name < shstrtab->get("sh_size"))
        {
            m_name = "";
            for (size_t i = name; i < shstrtab->get("sh_size"); ++i)
            {
                char c;
                blob()->read(shstrtab->get("sh_offset") + i, (uint8_t*) &c, 1);
                if (c)
                    m_name += c;
                else
                    break;
            }
        }
    }
}

template <typename T>
void ElfShdr::M_initRecord(Record* r)
{
    enj_internal_assert(BadArgument, r);

    r->addField("sh_name",      &T::sh_name);
    r->addField("sh_type",      &T::sh_type);
    r->addField("sh_flags",     &T::sh_flags);
    r->addField("sh_addr",      &T::sh_addr);
    r->addField("sh_offset",    &T::sh_offset);
    r->addField("sh_size",      &T::sh_size);
    r->addField("sh_link",      &T::sh_link);
    r->addField("sh_info",      &T::sh_info);
    r->addField("sh_addralign", &T::sh_addralign);
    r->addField("sh_entsize",   &T::sh_entsize);
}
