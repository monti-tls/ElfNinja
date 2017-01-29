#ifndef __ELFNINJA_CORE_DATA_H__
#define __ELFNINJA_CORE_DATA_H__

#include "elfninja/core/blob.h"
#include "elfninja/core/data.h"

#include <cstddef>
#include <cstdint>

namespace enj
{
    class Data
    {
    public:
        virtual ~Data();

        virtual size_t size() const = 0;
        virtual void writeTo(uint8_t* data) const = 0;
        virtual void readFrom(uint8_t const* data) = 0;
    };
}

#endif // __ELFNINJA_CORE_DATA_H__
