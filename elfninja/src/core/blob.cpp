#include "elfninja/core/blob.h"
#include "elfninja/core/error.h"

#include <algorithm>

using namespace enj;

Blob::Blob()
{}

Blob::~Blob()
{
    for (Cursor* c : m_cursors)
        delete c;

    for (Anchor* a : m_anchors)
        delete a;
}

size_t Blob::size() const
{
    return M_size();
}

void Blob::write(size_t pos, uint8_t const* data, size_t size)
{
    enj_assert(Bounds, pos + size <= M_size());

    M_write(pos, data, size);
}

void Blob::read(size_t pos, uint8_t* data, size_t size) const
{
    enj_assert(Bounds, pos + size <= M_size());

    M_read(pos, data, size);
}

void Blob::insert(size_t pos, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, data);
    enj_assert(Bounds, pos <= M_size());

    M_insert(pos, data, size);
    M_rshiftAnchors(0, 0, pos, size);
    M_updateCursors();
}

void Blob::insertAfter(Anchor* a, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, a && data && a->blob() == this);
    enj_assert(Bounds, a->pos() <= M_size());

    M_insert(a->pos(), data, size);
    M_rshiftAnchors(0, a, a->pos(), size);
    M_updateCursors();
}

void Blob::insertBefore(Anchor* a, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, a && data && a->blob() == this);
    enj_assert(Bounds, a->pos() <= M_size());

    M_insert(a->pos(), data, size);
    M_rshiftAnchors(a, 0, a->pos(), size);
    M_updateCursors();
}

void Blob::insertAfterStart(Cursor* c, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, c && data && c->blob() == this);
    enj_internal_assert(NullPointer, c->start() && c->end());
    enj_assert(Bounds, c->start()->pos() <= M_size());

    M_insert(c->start()->pos(), data, size);
    M_rshiftAnchors(c->end(), c->start(), c->start()->pos(), size);
}

void Blob::insertBeforeStart(Cursor* c, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, c && data && c->blob() == this);
    enj_internal_assert(NullPointer, c->start() && c->end());
    enj_assert(Bounds, c->start()->pos() <= M_size());

    M_insert(c->start()->pos(), data, size);
    M_rshiftAnchors(c->start(), 0, c->start()->pos(), size);
}

void Blob::insertAfterEnd(Cursor* c, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, c && data && c->blob() == this);
    enj_internal_assert(NullPointer, c->end() && c->end());
    enj_assert(Bounds, c->end()->pos() <= M_size());

    M_insert(c->end()->pos(), data, size);
    M_rshiftAnchors(0, c->end(), c->end()->pos(), size);
}

void Blob::insertBeforeEnd(Cursor* c, uint8_t const* data, size_t size)
{
    enj_assert(BadArgument, c && data && c->blob() == this);
    enj_internal_assert(NullPointer, c->end() && c->end());
    enj_assert(Bounds, c->end()->pos() <= M_size());

    M_insert(c->end()->pos(), data, size);
    M_rshiftAnchors(c->end(), 0, c->end()->pos(), size);
}

void Blob::remove(size_t pos, size_t size)
{
    enj_assert(Bounds, pos + size <= M_size());

    M_remove(pos, size);
    M_lshiftAnchors(0, 0, pos, size);
    M_updateCursors();
}

void Blob::removeAfter(Anchor* a, size_t size)
{
    enj_assert(BadArgument, a && a->blob() == this);
    enj_assert(Bounds, a->pos() + size <= M_size());

    M_remove(a->pos(), size);
    M_lshiftAnchors(0, a, a->pos(), size);
    M_updateCursors();
}

void Blob::removeBefore(Anchor* a, size_t size)
{
    enj_assert(BadArgument, a && a->blob() == this);
    enj_assert(Bounds, a->pos() + size <= M_size());

    M_remove(a->pos(), size);
    M_lshiftAnchors(a, 0, a->pos(), size);
    M_updateCursors();
}

void Blob::removeAfterStart(Cursor* c, size_t size)
{
    enj_assert(BadArgument, c && c->blob() == this);
    enj_internal_assert(NullPointer, c->start() && c->end());
    enj_assert(Bounds, c->start()->pos() + size <= M_size());

    M_remove(c->start()->pos(), size);
    M_lshiftAnchors(0, c->start(), c->start()->pos(), size);
    M_updateCursors();
}

/*void Blob::removeBeforeStart(Cursor* c, size_t size)
{
    enj_assert(BadArgument, c);
    enj_internal_assert(NullPointer, c->start() && c->end());
    enj_assert(Bounds, c->start()->pos() + size <= M_size());

    M_remove(c->start()->pos(), size);
    M_lshiftAnchors(c->start(), 0, c->start()->pos(), size);
    M_updateCursors();
}*/

