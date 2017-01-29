#ifndef __ELFNINJA_CORE_ELF_TABLE_IMPL_HPP__
#define __ELFNINJA_CORE_ELF_TABLE_IMPL_HPP__

#include "elfninja/core/elf_table.h"
#include "elfninja/core/elf.h"
#include "elfninja/core/error.h"

namespace enj
{
    template <typename T>
    ElfTable<T>::ElfTable(Elf* elf)
        : ElfItem(elf)
        , m_table(0)
    {}

    template <typename T>
    ElfTable<T>::~ElfTable()
    {
        if (m_table)
            delete m_table;
    }

    template <typename T>
    void ElfTable<T>::pull(size_t offset, size_t entsize, size_t count)
    {
        while (childrenCount())
            M_removeChild(children().front());

        if (m_table)
            delete m_table;

        m_table = new Piece(blob()->addCursor(offset, count * entsize));

        for (size_t i = 0; i < count; ++i)
            M_addChild(new T(i, elf()));

        ElfItem::pull();
    }

    template <typename T>
    size_t ElfTable<T>::count() const
    {
        enj_assert(BadOperation, m_table);
        enj_internal_assert(NullPointer, m_table->cursor());

        return childrenCount();
    }

    template <typename T>
    size_t ElfTable<T>::offset() const
    {
        enj_assert(BadOperation, m_table);
        enj_internal_assert(NullPointer, m_table->cursor());

        return m_table->cursor()->start()->pos();
    }

    template <typename T>
    size_t ElfTable<T>::entsize() const
    {
        enj_assert(BadOperation, m_table);
        enj_internal_assert(NullPointer, m_table->cursor());

        if (!size())
            return 0;

        size_t c = count();
        return c ? size() / c : 0;
    }

    template <typename T>
    size_t ElfTable<T>::size() const
    {
        enj_assert(BadOperation, m_table);
        enj_internal_assert(NullPointer, m_table->cursor());

        return m_table->cursor()->length();
    }

    template <typename T>
    T* ElfTable<T>::at(size_t index) const
    {
        return (T*) child(index);
    }
}

#endif // __ELFNINJA_CORE_ELF_TABLE_IMPL_HPP__