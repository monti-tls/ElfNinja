#ifndef __ELFNINJA_CORE_ELF_H__
#define __ELFNINJA_CORE_ELF_H__

#include "elfninja/core/blob.h"
#include "elfninja/core/elf_item.h"
#include "elfninja/core/elf_ehdr.h"

#include <cstddef>
#include <cstdint>

namespace enj
{
    class Elf : public ElfItem
    {
    public:
        Elf(std::string const& filename);
        Elf(std::istream const& stream);
        Elf(int fd);
        Elf(Blob* blob);
        ~Elf();

        Blob* blob() const;
        ElfEhdr* ehdr() const;
        bool is32bit() const;

        void pull();
        void update();

    private:
        void M_init();

    private:
        Blob* m_blob;
        bool m_owns_blob;
        ElfEhdr* m_ehdr;
        bool m_is32bit;
    };
}

#endif // __ELFNINJA_CORE_ELF_H__
