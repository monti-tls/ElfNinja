#ifndef __ELFNINJA_CORE_ELF_EHDR_H__
#define __ELFNINJA_CORE_ELF_EHDR_H__

#include "elfninja/core/elf_header.h"
#include "elfninja/core/record.h"

#include <cstddef>

namespace enj
{
    class ElfEhdr : public ElfHeader
    {
    public:
        ElfEhdr(Elf* elf);
        ~ElfEhdr();

        void pull();

    private:
        template <typename T>
        void M_initRecord(Record* r);
    };
}

#endif // __ELFNINJA_CORE_ELF_EHDR_H__
