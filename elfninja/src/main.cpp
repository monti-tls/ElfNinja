#include <iostream>
#include <vector>
#include <map>
#include <algorithm>

#include "elfninja/core/error.h"
#include "elfninja/core/blob.h"
#include "elfninja/core/contiguous_blob.h"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>

namespace enj
{
    template <typename T>
    class INode
    {
    public:
        INode(T* parent = 0)
            : m_init_once(false)
            , m_parent(parent)
            , m_being_destructed(false)
        {}

        virtual ~INode()
        {
            m_being_destructed = true;

            if (m_parent && !m_parent->m_being_destructed)
                m_parent->M_detachChild((T*) this);

            for (auto it : m_children)
                delete it;
        }

        T* parent() const
        {
            enj_assert(BadOperation, m_init_once);
            return m_parent;
        }

        size_t childrenCount() const
        {
            enj_assert(BadOperation, m_init_once);
            return m_children.size();
        }

        T* child(size_t i) const
        {
            enj_assert(BadOperation, m_init_once);
            enj_assert(BadArgument, i < m_children.size());
            return m_children.at(i);
        }

        std::vector<T*> const& children() const
        {
            enj_assert(BadOperation, m_init_once);
            return m_children;
        }

    protected:
        void M_init()
        {
            enj_assert(BadOperation, !m_init_once);

            if (m_parent)
                m_parent->M_addChild(static_cast<T*>(this));

            m_init_once = true;
        }

        void M_addChild(T* child)
        {
            enj_assert(BadOperation, m_init_once);
            enj_assert(BadArgument, child);
            enj_assert(AlreadyExists, std::find(m_children.begin(), m_children.end(), child) == m_children.end());

            m_children.push_back(child);

            M_updateNode();
        }

        void M_detachChild(T* child)
        {
            enj_assert(BadOperation, m_init_once);
            enj_assert(BadArgument, child);
            auto it = std::find(m_children.begin(), m_children.end(), child);
            enj_assert(DoesntExists, it != m_children.end());
            m_children.erase(it);
            child->m_parent = 0;

            M_updateNode();
        }

        void M_removeChild(T* child)
        {
            enj_assert(BadOperation, m_init_once);
            M_detachChild(child);
            delete child;
        }

        std::vector<T*>& M_children()
        {
            enj_assert(BadOperation, m_init_once);
            return m_children;
        }

        virtual void M_updateNode()
        {
            enj_assert(BadOperation, m_init_once);
        }

    private:
        bool m_init_once;
        T* m_parent;
        std::vector<T*> m_children;
        bool m_being_destructed;
    };

    template <typename T>
    class DPointer : public INode<T>
    {
    public:
        enum State
        {
            Leaf,
            Managed,
            Invalid
        };

    protected:
        enum EventType
        {
            Expand,
            Shrink,
            MoveUp,
            MoveDown
        };

        struct Event
        {
            EventType type;
            size_t offset;
            size_t count;
        };

    public:
        DPointer(Blob* blob, size_t offset, size_t size, T* parent = 0)
            : INode<T>(parent)
            , m_state(Leaf)
            , m_blob(blob)
            , m_offset(offset)
            , m_size(size)
        {
            INode<T>::M_init();
            enj_assert(BadArgument, blob);
        }

        ~DPointer()
        {}

        State state() const
        { return m_state; }

        Blob* blob() const
        { return m_blob; }

        size_t offset() const
        { return m_offset; }

        size_t size() const
        { return m_size; }

        void write(uint8_t const* data)
        {
            enj_assert(BadOperation, m_state == Leaf);
            enj_assert(BadArgument, data);

            m_blob->write(m_offset, data, m_size);
        }

        void read(uint8_t* data) const
        {
            enj_assert(BadOperation, m_state == Leaf);
            enj_assert(BadArgument, data);

            m_blob->read(m_offset, data, m_size);
        }

        void resize(size_t size)
        {
            enj_assert(BadOperation, m_state == Leaf);

            if (size < m_size)
            {
                blob()->remove(m_offset, m_size - size);

                Event evt =
                {
                    .type = Shrink,
                    .offset = m_offset,
                    .count = m_size - size
                };
                M_notifyUp(evt);
            }
            else if (size > m_size)
            {
                blob()->insert(m_offset, 0, size - m_size);

                Event evt =
                {
                    .type = Expand,
                    .offset = m_offset,
                    .count = size - m_size
                };
                M_notifyUp(evt);
            }
        }

