#ifndef __ELFNINJA_CORE_DATA_H__
#define __ELFNINJA_CORE_DATA_H__

#include "elfninja/core/blob.h"
#include "elfninja/core/record.h"

#include <cstddef>
#include <cstdint>

namespace enj
{
    class Data
    {
    public:
        Data(Data const&) = delete;
        Data(Blob::Cursor* cursor, Record* record);
        ~Data();

        Blob::Cursor* cursor() const;
        Record* record() const;

        void pull();
        void push() const;

    private:
        void M_manageScratch();

    private:
        Blob::Cursor* m_cursor;
        Record* m_record;
        uint8_t* m_scratch;
        size_t m_scratch_size;
    };
}

#endif // __ELFNINJA_CORE_DATA_H__
