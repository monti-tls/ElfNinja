#include "elfninja/core/record.h"
#include "elfninja/core/error.h"

#include <cstring>

using namespace enj;

Record::Record()
    : m_scratch(0)
    , m_scratch_size(0)
{}

Record::~Record()
{
    delete[] m_scratch;

    for (auto it : m_fields)
        delete it.second;
}

size_t Record::size() const
{
    size_t size = 0;

    for (auto it : m_fields)
    {
        Field* f = it.second;
        enj_internal_assert(NullPointer, f);

        size_t end = f->offset() + f->size();
        if (end > size)
            size = end;
    }

    return size;
}

void Record::writeTo(Blob::Cursor* c) const
{
    enj_assert(BadArgument, c);
    enj_assert(SizeMismatch, c->length() == size());
    enj_internal_assert(NullPointer, c->blob());

    const_cast<Record*>(this)->M_manageScratch();

    for (auto it : m_fields)
    {
        Field* f = it.second;
        enj_internal_assert(NullPointer, f);

        f->writeTo(m_scratch + f->offset());
    }

    c->blob()->write(c->start()->pos(), m_scratch, size());
}

void Record::readFrom(Blob::Cursor* c)
{
    enj_assert(BadArgument, c);
    enj_assert(SizeMismatch, c->length() == size());
    enj_internal_assert(NullPointer, c->blob());

    M_manageScratch();

    c->blob()->read(c->start()->pos(), m_scratch, size());

    for (auto it : m_fields)
    {
        Field* f = it.second;
        enj_internal_assert(NullPointer, f);

        f->readFrom(m_scratch + f->offset());
    }
}

void Record::addField(std::string const& name, size_t offset, size_t size, FieldSignedness signedness)
{
    enj_assert(AlreadyExists, m_fields.find(name) == m_fields.end());

    for (auto it : m_fields)
    {
        Field* f = it.second;
        enj_internal_assert(NullPointer, f);

        enj_assert(Overlap, offset < f->offset() || offset >= f->offset() + f->size());
    }

    m_fields[name] = new Field(offset, size, signedness);
}

void Record::removeField(std::string const& name)
{
    auto it = m_fields.find(name);
    enj_assert(DoesntExists, it != m_fields.end());

    delete it->second;
    m_fields.erase(it);
}

Record::Field const& Record::field(std::string const& name) const
{
    auto it = m_fields.find(name);
    enj_assert(DoesntExists, it != m_fields.end());

    Field* f = it->second;
    enj_internal_assert(NullPointer, f);

    return *f;
}

size_t Record::get(std::string const& name) const
{
    auto it = m_fields.find(name);
    enj_assert(DoesntExists, it != m_fields.end());

    Field* f = it->second;
    enj_internal_assert(NullPointer, f);

    return f->get();
}

void Record::set(std::string const& name, size_t value)
{
    auto it = m_fields.find(name);
    enj_assert(DoesntExists, it != m_fields.end());

    Field* f = it->second;
    enj_internal_assert(NullPointer, f);

    f->set(value);
}

void Record::M_manageScratch()
{
    if (m_scratch_size != size())
    {
        delete[] m_scratch;
        m_scratch = new uint8_t[size()];
        memset(m_scratch, 0, size());
    }
}

size_t Record::Field::offset() const
{
    return m_offset;
}

size_t Record::Field::size() const
{
    return m_size;
}

Record::FieldSignedness Record::Field::signedness() const
{
    return m_signedness;
}

void Record::Field::writeTo(uint8_t* data) const
{
    enj_assert(BadArgument, data);

    std::memcpy(data, &m_value, m_size);
}

void Record::Field::readFrom(uint8_t const* data)
{
    enj_assert(BadArgument, data);

    m_value = 0;
    std::memcpy(&m_value, data, m_size);

    //TODO: extend sign ?
}

size_t Record::Field::get() const
{
    return m_value;
}

void Record::Field::set(size_t value)
{
    m_value = value;
}

Record::Field::Field(size_t offset, size_t size, FieldSignedness signedness)
    : m_offset(offset)
    , m_size(size)
    , m_signedness(signedness)
    , m_value(0)
{}

Record::Field::~Field()
{}