        void remove()
        {
            enj_assert(BadOperation, m_state != Invalid);

            m_blob->remove(m_offset, m_size);
            // INode<T>::parent()->M_detachChild((T*) this);

            Event evt =
            {
                .type = Shrink,
                .offset = m_offset,
                .count = m_size
            };
            M_notifyUp(evt);
        }

        void insert(size_t offset, size_t size)
        {
            enj_assert(BadOperation, m_state != Invalid);
            enj_assert(BadArgument, offset >= m_offset && offset <= m_offset + m_size);

            m_blob->insert(offset, 0, size);

            Event evt =
            {
                .type = Expand,
                .offset = offset,
                .count = size
            };
            M_notifyUp(evt);
        }

        /*void move(size_t offset)
        {
            enj_assert(BadOperation, m_state != Invalid);
            enj_assert(BadArgument, offset + m_size <= m_blob->size());
            if (parent())
                enj_assert(BadArgument, offset >= parent()->offset());

            if (offset == m_offset)
                return;

            Event evt =
            {
                .type = offset < m_offset ? MoveUp : MoveDown,
                .offset = offset,
                .count = offset < m_offset ? m_offset - offset : offset - m_offset
            };

            m_offset = offset;
            M_notifyDown(evt);
        }

        void shift(ssize_t amount)
        {
            enj_assert(BadArgument, amount > 0 ? (size_t) amount < m_offset : (size_t) -amount < m_offset);
            move(m_offset + amount);
        }*/

    protected:
        void M_forceSize(size_t new_size)
        {
            enj_assert(BadArgument, m_offset + new_size <= m_blob->size());

            M_setSize(new_size);
        }

        virtual void M_updateNode() override
        {
            enj_assert(BadOperation, m_state != Invalid);

            // If we don't have any children, switch to invalid mode
            //   if we were previously in Managed mode
            if (!INode<T>::childrenCount())
            {
                if (m_state == Managed)
                    m_state = Invalid;

                return;
            }

            // Switch to managed mode : offset and size are
            //   determined using child nodes
            m_state = Managed;

            // First of all, sort children nodes by offset
            std::sort(INode<T>::M_children().begin(), INode<T>::M_children().end(),
                [](DPointer* a, DPointer* b) { return a->offset() < b->offset(); });

            //TODO: check for overlapping children

            // Update offset and size according to children
            DPointer* first = INode<T>::M_children().front();
            DPointer* last = INode<T>::M_children().back();

            M_setOffset(first->offset());
            M_setSize(last->offset() + last->size() - m_offset);

            // Propagate changes to parent
            if (INode<T>::parent())
                INode<T>::parent()->M_updateNode();
        }

        virtual void M_updateOffset(size_t new_offset)
        {}

        virtual void M_updateSize(size_t new_size)
        {}

    private:
        void M_notifyUp(Event const& evt)
        {
            if (INode<T>::parent())
            {
                INode<T>::parent()->M_handle(evt);
                INode<T>::parent()->M_updateNode();
            }
        }

        void M_notifyDown(Event const& evt)
        {
            for (auto it : INode<T>::children())
                it->M_handle(evt);
        }

        void M_handle(Event const& evt)
        {
            switch (evt.type)
            {
                case Expand:
                case Shrink:
                {
                    if (evt.offset > m_offset + m_size)
                        return;

                    // First, move all childs
                    Event move =
                    {
                        .type = evt.type == Expand ? MoveDown : MoveUp,
                        .offset = evt.offset,
                        .count = evt.count
                    };

                    M_notifyDown(move);

                    // Then, update ourselves
                    if (evt.offset < m_offset)
                        M_setOffset(m_offset + (evt.type == Expand ? evt.count : -evt.count));
                    else
                        M_setSize(m_size + (evt.type == Expand ? evt.count : -evt.count));

                    // M_notifyUp(evt);

                    break;
                }

                case MoveDown:
                    if (evt.offset > m_offset + m_size)
                        return;

                    if (evt.offset < m_offset)
                        M_setOffset(m_offset + evt.count);
                    else
                        M_setSize(m_size + evt.count);

                    M_notifyDown(evt);

                    break;

                case MoveUp:
                    if (evt.offset > m_offset + m_size)
                        return;

                    if (evt.offset < m_offset)
                        M_setOffset(m_offset - evt.count);
                    else
                        M_setSize(m_size - evt.count);

                    M_notifyDown(evt);

                    break;
            }
        }

