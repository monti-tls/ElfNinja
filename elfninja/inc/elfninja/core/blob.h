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
        struct Event;
        struct WriteEvent;
        struct ReadEvent;
        struct InsertEvent;
        struct RemoveEvent;
        struct MoveEvent;

        class EventSubscriber;
        class Anchor;
        class Cursor;

    public:
        virtual ~Blob();

        void write(size_t pos, uint8_t const* buffer, size_t size);
        void read(size_t pos, uint8_t* buffer, size_t size) const;
        void insert(size_t pos, uint8_t const* buffer, size_t size);
        void remove(size_t pos, size_t size);
        void move(size_t src, size_t dest, size_t size);

        Anchor* addAnchor(size_t pos);
        void removeAnchor(Anchor* anchor);

        Cursor* addCursor(size_t pos, size_t size);
        void removeCursor(Cursor* cursor);

        virtual size_t size() const = 0;

    protected:
        virtual void M_event(Blob::Event* evt) = 0;

    private:
        void M_addEventSubscriber(EventSubscriber* subscriber);
        void M_removeEventSubscriber(EventSubscriber* subscriber);
        void M_notify(Event* event);

    private:
        std::list<EventSubscriber*> m_eventSubscribers;
    };

    struct Blob::Event
    {};

    struct Blob::WriteEvent : public Blob::Event
    {
        size_t pos;
        uint8_t const* buffer;
        size_t size;
    };

    struct Blob::ReadEvent : public Blob::Event
    {
        size_t pos;
        uint8_t* buffer;
        size_t size;
    };

    struct Blob::InsertEvent : public Blob::Event
    {
        size_t pos;
        uint8_t const* buffer;
        size_t size;
    };

    struct Blob::RemoveEvent : public Blob::Event
    {
        size_t pos;
        size_t size;
    };

    struct Blob::MoveEvent : public Blob::Event
    {
        size_t src;
        size_t dest;
        size_t size;
    };

    class Blob::EventSubscriber
    {
    public:
        EventSubscriber(Blob* parent);
        virtual ~EventSubscriber();

        Blob* parent() const;

        virtual void event(Blob::InsertEvent* event) = 0;
        virtual void event(Blob::RemoveEvent* event) = 0;
        virtual void event(Blob::MoveEvent* event) = 0;
        virtual void event(Blob::Event* event);

    private:
        Blob* m_parent;
    };

    class Blob::Anchor : public Blob::EventSubscriber
    {
        friend class Blob;
    private:
        Anchor(Blob* parent, size_t pos);
        ~Anchor();

    public:
        size_t pos() const;

        void event(Blob::InsertEvent* event);
        void event(Blob::RemoveEvent* event);
        void event(Blob::MoveEvent* event);

    private:
        size_t m_pos;
    };

    class Blob::Cursor : public Blob::EventSubscriber
    {
        friend class Blob;
    private:
        Cursor(Blob* parent, size_t pos, size_t size);
        ~Cursor();

    public:
        size_t pos() const;
        size_t size() const;

        void event(Blob::InsertEvent* event);
        void event(Blob::RemoveEvent* event);
        void event(Blob::MoveEvent* event);

    private:
        size_t m_pos;
        size_t m_size;
    };
}

#endif // __ELFNINJA_BLOB_H__
