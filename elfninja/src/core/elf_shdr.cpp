#include "elfninja/core/elf_shdr.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/string.h"
#include "elfninja/core/error.h"

#include "elfninja/core/elf_table_impl.hpp"
#include "elfninja/core/elf_table_item_impl.hpp"

#include <elf.h>

using namespace enj;

ElfShdr::ElfShdr(size_t index, Elf* elf)
    : ElfTableItem<ElfHeader, ElfShdr>(index, elf)
    , m_name(0)
    , m_data(0)
{}

ElfShdr::~ElfShdr()
{
    delete m_name;
}

void ElfShdr::pull()
{
    /* Prepare the header record depending on ELF class */

    Record* r = new Record();
    if (is32bit())
        M_initRecord<Elf32_Shdr>(r);
    else
        M_initRecord<Elf64_Shdr>(r);

    /* Setup the header record and pull + update */

    M_setHeader(blob()->addCursor(offset(), r->size()), r);
    ElfHeader::pull();

    /* Setup data cursor */

    if (m_data)
    {
        blob()->removeCursor(m_data);
        m_data = 0;
    }

    size_t offset = get("sh_offset");
    size_t size = get("sh_size");

    if (offset)
    {
        if (offset + size <= blob()->size())
            m_data = blob()->addCursor(offset, size);
        else
            ; //TODO: raise anomaly
    }
}

void ElfShdr::update()
{
    /* Update header stuff */

    ElfHeader::update();

    /* Update the section name */

    delete m_name;
    m_name = 0;

    ElfShdr* shstrtab = elf()->shstrtab();
    if (shstrtab)
    {
        size_t name_off = get("sh_name");
        if (name_off < shstrtab->get("sh_size"))
        {
            m_name = new Piece(String::cursor(elf(), shstrtab, name_off), new String());
            m_name->read();
        }
        else
            ; //TODO: raise 'invalid name offset' anomaly
    }
}

void ElfShdr::push()
{
    /* Push the section name */

    if (m_name)
    {
        enj_internal_assert(NullPointer, elf()->shstrtab());

        m_name->write();

        set("sh_name", m_name->cursor()->start()->pos() - elf()->shstrtab()->data()->start()->pos());
    }
    else
        set("sh_name", 0);

    /* Update data offset and size */

    if (m_data)
    {
        set("sh_offset", m_data->start()->pos());
        set("sh_size", m_data->length());
    }
    else
    {
        set("sh_offset", 0);
        set("sh_size", 0);
    }

    //TODO: manage sh_addr ?

    /* Push the header stuff */

    ElfHeader::push();
}

std::string const& ElfShdr::getName() const
{
    enj_assert(BadOperation, m_name);

    return m_name->as<String>()->get();
}

void ElfShdr::setName(std::string const& name)
{
    enj_assert(BadOperation, m_name);

    return m_name->as<String>()->set(name);
}

Blob::Cursor* ElfShdr::data() const
{
    enj_assert(BadOperation, m_data);

    return m_data;
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
