#include "elfninja/core/piece.h"
#include "elfninja/core/blob.h"
#include "elfninja/core/error.h"

#include <cstring>

using namespace enj;

Piece::Piece(Blob::Cursor* cursor, Data* data)
    : m_cursor(cursor)
    , m_data(data)
    , m_scratch(0)
    , m_scratch_size(0)
{
    enj_assert(BadArgument, cursor && data);
}

Piece::~Piece()
{
    enj_internal_assert(NullPointer, m_cursor);

    delete[] m_scratch;
    delete m_data;
    m_cursor->blob()->removeCursor(m_cursor);
}

Blob::Cursor* Piece::cursor() const
{
    return m_cursor;
}

Data* Piece::data() const
{
    return m_data;
}

void Piece::read()
{
    enj_internal_assert(Internal, m_data->size() == m_cursor->length());

    M_manageScratch();
    enj_internal_assert(NullPointer, m_scratch);

    m_cursor->blob()->read(m_cursor->start()->pos(), m_scratch, m_data->size());
    m_data->readFrom(m_scratch);
}

void Piece::write() const
{
    enj_internal_assert(Internal, m_data->size() == m_cursor->length());

    const_cast<Piece*>(this)->M_manageScratch();
    enj_internal_assert(NullPointer, m_scratch);

    m_data->writeTo(m_scratch);
    m_cursor->blob()->write(m_cursor->start()->pos(), m_scratch, m_data->size());
}

void Piece::M_manageScratch()
{
    if (m_scratch_size != m_data->size())
    {
        delete[] m_scratch;
        m_scratch = new uint8_t[m_data->size()];
    }
}
