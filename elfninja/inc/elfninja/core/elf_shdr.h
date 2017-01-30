#ifndef __ELFNINJA_CORE_ELF_SHDR_H__
#define __ELFNINJA_CORE_ELF_SHDR_H__

#include "elfninja/core/elf_header.h"
#include "elfninja/core/elf_table_item.h"
#include "elfninja/core/piece.h"

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
        void push();

        std::string const& getName() const;
        void setName(std::string const& name);
        Blob::Cursor* data() const;

    private:
        template <typename T>
        void M_initRecord(Record* r);

    private:
        Piece* m_name;
        Blob::Cursor* m_data;
    };
}

#endif // __ELFNINJA_CORE_ELF_SHDR_H__
