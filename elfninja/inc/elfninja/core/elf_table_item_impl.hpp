#ifndef __ELFNINJA_CORE_ELF_TABLE_ITEM_IMPL_HPP__
#define __ELFNINJA_CORE_ELF_TABLE_ITEM_IMPL_HPP__

#include "elfninja/core/elf_table_item.h"

namespace enj
{
    template <typename Base, typename Derived>
    template <typename... Args>
    ElfTableItem<Base, Derived>::ElfTableItem(size_t index, Args const&... args)
        : Base(args...)
        , m_index(index)
    {}

    template <typename Base, typename Derived>
    ElfTableItem<Base, Derived>::~ElfTableItem()
    { }

    template <typename Base, typename Derived>
    size_t ElfTableItem<Base, Derived>::index() const
    { return m_index; }

    template <typename Base, typename Derived>
    size_t ElfTableItem<Base, Derived>::offset() const
    { return table()->offset() + m_index * table()->entsize(); }

    template <typename Base, typename Derived>
    ElfTable<Derived>* ElfTableItem<Base, Derived>::table() const
    { return (ElfTable<Derived>*) Base::parent(); }
}

#endif // __ELFNINJA_CORE_ELF_TABLE_ITEM_IMPL_HPP__
