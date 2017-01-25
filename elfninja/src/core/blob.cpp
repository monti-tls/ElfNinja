#include "elfninja/core/blob.h"

#include <algorithm>

using namespace enj;

Blob::~Blob()
{
    for (auto it : m_eventSubscribers)
        delete it;
}

void Blob::write(size_t pos, uint8_t const* buffer, size_t size)
{
    WriteEvent* event = new WriteEvent();
    event->pos = pos;
    event->buffer = buffer;
    event->size = size;

    M_event(event);
    M_notify(event);

    delete event;
}

void Blob::read(size_t pos, uint8_t* buffer, size_t size) const
{
    ReadEvent* event = new ReadEvent();
    event->pos = pos;
    event->buffer = buffer;
    event->size = size;

    const_cast<Blob*>(this)->M_event(event);
    const_cast<Blob*>(this)->M_notify(event);

    delete event;
}

void Blob::insert(size_t pos, uint8_t const* buffer, size_t size)
{
    InsertEvent* event = new InsertEvent();
    event->pos = pos;
    event->buffer = buffer;
    event->size = size;

    M_event(event);
    M_notify(event);

    delete event;
}

void Blob::remove(size_t pos, size_t size)
{
    RemoveEvent* event = new RemoveEvent();
    event->pos = pos;
    event->size = size;

    M_event(event);
    M_notify(event);

    delete event;
}

void Blob::move(size_t src, size_t dest, size_t size)
{
    MoveEvent* event = new MoveEvent();
    event->src = src;
    event->dest = dest;
    event->size = size;

    M_event(event);
    M_notify(event);

    delete event;
}

Blob::Anchor* Blob::addAnchor(size_t pos)
{
    Anchor* anchor = new Anchor(this, pos);
    M_addEventSubscriber(anchor);
    return anchor;
}

void Blob::removeAnchor(Blob::Anchor* anchor)
{
    M_removeEventSubscriber(anchor);
    delete anchor;
}

Blob::Cursor* Blob::addCursor(size_t pos, size_t size)
{
    Cursor* cursor = new Cursor(this, pos, size);
    M_addEventSubscriber(cursor);
    return cursor;
}

void Blob::removeCursor(Blob::Cursor* cursor)
{
    M_removeEventSubscriber(cursor);
    delete cursor;
}

void Blob::M_addEventSubscriber(Blob::EventSubscriber* subscriber)
{
    m_eventSubscribers.push_back(subscriber);
}

void Blob::M_removeEventSubscriber(Blob::EventSubscriber* subscriber)
{
    auto it = std::find(m_eventSubscribers.begin(), m_eventSubscribers.end(), subscriber);

    if (it != m_eventSubscribers.end())
        m_eventSubscribers.erase(it);
}

void Blob::M_notify(Event* event)
{
    for (auto it : m_eventSubscribers)
        it->event(event);
}

Blob::EventSubscriber::EventSubscriber(Blob* parent)
    : m_parent(parent)
{}

Blob::EventSubscriber::~EventSubscriber()
{}

Blob* Blob::EventSubscriber::parent() const
{
    return m_parent;
}

void Blob::EventSubscriber::event(Blob::Event* event)
{}

Blob::Anchor::Anchor(Blob* parent, size_t pos)
    : Blob::EventSubscriber(parent)
    , m_pos(pos)
{}

Blob::Anchor::~Anchor()
{}

size_t Blob::Anchor::pos() const
{
    return m_pos;
}

void Blob::Anchor::event(Blob::InsertEvent* event)
{

}

void Blob::Anchor::event(Blob::RemoveEvent* event)
{

}

void Blob::Anchor::event(Blob::MoveEvent* event)
{

}

Blob::Cursor::Cursor(Blob* parent, size_t pos, size_t size)
    : Blob::EventSubscriber(parent)
    , m_pos(pos)
    , m_size(size)
{}

Blob::Cursor::~Cursor()
{}

size_t Blob::Cursor::pos() const
{
    return m_pos;
}

size_t Blob::Cursor::size() const
{
    return m_size;
}

void Blob::Cursor::event(Blob::InsertEvent* event)
{

}

void Blob::Cursor::event(Blob::RemoveEvent* event)
{

}

void Blob::Cursor::event(Blob::MoveEvent* event)
{

}
