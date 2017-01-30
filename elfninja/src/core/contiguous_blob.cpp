#include "elfninja/core/contiguous_blob.h"
#include "elfninja/core/error.h"

#include <cstring>

using namespace enj;

ContiguousBlob::ContiguousBlob(size_t chunk_size)
    : m_chunk_size(chunk_size)
    , m_chunks(0)
    , m_buffer(0)
    , m_size(0)
{}

ContiguousBlob::~ContiguousBlob()
{
    std::free(m_buffer);
}

void ContiguousBlob::write(size_t pos, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, pos + size <= m_size);
    enj_assert(BadArgument, data);

    std::memcpy(m_buffer + pos, data, size);
}

void ContiguousBlob::read(size_t pos, uint8_t* data, size_t size) const
{
    enj_assert(BadArgument, pos + size <= m_size);
    enj_assert(BadArgument, data);

    std::memcpy(data, m_buffer + pos, size);
}

void ContiguousBlob::insert(size_t pos, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, pos <= m_size);

    size_t old_size = m_size;
    M_resize(m_size + size);

    std::memmove(m_buffer + pos + size, m_buffer + pos, old_size - pos);

    if (data)
        std::memcpy(m_buffer + pos, data, size);
    else
        std::memset(m_buffer + pos, 0, size);
}

void ContiguousBlob::remove(size_t pos, size_t size)
{
    enj_assert(BadArgument, pos + size <= m_size);

    std::memmove(m_buffer + pos, m_buffer + pos + size, m_size - (pos + size));
    M_resize(m_size - size);
}

size_t ContiguousBlob::size() const
{
    return m_size;
}

void ContiguousBlob::M_resize(size_t size)
{
    if (size < m_size)
    {
        while (m_chunks * m_chunk_size - size >= m_chunk_size)
            M_shrink();
    }
    else if (size > m_size)
    {
        while (m_chunks * m_chunk_size < size)
            M_grow();
    }

    m_size = size;
}

void ContiguousBlob::M_shrink()
{
    enj_internal_assert(Bounds, m_chunks);

    m_buffer = (uint8_t*) std::realloc(m_buffer, --m_chunks * m_chunk_size);
    enj_assert(Malloc, m_chunks && m_buffer);
}

void ContiguousBlob::M_grow()
{
    m_buffer = (uint8_t*) std::realloc(m_buffer, ++m_chunks * m_chunk_size);
    enj_assert(Malloc, m_buffer);
}
