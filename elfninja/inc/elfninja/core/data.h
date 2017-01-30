#ifndef __ELFNINJA_CORE_DATA_H__
#define __ELFNINJA_CORE_DATA_H__

#include "elfninja/core/blob.h"

#include <cstddef>
#include <cstdint>

namespace enj
{
    class Data
    {
    public:
        virtual ~Data();

        virtual size_t size() const = 0;
        virtual void writeTo(Blob::Cursor* c) const = 0;
        virtual void readFrom(Blob::Cursor* c) = 0;
    };
}

#endif // __ELFNINJA_CORE_DATA_H__
