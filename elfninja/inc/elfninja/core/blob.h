#ifndef __ELFNINJA_BLOB_H__
#define __ELFNINJA_BLOB_H__

#include <cstddef>
#include <cstdint>
#include <list>

namespace enj
{
    class Blob
    {
    public:
        Blob();
        virtual ~Blob();

        virtual void write(size_t pos, uint8_t const* data, size_t size) = 0;
        virtual void read(size_t pos, uint8_t* data, size_t size) const = 0;
        virtual void insert(size_t pos, uint8_t const* data, size_t size) = 0;
        virtual void remove(size_t pos, size_t size) = 0;
        virtual size_t size() const = 0;
    };
}

#endif // __ELFNINJA_BLOB_H__
