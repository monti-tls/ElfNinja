#include "elfninja/core/elf.h"
#include "elfninja/core/error.h"

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

ElfEhdr* Elf::ehdr() const
{
    return m_ehdr;
}

bool Elf::is32bit() const
{
    return m_is32bit;
}

void Elf::pull()
{
    ElfItem::pull();

    /* bla bla */

    update();
}

void Elf::update()
{
    ElfItem::update();

    m_is32bit = ehdr()->get("ei_class") == ELFCLASS32;
}

void Elf::M_init()
{
    m_ehdr = new ElfEhdr(this);
    M_addChild(m_ehdr);
}
