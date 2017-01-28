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
        class Anchor;
        class Cursor;

    public:
        virtual ~Blob();

        size_t size() const;
        void write(size_t pos, uint8_t const* data, size_t size);
        void read(size_t pos, uint8_t* data, size_t size) const;

        void insert(size_t pos, uint8_t const* data, size_t size);
        void insertAfter(Anchor* a, uint8_t const* data, size_t size);
        void insertBefore(Anchor* a, uint8_t const* data, size_t size);
        void insertAfterStart(Cursor* c, uint8_t const* data, size_t size);
        void insertBeforeStart(Cursor* c, uint8_t const* data, size_t size);
        void insertAfterEnd(Cursor* c, uint8_t const* data, size_t size);
        void insertBeforeEnd(Cursor* c, uint8_t const* data, size_t size);

        void remove(size_t pos, size_t size);
        void removeAfter(Anchor* a, size_t size);
        void removeBefore(Anchor* a, size_t size);
        void removeAfterStart(Cursor* c, size_t size);
        // void removeBeforeStart(Cursor* c, size_t size);
        void removeAfterEnd(Cursor* c, size_t size);
        // void removeBeforeEnd(Cursor* c, size_t size);

        Anchor* addAnchor(size_t pos);
        void removeAnchor(Anchor* a);

        Cursor* addCursor(size_t pos, size_t size);
        void removeCursor(Cursor* c);

    protected:
        virtual void M_write(size_t pos, uint8_t const* data, size_t size) = 0;
        virtual void M_read(size_t pos, uint8_t* data, size_t size) const = 0;
        virtual void M_insert(size_t pos, uint8_t const* data, size_t size) = 0;
        virtual void M_remove(size_t pos, size_t size) = 0;
        virtual size_t M_size() const = 0;

    private:
        void M_lshiftAnchors(Anchor* before, Anchor* after, size_t pos, size_t amount);
        void M_rshiftAnchors(Anchor* before, Anchor* after, size_t pos, size_t amount);
        void M_updateCursors();

    private:
        std::list<Anchor*> m_anchors;
        std::list<Cursor*> m_cursors;
    };

    class Blob::Anchor
    {
        friend class Blob;

    public:
        size_t pos() const;

    private:
        Anchor();
        ~Anchor();

        size_t m_pos;
    };

    class Blob::Cursor
    {
        friend class Blob;

    public:
        Anchor* start() const;
        Anchor* end() const;
        size_t length() const;

    private:
        Cursor();
        ~Cursor();

        void update();

        Anchor* m_start;
        Anchor* m_end;
        size_t m_length;
    };
}

#endif // __ELFNINJA_BLOB_H__
