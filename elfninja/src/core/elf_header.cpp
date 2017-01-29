#include "elfninja/core/elf_header.h"
#include "elfninja/core/error.h"

#include <elf.h>

using namespace enj;

ElfHeader::ElfHeader(Elf* elf)
    : ElfItem(elf)
    , m_header(0)
{}

ElfHeader::~ElfHeader()
{
    delete m_header;
}

void ElfHeader::read()
{
    enj_assert(BadOperation, m_header);

    m_header->read();
    ElfItem::read();
}

void ElfHeader::write() const
{
    enj_assert(BadOperation, m_header);

    ElfItem::write();
    m_header->write();
}

void ElfHeader::pull()
{
    read();
    update();
}

void ElfHeader::push() const
{
    enj_assert(BadOperation, m_header);

    write();
}

void ElfHeader::set(std::string const& name, size_t value)
{
    enj_assert(BadOperation, m_header);

    m_header->as<Record>()->set(name, value);
}

size_t ElfHeader::get(std::string const& name) const
{
    enj_assert(BadOperation, m_header);

    return m_header->as<Record>()->get(name);
}

size_t ElfHeader::size(std::string const& name) const
{
    enj_assert(BadOperation, m_header);

    return m_header->as<Record>()->field(name).size();
}

Piece* ElfHeader::M_header() const
{
    return m_header;
}

void ElfHeader::M_setHeader(Blob::Cursor* cursor, Record* record)
{
    enj_internal_assert(BadArgument, cursor && record);

    delete m_header;
    m_header = new Piece(cursor, record);
}
