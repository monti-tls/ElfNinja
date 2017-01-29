#ifndef __ELFNINJA_CORE_ELF_H__
#define __ELFNINJA_CORE_ELF_H__

#include "elfninja/core/blob.h"
#include "elfninja/core/elf_item.h"
#include "elfninja/core/elf_table.h"
#include "elfninja/core/elf_ehdr.h"
#include "elfninja/core/elf_phdr.h"
#include "elfninja/core/elf_shdr.h"

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

        bool is32bit() const;
        size_t shstrndx() const;
        ElfShdr* shstrtab() const;

        ElfEhdr* ehdr() const;
        ElfPhdr* phdrs() const;
        ElfShdr* shdrs() const;

        void pull();
        void update();

    private:
        void M_init();

    private:
        Blob* m_blob;
        bool m_owns_blob;
        ElfEhdr* m_ehdr;
        ElfTable<ElfPhdr>* m_phdrs;
        ElfTable<ElfShdr>* m_shdrs;
        bool m_is32bit;
        size_t m_shstrndx;
        ElfShdr* m_shstrtab;
    };
}

#endif // __ELFNINJA_CORE_ELF_H__
