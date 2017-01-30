#include "elfninja/core/string.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/elf_shdr.h"
#include "elfninja/core/error.h"

#include <cstring>

using namespace enj;

String::String()
    : m_value(0)
    , m_muted(false)
{}

String::~String()
{
    delete m_value;
}

size_t String::size() const
{
    enj_assert(BadOperation, m_value);
    return m_value->size();
}

void String::writeTo(Blob::Cursor* c) const
{
    enj_assert(BadArgument, c);
    enj_internal_assert(NullPointer, c->blob());
    enj_assert(BadOperation, m_value);

    if (!m_muted)
    {
        enj_assert(SizeMismatch, c->length() == (m_value->size() + 1));
        c->blob()->write(c->start()->pos(), (uint8_t const*) m_value->c_str(), m_value->size());
    }
    else
    {
        c->blob()->removeAfterStart(c, c->length());
        c->blob()->insertAfterStart(c, (uint8_t const*) m_value->c_str(), m_value->size() + 1);
    }
}

void String::readFrom(Blob::Cursor* c)
{
    enj_assert(BadArgument, c);
    enj_internal_assert(NullPointer, c->blob());

    if (m_value)
        m_value->clear();
    else
        m_value = new std::string();

    m_muted = false;

    char* temp = new char[c->length()];
    std::memset(temp, 0, c->length());
    c->blob()->read(c->start()->pos(), (uint8_t*) temp, c->length());
    *m_value = temp;
    delete[] temp;
}

std::string const& String::get() const
{
    enj_assert(BadOperation, m_value);
    return *m_value;
}

void String::set(std::string const& value)
{
    enj_assert(BadOperation, m_value);

    m_value->clear();
    m_muted = true;

    *m_value = value;
}

String::operator std::string const&() const
{
    return get();
}

Blob::Cursor* String::cursor(Elf* elf, ElfShdr* shdr, size_t index)
{
    enj_assert(BadArgument, elf && shdr);
    enj_assert(BadStringIndex, index < shdr->data()->length());
    enj_internal_assert(NullPointer, elf->blob());

    size_t offset = shdr->data()->start()->pos();
    size_t len = 0;
    char c;
    do
    {
        elf->blob()->read(offset + index + len, (uint8_t*) &c, 1);
        ++len;
    } while (c != '\0' && index + len <= shdr->data()->length());

    return elf->blob()->addCursor(offset + index, len);
}
