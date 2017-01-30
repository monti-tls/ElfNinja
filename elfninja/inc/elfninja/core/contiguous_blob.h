#ifndef __ELFNINJA_CONTIGUOUS_BLOB_H__
#define __ELFNINJA_CONTIGUOUS_BLOB_H__

#include "elfninja/core/blob.h"

namespace enj
{
    class ContiguousBlob : public Blob
    {
    public:
        ContiguousBlob(size_t chunk_size = 128);
        virtual ~ContiguousBlob();

        void write(size_t pos, uint8_t const* data, size_t size);
        void read(size_t pos, uint8_t* data, size_t size) const;
        void insert(size_t pos, uint8_t const* data, size_t size);
        void remove(size_t pos, size_t size);
        size_t size() const;

    private:
        void M_resize(size_t size);
        void M_shrink();
        void M_grow();

    private:
        size_t m_chunk_size;
        size_t m_chunks;
        uint8_t* m_buffer;
        size_t m_size;
    };
}

#endif // __ELFNINJA_CONTIGUOUS_BLOB_H__
