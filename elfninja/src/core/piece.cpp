#include "elfninja/core/piece.h"
#include "elfninja/core/blob.h"
#include "elfninja/core/error.h"

#include <cstring>

using namespace enj;

Piece::Piece(Blob::Cursor* cursor, Data* data)
    : m_cursor(cursor)
    , m_data(data)
{
    enj_assert(BadArgument, cursor);
}

Piece::~Piece()
{
    enj_internal_assert(NullPointer, m_cursor);

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
    m_data->readFrom(m_cursor);
}

void Piece::write() const
{
    m_data->writeTo(m_cursor);
}
