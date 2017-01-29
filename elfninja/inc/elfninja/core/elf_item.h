#ifndef __ELFNINJA_CORE_ELF_ITEM_H__
#define __ELFNINJA_CORE_ELF_ITEM_H__

#include "elfninja/core/blob.h"

#include <cstddef>
#include <list>

namespace enj
{
    class Elf;

    class ElfItem
    {
    public:
        ElfItem(Elf* elf);
        virtual ~ElfItem();

        Elf* elf() const;
        Blob* blob() const;
        bool is32bit() const;

        ElfItem* parent() const;
        std::list<ElfItem*> const& children() const;
        size_t childrenCount() const;

        ElfItem* prev() const;
        ElfItem* next() const;

        virtual void read();
        virtual void write() const;

        virtual void pull();
        virtual void update();
        virtual void push() const;

    protected:
        void M_addChild(ElfItem* item);
        void M_detachChild(ElfItem* item);
        void M_removeChild(ElfItem* item);

    private:
        Elf* m_elf;
        ElfItem* m_parent;
        std::list<ElfItem*> m_children;
    };
}

#endif // __ELFNINJA_CORE_ELF_ITEM_H__
