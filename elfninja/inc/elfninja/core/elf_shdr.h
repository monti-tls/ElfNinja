#ifndef __ELFNINJA_CORE_ELF_SHDR_H__
#define __ELFNINJA_CORE_ELF_SHDR_H__

#include "elfninja/core/elf_header.h"
#include "elfninja/core/elf_table_item.h"
#include "elfninja/core/record.h"

#include <cstddef>

namespace enj
{
    class ElfShdr : public ElfTableItem<ElfHeader, ElfShdr>
    {
    public:
        ElfShdr(size_t index, Elf* elf);
        ~ElfShdr();

        void pull();
        void update();

        std::string const& name() const
        { return m_name; }

    private:
        template <typename T>
        void M_initRecord(Record* r);

    private:
        std::string m_name;
    };
}

#endif // __ELFNINJA_CORE_ELF_SHDR_H__
