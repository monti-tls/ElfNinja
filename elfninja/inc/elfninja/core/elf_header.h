#ifndef __ELFNINJA_CORE_ELF_HEADER_H__
#define __ELFNINJA_CORE_ELF_HEADER_H__

#include "elfninja/core/elf_item.h"
#include "elfninja/core/piece.h"
#include "elfninja/core/record.h"
#include "elfninja/core/blob.h"

#include <cstddef>
#include <string>

namespace enj
{
    class ElfHeader : public ElfItem
    {
    public:
        ElfHeader(Elf* elf);
        virtual ~ElfHeader();

        void read();
        void write() const;

        void pull();
        void push() const;

        void set(std::string const& name, size_t value);
        size_t get(std::string const& name) const;
        size_t size(std::string const& name) const;

    protected:
        Piece* M_header() const;
        void M_setHeader(Blob::Cursor* cursor, Record* record);

    private:
        Piece* m_header;
    };
}

#endif // __ELFNINJA_CORE_ELF_HEADER_H__
