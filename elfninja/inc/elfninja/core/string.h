#ifndef __ELFNINJA_CORE_STRING_H__
#define __ELFNINJA_CORE_STRING_H__

#include "elfninja/core/data.h"

#include <cstddef>
#include <string>

namespace enj
{
    class Elf;
    class ElfShdr;

    class String : public Data
    {
    public:
        String();
        ~String();

        size_t size() const;
        void writeTo(Blob::Cursor* c) const;
        void readFrom(Blob::Cursor* c);

        std::string const& get() const;
        void set(std::string const& value);

        operator std::string const&() const;

    public:
        static Blob::Cursor* cursor(Elf* elf, ElfShdr* shdr, size_t index);

    private:
        std::string* m_value;
        bool m_muted;
    };
}

#endif // __ELFNINJA_CORE_STRING_H__