        void M_setOffset(size_t new_offset)
        {
            //TODO: what order ?
            M_updateOffset(new_offset);
            m_offset = new_offset;
        }

        void M_setSize(size_t new_size)
        {
            //TODO: what order ?
            M_updateSize(new_size);
            m_size = new_size;
        }

    private:
        State m_state;
        Blob* m_blob;
        size_t m_offset;
        size_t m_size;
    };

    class DModel : public DPointer<DModel>
    {
    public:
        DModel(Blob* blob, size_t offset, size_t size, DModel* parent = 0)
            : DPointer(blob, offset, size, parent)
        {}

        bool hasAttr(std::string const& name) const
        {
            auto it = m_attrs.find(name);
            if (it != m_attrs.end())
                return true;

            return parent() && parent()->hasAttr(name);
        }

        void setAttr(std::string const& name, std::string const& value = "", bool enforce_local = false)
        {
            auto it = m_attrs.find(name);
            if (it != m_attrs.end() || enforce_local)
            {
                m_attrs[name] = value;
                return;
            }

            if (parent() && parent()->hasAttr(name))
                parent()->setAttr(name, value);
            else
                m_attrs[name] = value;
        }

        std::string const& attr(std::string const& name) const
        {
            auto it = m_attrs.find(name);
            if (it != m_attrs.end())
                return it->second;

            enj_assert(DoesntExists, parent());
            return parent()->attr(name);
        }

    private:
        std::map<std::string, std::string> m_attrs;
    };

    class DIntModel : public DModel
    {
    public:
        DIntModel(Blob* blob, size_t offset, size_t size, DModel* parent = 0)
            : DModel(blob, offset, size, parent)
        {
            enj_assert(BadArgument, size <= sizeof(size_t));
        }

        size_t get() const
        {
            size_t value = 0;
            read((uint8_t*) &value);
            return value;
        }

        void set(size_t value)
        {
            write((uint8_t const*) &value);
        }
    };

    class DRecordModel : public DModel
    {
    public:
        DRecordModel(Blob* blob, size_t offset, DModel* parent = 0)
            : DModel(blob, offset, 0, parent)
        {}

        DIntModel* get(std::string const& name) const
        {
            auto it = m_fields.find(name);
            enj_assert(DoesntExists, it != m_fields.end());
            enj_assert(DoesntExists, it->second < childrenCount());

            return (DIntModel*) child(it->second);
        }

    protected:
        void M_addField(std::string const& name, size_t offset, size_t size)
        {
            enj_assert(AlreadyExists, m_fields.find(name) == m_fields.end());
            m_fields[name] = childrenCount();
            new DIntModel(blob(), this->offset() + offset, size, this);
        }

        void M_updateNode() override
        {
            DModel::M_updateNode();

            for (;;)
            {
                bool exit = true;

                for (auto it = m_fields.begin(); it != m_fields.end(); ++it)
                {
                    if (it->second >= childrenCount())
                    {
                        m_fields.erase(it);
                        exit = false;
                        break;
                    }
                }

                if (exit)
                    break;
            }
        }

    private:
        std::map<std::string, size_t> m_fields;
    };

    class DStringModel : public DModel
    {
    public:
        DStringModel(Blob* blob, size_t index, DIntModel* relative_section = 0, DModel* parent = 0)
            : DModel(blob, relative_section ? relative_section->offset() + index : index, 0, parent)
            , m_relative_section(relative_section)
        {
            M_forceSize(M_measureLength());
        }

        std::string get() const
        {
            char* temp = new char[size()];
            read((uint8_t*) temp);
            std::string str = temp;
            delete[] temp;

            return str;
        }

