#include <iostream>

#include "elfninja/core/error.h"
#include "elfninja/core/contiguous_blob.h"

int main(int argc, char** argv)
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
