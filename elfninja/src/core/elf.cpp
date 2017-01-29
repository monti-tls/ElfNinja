#include "elfninja/core/elf.h"
#include "elfninja/core/error.h"

#include "elfninja/core/elf_table_impl.hpp"

#include <elf.h>

using namespace enj;

Elf::Elf(Blob* blob)
    : ElfItem(this)
    , m_blob(blob)
    , m_owns_blob(false)
{
    enj_assert(BadArgument, blob);

    M_init();
}

Elf::~Elf()
{
    if (m_owns_blob)
        delete m_blob;
}

Blob* Elf::blob() const
{
    return m_blob;
}

bool Elf::is32bit() const
{
    return m_is32bit;
}

size_t Elf::shstrndx() const
{
    return m_shstrndx;
}

ElfShdr* Elf::shstrtab() const
{
    return m_shstrtab;
}

ElfEhdr* Elf::ehdr() const
{
    return m_ehdr;
}

ElfPhdr* Elf::phdrs() const
{
    if (m_phdrs && m_phdrs->count())
        return m_phdrs->at(0);

    return 0;
}

ElfShdr* Elf::shdrs() const
{
    if (m_shdrs && m_shdrs->count())
        return m_shdrs->at(0);

    return 0;
}

void Elf::pull()
{
    /* Manage EHDR */

    m_ehdr->pull();

    m_is32bit = ehdr()->get("ei_class") == ELFCLASS32;
    m_shstrndx = 0;
    m_shstrtab = 0;

    /* Manage PHDRs */

    size_t phoff = ehdr()->get("e_phoff");
    size_t phentsize = ehdr()->get("e_phentsize");
    size_t phnum = ehdr()->get("e_phnum");
    m_phdrs->pull(phoff, phentsize, phnum);

    /* Manage SHDRs */

    size_t shoff = ehdr()->get("e_shoff");
    size_t shentsize = ehdr()->get("e_shentsize");
    size_t shnum = ehdr()->get("e_shnum");
    m_shdrs->pull(shoff, shentsize, shnum);

    /* Update everything */

    update();
}

void Elf::update()
{
    /* Get eventual shstrtab */

    m_shstrndx = ehdr()->get("e_shstrndx");

    if (m_shstrndx > 0 && m_shstrndx < m_shdrs->count())
        m_shstrtab = m_shdrs->at(m_shstrndx);
    else
        m_shstrtab = 0; //TODO: raise flag for this inconsistency

    /* Update all elements */

    ElfItem::update();
}

void Elf::M_init()
{
    m_ehdr = new ElfEhdr(this);
    M_addChild(m_ehdr);

    m_phdrs = new ElfTable<ElfPhdr>(this);
    M_addChild(m_phdrs);

    m_shdrs = new ElfTable<ElfShdr>(this);
    M_addChild(m_shdrs);
}