        void set(std::string const& value)
        {
            size_t old_size = size();
            size_t new_size = value.size() + 1;

            if (old_size != new_size)
                resize(new_size);

            write((uint8_t const*) value.c_str());
        }

        void setIndexWriteBack(DIntModel* wb)
        { m_index_wb = wb; }

    private:
        size_t M_measureLength() const
        {
            size_t len = 0;
            size_t off = offset();

            char c;
            do
            {
                blob()->read(off + len, (uint8_t*) &c, 1);
                ++len;
            } while (c != '\0' && off + len <= blob()->size());

            return len;
        }

        void M_updateOffset(size_t new_offset) override
        {
            if (m_index_wb)
                m_index_wb->set(m_relative_section ? new_offset - m_relative_section->offset() : new_offset);
        }

    private:
        DIntModel* m_relative_section;
        DIntModel* m_index_wb;
    };

    template <typename T>
    class DTableModel : public DModel
    {
    public:
        DTableModel(Blob* blob, size_t offset, size_t entsize, size_t count, DModel* parent = 0)
            : DModel(blob, offset, entsize * count, parent)
            , m_entsize(entsize)
            , m_offset_wb(0)
            , m_count_wb(0)
        {
            enj_assert(BadArgument, m_entsize != 0);

            for (size_t i = 0; i < count; ++i)
                new T(blob, i, offset + i * entsize, this);
        }

        size_t entsize() const
        { return m_entsize; }

        size_t count() const
        { return childrenCount(); }

        void setOffsetWriteBack(DIntModel* wb)
        { m_offset_wb = wb; }

        void setCountWriteBack(DIntModel* wb)
        { m_count_wb = wb; }

        T* get(size_t index) const
        {
            enj_assert(BadArgument, index < count());

            return (T*) child(index);
        }

        void remove(size_t index)
        {
            enj_assert(BadArgument, index < count());

            // Update other section indices
            for (size_t i = index + 1; i < count(); ++i)
                get(i)->setIndex(i - 1);

            child(index)->remove();
            delete child(index);
        }

        T* insert()
        {
            size_t new_offset = offset() + size();
            DModel::insert(new_offset, m_entsize);

            return new T(blob(), count(), new_offset, this);
        }

    protected:
        void M_updateOffset(size_t new_offset) override
        {
            if (m_offset_wb)
                m_offset_wb->set(new_offset);
        }

        void M_updateSize(size_t new_size) override
        {
            if (m_count_wb)
                m_count_wb->set(new_size / m_entsize);
        }

    private:
        size_t m_entsize;
        DIntModel* m_offset_wb;
        DIntModel* m_count_wb;
    };

    class DEhdrModel : public DRecordModel
    {
    public:
        DEhdrModel(Blob* blob, size_t offset, DModel* parent = 0)
            : DRecordModel(blob, offset, parent)
        {
            M_addField("ei_mag",        EI_MAG0, SELFMAG);
            M_addField("ei_class",      EI_CLASS, 1);
            M_addField("ei_data",       EI_DATA, 1);
            M_addField("ei_version",    EI_VERSION, 1);
            M_addField("ei_osabi",      EI_OSABI, 1);
            M_addField("ei_abiversion", EI_ABIVERSION, 1);
            M_addField("ei_pad",        EI_PAD, EI_NIDENT - EI_PAD);

            if (attr("class") == "32")
                M_addFields<Elf32_Ehdr>();
            else
                M_addFields<Elf64_Ehdr>();
        }

