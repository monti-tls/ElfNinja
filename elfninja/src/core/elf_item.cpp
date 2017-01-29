#include "elfninja/core/elf_item.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/error.h"

#include <algorithm>

using namespace enj;

ElfItem::ElfItem(Elf* elf)
    : m_elf(elf)
    , m_parent(0)
{
    enj_assert(BadArgument, elf);
}

ElfItem::~ElfItem()
{
    for (auto child : m_children)
        delete child;
}

Elf* ElfItem::elf() const
{
    return m_elf;
}

Blob* ElfItem::blob() const
{
    enj_internal_assert(NullPointer, m_elf);

    return m_elf->blob();
}

bool ElfItem::is32bit() const
{
    enj_internal_assert(NullPointer, m_elf);

    return m_elf->is32bit();
}

ElfItem* ElfItem::parent() const
{
    return m_parent;
}

std::list<ElfItem*> const& ElfItem::children() const
{
    return m_children;
}

size_t ElfItem::childrenCount() const
{
    return m_children.size();
}

ElfItem* ElfItem::prev() const
{
    if (!m_parent)
        return 0;

    auto it = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
    enj_internal_assert(Internal, it != m_parent->m_children.end());

    if (it == m_parent->m_children.begin())
        return 0;

    return *it--;
}

ElfItem* ElfItem::next() const
{
    if (!m_parent)
        return 0;

    auto it = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
    enj_internal_assert(Internal, it != m_parent->m_children.end());

    ++it;

    if (it == m_parent->m_children.end())
        return 0;

    return *it;
}

void ElfItem::read()
{
    for (auto it : m_children)
        it->read();
}

void ElfItem::write() const
{
    for (auto it : m_children)
        it->write();
}

void ElfItem::pull()
{
    for (auto it : m_children)
        it->pull();
}

void ElfItem::update()
{
    for (auto it : m_children)
        it->update();
}

void ElfItem::push() const
{
    for (auto it : m_children)
        it->push();
}

void ElfItem::M_addChild(ElfItem* item)
{
    enj_assert(BadArgument, item);
    enj_assert(AlreadyExists,std::find(m_children.begin(), m_children.end(), item) == m_children.end());
    enj_internal_assert(Internal, !item->m_parent);

    m_children.push_back(item);
    item->m_parent = this;
}

void ElfItem::M_detachChild(ElfItem* item)
{
    enj_assert(BadArgument, item);

    auto it = std::find(m_children.begin(), m_children.end(), item);
    enj_assert(DoesntExists, it != m_children.end());
    enj_internal_assert(Internal, item->m_parent == this);

    m_children.erase(it);

    item->m_parent = 0;
}

void ElfItem::M_removeChild(ElfItem* item)
{
    enj_assert(BadArgument, item);

    M_detachChild(item);
    delete item;
}
