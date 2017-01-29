#ifndef __ELFNINJA_CORE_PIECE_H__
#define __ELFNINJA_CORE_PIECE_H__

#include "elfninja/core/blob.h"
#include "elfninja/core/data.h"

#include <cstddef>
#include <cstdint>

namespace enj
{
    class Piece
    {
    public:
        Piece(Piece const&) = delete;
        Piece(Blob::Cursor* cursor, Data* data);
        ~Piece();

        Blob::Cursor* cursor() const;
        Data* data() const;

        void read();
        void write() const;

        template <typename T>
        T* as() const
        { return (T*) m_data; }

    private:
        void M_manageScratch();

    private:
        Blob::Cursor* m_cursor;
        Data* m_data;
        uint8_t* m_scratch;
        size_t m_scratch_size;
    };
}

#endif // __ELFNINJA_CORE_PIECE_H__