    private:
        template <typename T>
        void M_addFields()
        {
            M_addField("e_type",        offsetof(T, e_type),      sizeof(T::e_type));
            M_addField("e_machine",     offsetof(T, e_machine),   sizeof(T::e_machine));
            M_addField("e_version",     offsetof(T, e_version),   sizeof(T::e_version));
            M_addField("e_entry",       offsetof(T, e_entry),     sizeof(T::e_entry));
            M_addField("e_phoff",       offsetof(T, e_phoff),     sizeof(T::e_phoff));
            M_addField("e_shoff",       offsetof(T, e_shoff),     sizeof(T::e_shoff));
            M_addField("e_flags",       offsetof(T, e_flags),     sizeof(T::e_flags));
            M_addField("e_ehsize",      offsetof(T, e_ehsize),    sizeof(T::e_ehsize));
            M_addField("e_phentsize",   offsetof(T, e_phentsize), sizeof(T::e_phentsize));
            M_addField("e_phnum",       offsetof(T, e_phnum),     sizeof(T::e_phnum));
            M_addField("e_shentsize",   offsetof(T, e_shentsize), sizeof(T::e_shentsize));
            M_addField("e_shnum",       offsetof(T, e_shnum),     sizeof(T::e_shnum));
            M_addField("e_shstrndx",    offsetof(T, e_shstrndx),  sizeof(T::e_shstrndx));
        }
    };

    class DPhdrModel : public DRecordModel
    {
    public:
        DPhdrModel(Blob* blob, size_t index, size_t offset, DModel* parent = 0)
            : DRecordModel(blob, offset, parent)
            , m_index(index)
        {
            if (attr("class") == "32")
                M_addFields<Elf32_Phdr>();
            else
                M_addFields<Elf64_Phdr>();
        }

        size_t index() const
        { return m_index; }

        void setIndex(size_t index)
        { m_index = index; }

    private:
        template <typename T>
        void M_addFields()
        {
            M_addField("p_type",   offsetof(T, p_type),   sizeof(T::p_type));
            M_addField("p_offset", offsetof(T, p_offset), sizeof(T::p_offset));
            M_addField("p_vaddr",  offsetof(T, p_vaddr),  sizeof(T::p_vaddr));
            M_addField("p_paddr",  offsetof(T, p_paddr),  sizeof(T::p_paddr));
            M_addField("p_filesz", offsetof(T, p_filesz), sizeof(T::p_filesz));
            M_addField("p_memsz",  offsetof(T, p_memsz),  sizeof(T::p_memsz));
            M_addField("p_flags",  offsetof(T, p_flags),  sizeof(T::p_flags));
            M_addField("p_align",  offsetof(T, p_align),  sizeof(T::p_align));
        }

    private:
        size_t m_index;
    };

    class DShdrModel : public DRecordModel
    {
    public:
        DShdrModel(Blob* blob, size_t index, size_t offset, DModel* parent = 0)
            : DRecordModel(blob, offset, parent)
            , m_index(index)
            , m_index_wb(0)
        {
            if (attr("class") == "32")
                M_addFields<Elf32_Shdr>();
            else
                M_addFields<Elf64_Shdr>();
        }

        size_t index() const
        { return m_index; }

        void setIndex(size_t index)
        {
            if (parent())
            {
                //TODO: check if parent is indeed a DTableModel<DShdrModel> (w/o RTTI ?)
                DTableModel<DShdrModel>* table = (DTableModel<DShdrModel>*) parent();

                for (size_t i = 0; i < table->count(); ++i)
                {
                    DShdrModel* shdr = table->get(i);

                    if (shdr->get("sh_link")->get() == m_index)
                        shdr->get("sh_link")->set(index);
                }
            }

            m_index = index;

            if (m_index_wb)
                m_index_wb->set(m_index);
        }

        void setIndexWriteBack(DIntModel* wb)
        { m_index_wb = wb; }

    private:
        template <typename T>
        void M_addFields()
        {
            M_addField("sh_name",      offsetof(T, sh_name),      sizeof(T::sh_name));
            M_addField("sh_type",      offsetof(T, sh_type),      sizeof(T::sh_type));
            M_addField("sh_flags",     offsetof(T, sh_flags),     sizeof(T::sh_flags));
            M_addField("sh_addr",      offsetof(T, sh_addr),      sizeof(T::sh_addr));
            M_addField("sh_offset",    offsetof(T, sh_offset),    sizeof(T::sh_offset));
            M_addField("sh_size",      offsetof(T, sh_size),      sizeof(T::sh_size));
            M_addField("sh_link",      offsetof(T, sh_link),      sizeof(T::sh_link));
            M_addField("sh_info",      offsetof(T, sh_info),      sizeof(T::sh_info));
            M_addField("sh_addralign", offsetof(T, sh_addralign), sizeof(T::sh_addralign));
            M_addField("sh_entsize",   offsetof(T, sh_entsize),   sizeof(T::sh_entsize));
        }

