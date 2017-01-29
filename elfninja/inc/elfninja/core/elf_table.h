#ifndef __ELFNINJA_CORE_ELF_TABLE_H__
#define __ELFNINJA_CORE_ELF_TABLE_H__

#include "elfninja/core/elf_item.h"
#include "elfninja/core/piece.h"

#include <cstddef>
#include <string>

namespace enj
{
    template <typename T>
    class ElfTable : public ElfItem
    {
    public:
        ElfTable(Elf* elf);
        virtual ~ElfTable();

        void pull(size_t offset, size_t entsize, size_t count);

        size_t count() const;
        size_t offset() const;
        size_t entsize() const;
        size_t size() const;

        T* at(size_t index) const;

    private:
        Piece* m_table;
    };
}

#endif // __ELFNINJA_CORE_ELF_TABLE_H__