void Blob::removeAfterEnd(Cursor* c, size_t size)
{
    enj_assert(BadArgument, c && c->blob() == this);
    enj_internal_assert(NullPointer, c->end() && c->end());
    enj_assert(Bounds, c->end()->pos() + size <= M_size());

    M_remove(c->end()->pos(), size);
    M_lshiftAnchors(0, c->end(), c->end()->pos(), size);
    M_updateCursors();
}

/*void Blob::removeBeforeEnd(Cursor* c, size_t size)
{
    enj_assert(BadArgument, c);
    enj_internal_assert(NullPointer, c->end() && c->end());
    enj_assert(Bounds, c->end()->pos() + size <= M_size());

    M_remove(c->end()->pos(), size);
    M_lshiftAnchors(c->end(), 0, c->end()->pos(), size);
    M_updateCursors();
}*/

Blob::Anchor* Blob::addAnchor(size_t pos)
{
    enj_assert(Bounds, pos <= M_size());

    Anchor* a = new Anchor(this);
    a->m_pos = pos;

    m_anchors.push_back(a);

    return a;
}

void Blob::removeAnchor(Anchor* a)
{
    enj_assert(BadArgument, a && a->blob() == this);

    auto it = std::find(m_anchors.begin(), m_anchors.end(), a);

    if (it != m_anchors.end())
    {
        m_anchors.erase(it);
        delete a;
    }
}

Blob::Cursor* Blob::addCursor(size_t pos, size_t size)
{
    enj_assert(Bounds, pos <= M_size());

    Cursor* c = new Cursor(this);
    c->m_start = addAnchor(pos);
    c->m_end = addAnchor(pos + size);
    c->update();

    m_cursors.push_back(c);

    return c;
}

void Blob::removeCursor(Cursor* c)
{
    enj_assert(BadArgument, c && c->blob() == this);

    auto it = std::find(m_cursors.begin(), m_cursors.end(), c);

    if (it != m_cursors.end())
    {
        removeAnchor(c->end());
        removeAnchor(c->start());

        m_cursors.erase(it);
        delete c;
    }
}

void Blob::M_lshiftAnchors(Blob::Anchor* before, Blob::Anchor* after, size_t pos, size_t amount)
{
    for (Anchor* a : m_anchors)
    {
        enj_internal_assert(NullPointer, a);

        // Default lshift policy is 'after'

        if (a->pos() > pos)
        {
            a->m_pos -= amount;
        }
        else if (a->pos() == pos)
        {
            if (a == before)
                a->m_pos -= amount;
        }
    }
}

void Blob::M_rshiftAnchors(Blob::Anchor* before, Blob::Anchor* after, size_t pos, size_t amount)
{
    for (Anchor* a : m_anchors)
    {
        enj_internal_assert(NullPointer, a);

        // Default rshift policy is 'after'

        if (a->pos() > pos)
        {
            a->m_pos += amount;
        }
        else if (a->pos() == pos)
        {
            if (a == before)
                a->m_pos += amount;
        }
    }
}

void Blob::M_updateCursors()
{
    for (Cursor* c : m_cursors)
    {
        enj_internal_assert(NullPointer, c);
        enj_internal_assert(Internal, c->blob() == this);

        c->update();
    }
}

Blob::Anchor::Anchor(Blob* blob)
    : m_blob(blob)
{
    enj_internal_assert(BadArgument, blob);
}

Blob::Anchor::~Anchor()
{}

Blob* Blob::Anchor::blob() const
{
    return m_blob;
}

size_t Blob::Anchor::pos() const
{
    return m_pos;
}

Blob::Cursor::Cursor(Blob* blob)
    : m_blob(blob)
{
    enj_internal_assert(BadArgument, blob);
}

Blob::Cursor::~Cursor()
{}

Blob* Blob::Cursor::blob() const
{
    return m_blob;
}

Blob::Anchor* Blob::Cursor::start() const
{
    return m_start;
}

Blob::Anchor* Blob::Cursor::end() const
{
    return m_end;
}

size_t Blob::Cursor::length() const
{
    return m_length;
}

void Blob::Cursor::update()
{
    if (!m_start || !m_end)
        throw std::runtime_error("Blob::Cursor::update: null pointer");
    else if (m_end->pos() < m_start->pos())
        throw std::runtime_error("Blob::Cursor::update: negative length");

    m_length = m_end->pos() - m_start->pos();
}