    private:
        size_t m_index;
        DIntModel* m_index_wb;
    };

    class DElfModel : public DModel
    {
    public:
        DElfModel(Blob* blob)
            : DModel(blob, 0, blob->size())
        {
            /* Get the ELF file class and check magic bytes */

            DIntModel* ei_mag = new DIntModel(blob, EI_MAG0, SELFMAG);
            enj_assert(BadElfMag, ei_mag->get() != *((size_t*) ELFMAG));

            DIntModel* ei_class = new DIntModel(blob, EI_CLASS, 1);
            size_t ei_class_val = ei_class->get();
            enj_assert(BadElfClass, ei_class_val == ELFCLASS32 || ei_class_val == ELFCLASS64);

            if (ei_class_val == ELFCLASS32)
                setAttr("class", "32");
            else if (ei_class_val == ELFCLASS64)
                setAttr("class", "64");

            delete ei_class;
            delete ei_mag;

            /* Setup ELF file header */

            m_ehdr = new DEhdrModel(blob, 0, this);

            /* Setup program headers */

            DIntModel* phoff = m_ehdr->get("e_phoff");
            DIntModel* phentsize = m_ehdr->get("e_phentsize");
            DIntModel* phnum = m_ehdr->get("e_phnum");

            if (phoff->get())
            {
                m_phdr_table = new DTableModel<DPhdrModel>(blob, phoff->get(), phentsize->get(), phnum->get(), this);
                m_phdr_table->setOffsetWriteBack(phoff);
                m_phdr_table->setCountWriteBack(phnum);
            }

            /* Setup section headers */

            DIntModel* shoff = m_ehdr->get("e_shoff");
            DIntModel* shentsize = m_ehdr->get("e_shentsize");
            DIntModel* shnum = m_ehdr->get("e_shnum");

            if (shoff->get())
            {
                m_shdr_table = new DTableModel<DShdrModel>(blob, shoff->get(), shentsize->get(), shnum->get(), this);
                m_shdr_table->setOffsetWriteBack(shoff);
                m_shdr_table->setCountWriteBack(shnum);
            }

            /* Setup .shstrtab if there's one */

            size_t shstrndx = m_ehdr->get("e_shstrndx")->get();
            if (shstrndx > 0 && shstrndx < m_shdr_table->count())
                M_setShstrtab(m_shdr_table->get(shstrndx));
        }

        DEhdrModel* ehdr() const
        { return m_ehdr; }

        DTableModel<DPhdrModel>* phdrTable() const
        { return m_phdr_table; }

        DTableModel<DShdrModel>* shdrTable() const
        { return m_shdr_table; }

    private:
        void M_setShstrtab(DShdrModel* shdr)
        {
            enj_assert(BadArgument, shdr);

            shdr->setIndexWriteBack(m_ehdr->get("e_shstrndx"));
            m_shstrtab = shdr;
        }

    private:
        DEhdrModel* m_ehdr;
        DTableModel<DPhdrModel>* m_phdr_table;
        DTableModel<DShdrModel>* m_shdr_table;
        DShdrModel* m_shstrtab;
    };
}

void debug(enj::DModel* ptr, size_t indent = 0)
{
    for (size_t i = 0; i < indent; ++i)
        printf("  ");

    printf("(%ld, %ld)\n", ptr->offset(), ptr->size());
    for (auto it : ptr->children())
        debug(it, indent + 1);
}

int main(int argc, char** argv)
{
    using namespace enj;

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

        Blob* blob = new ContiguousBlob();
        blob->insert(0, file, size);
        delete[] file;

        /* Actual test */

        DElfModel* elf = new DElfModel(blob);

        elf->shdrTable()->remove(2);
        elf->shdrTable()->remove(2);

        for (size_t i = 0; i < elf->shdrTable()->count(); ++i)
        {
            DShdrModel* shdr = elf->shdrTable()->get(i);

            printf("%2ld <-> %2ld | 0x%08lX %ld\n", i, shdr->index(), shdr->get("sh_offset")->get(), shdr->get("sh_link")->get());
        }

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
