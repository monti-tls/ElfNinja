#include <iostream>
#include <elf.h>

#include "elfninja/core/error.h"
#include "elfninja/core/contiguous_blob.h"
#include "elfninja/core/record.h"
#include "elfninja/core/data.h"
#include "elfninja/core/elf_item.h"
#include "elfninja/core/elf_table.h"
#include "elfninja/core/elf_ehdr.h"
#include "elfninja/core/elf_phdr.h"
#include "elfninja/core/elf.h"

#include "elfninja/core/elf_table_impl.hpp"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    try
    {
        /* Read file contents */

        int fd = open("/bin/ls", O_RDONLY);
        size_t size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        uint8_t* file = new uint8_t[size];
        read(fd, file, size);
        close(fd);

        /* Get this into a blob */

        enj::Blob* blob = new enj::ContiguousBlob();
        blob->insert(0, file, size);

        delete[] file;

        /* Create ELF descriptor */

        enj::Elf* elf = new enj::Elf(blob);
        elf->pull();
        elf->update();

        /* Print some useful info */

        printf("e_entry = %08lX\n", elf->ehdr()->get("e_entry"));

        printf(".:: PHDRs ::.\n");
        for (enj::ElfPhdr* phdr = elf->phdrs(); phdr; phdr = (enj::ElfPhdr*) phdr->next())
        {
            printf("%08lX %08lX\n", phdr->get("p_offset"), phdr->get("p_vaddr"));
        }

        printf(".:: SHDRs ::.\n");
        for (enj::ElfShdr* shdr = elf->shdrs(); shdr; shdr = (enj::ElfShdr*) shdr->next())
        {
            printf("%08lX %08lX %s\n", shdr->get("sh_offset"), shdr->get("sh_size"), shdr->name().c_str());
        }

        elf->shdrs()->set("sh_type", 2);
        elf->push();

        /* Write output file */

        fd = creat("bin/ls", 0777);
        file = new uint8_t[blob->size()];
        blob->read(0, file, blob->size());
        write(fd, file, blob->size());
        delete[] file;

        /* Cleanup */

        delete elf;
        delete blob;
    }
    catch (enj::Error const& e)
    {
        std::cerr << "[" << e.location() << "]: " << e.description() << std::endl;
        return -1;
    }

    return 0;
}

int main_blob(int argc, char** argv)
{
    try
    {
        enj::Blob* b = new enj::ContiguousBlob(4);

        enj::Blob::Cursor* c = b->addCursor(0, 0);

        b->insertAfterStart(c, (uint8_t const*) "3456", 4);

        b->insertBeforeStart(c, (uint8_t const*) "AB", 2);
        b->insertAfterEnd(c, (uint8_t const*) "CD", 2);
        b->insertBeforeEnd(c, (uint8_t const*) "789", 3);
        b->insertAfterStart(c, (uint8_t const*) "012", 3);

        b->insert(0, (uint8_t const*) "__", 2);

        // Now should be __AB0123456789CD

        b->removeAfterEnd(c, 1);
        b->removeAfterStart(c, 2);

        char as_str[256];
        b->read(0, (uint8_t*) &as_str[0], b->size());
        for (size_t i = 0; i < b->size(); ++i)
            if (!as_str[i])
                as_str[i] = '*';

        std::cout << "Anchor = (" << c->start()->pos();
        std::cout << "=" << as_str[c->start()->pos()] << " -> ";
        std::cout << c->end()->pos();
        std::cout << "=" << as_str[c->end()->pos()] << ")" << std::endl;

        as_str[b->size()] = '\0';
        std::cout << "'" << &as_str[0] << "'" << std::endl;

        delete b;
    }
    catch (enj::Error const& e)
    {
        std::cerr << "[" << e.location() << "]: " << e.description() << std::endl;
        return -1;
    }

    return 0;
}
