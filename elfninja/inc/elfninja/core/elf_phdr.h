#ifndef __ELFNINJA_CORE_ELF_PHDR_H__
#define __ELFNINJA_CORE_ELF_PHDR_H__

#include "elfninja/core/elf_header.h"
#include "elfninja/core/elf_table_item.h"
#include "elfninja/core/record.h"

#include <cstddef>

namespace enj
{
    class ElfPhdr : public ElfTableItem<ElfHeader, ElfPhdr>
    {
    public:
        ElfPhdr(size_t index, Elf* elf);
        ~ElfPhdr();

        void pull();

    private:
        template <typename T>
        void M_initRecord(Record* r);
    };
}

#endif // __ELFNINJA_CORE_ELF_PHDR_H__
