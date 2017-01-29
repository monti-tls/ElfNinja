#ifndef __ELFNINJA_CORE_RECORD_H__
#define __ELFNINJA_CORE_RECORD_H__

#include "elfninja/core/data.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <type_traits>

namespace enj
{
    class Record : public Data
    {
    public:
        enum FieldSignedness
        {
            SignedField,
            UnsignedField
        };

        class Field;

    public:
        Record();
        ~Record();

        size_t size() const;
        void writeTo(uint8_t* data) const;
        void readFrom(uint8_t const* data);

        void addField(std::string const& name, size_t offset, size_t size, FieldSignedness signedness = UnsignedField);
        void removeField(std::string const& name);

        Field const& field(std::string const& name) const;
        size_t get(std::string const& name) const;
        void set(std::string const& name, size_t value);

        template <typename T>
        void addField(std::string const& name, size_t offset)
        { addField(name, offset, sizeof(T), std::is_signed<T>::value ? SignedField : UnsignedField); }

        template <typename T, typename U>
        void addField(std::string const& name, T U::* member)
        { addField<T>(name, (size_t) &(((U const volatile*) 0)->*member)); }

    private:
        std::map<std::string, Field*> m_fields;
    };

    class Record::Field
    {
        friend class Record;

    public:
        size_t offset() const;
        size_t size() const;
        FieldSignedness signedness() const;

        void writeTo(uint8_t* data) const;
        void readFrom(uint8_t const* data);

        size_t get() const;
        void set(size_t value);

    private:
        Field(size_t offset, size_t size, FieldSignedness signedness);
        ~Field();

    private:
        size_t m_offset;
        size_t m_size;
        FieldSignedness m_signedness;
        size_t m_value;
    };
}

#endif // __ELFNINJA_CORE_RECORD_H__
