#include "elfninja/core/data.h"
#include "elfninja/core/blob.h"
#include "elfninja/core/error.h"

#include <cstring>

using namespace enj;

Data::Data(Blob::Cursor* cursor, Record* record)
    : m_cursor(cursor)
    , m_record(record)
    , m_scratch(0)
    , m_scratch_size(0)
{
    enj_assert(Argument, cursor && record);
}

Data::~Data()
{
    enj_internal_assert(NullPointer, m_cursor);

    delete[] m_scratch;
    delete m_record;
    m_cursor->blob()->removeCursor(m_cursor);
}

Blob::Cursor* Data::cursor() const
{
    return m_cursor;
}

Record* Data::record() const
{
    return m_record;
}

void Data::pull()
{
    enj_assert(Inconsistency, m_record->size() == m_cursor->length());

    M_manageScratch();
    enj_internal_assert(NullPointer, m_scratch);

    m_cursor->blob()->read(m_cursor->start()->pos(), m_scratch, m_record->size());
    m_record->readFrom(m_scratch);
}

void Data::push() const
{
    enj_assert(Inconsistency, m_record->size() == m_cursor->length());

    const_cast<Data*>(this)->M_manageScratch();
    enj_internal_assert(NullPointer, m_scratch);

    m_record->writeTo(m_scratch);
    m_cursor->blob()->write(m_cursor->start()->pos(), m_scratch, m_record->size());
}

void Data::M_manageScratch()
{
    if (m_scratch_size != m_record->size())
    {
        delete[] m_scratch;
        m_scratch = new uint8_t[m_record->size()];
    }
}
