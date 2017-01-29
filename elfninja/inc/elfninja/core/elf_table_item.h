#ifndef __ELFNINJA_CORE_ELF_TABLE_ITEM_H__
#define __ELFNINJA_CORE_ELF_TABLE_ITEM_H__

#include "elfninja/core/elf_header.h"
#include "elfninja/core/elf_table.h"
#include "elfninja/core/record.h"

#include <cstddef>

namespace enj
{
    template <typename Base, typename Derived>
    class ElfTableItem : public Base
    {
    public:
        template <typename... Args>
        ElfTableItem(size_t index, Args const&... args);
        virtual ~ElfTableItem();

        size_t index() const;
        size_t offset() const;
        ElfTable<Derived>* table() const;

    private:
        size_t m_index;
    };
}

#endif // __ELFNINJA_CORE_ELF_TABLE_ITEM_H__
