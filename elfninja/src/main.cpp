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
    class ITree
    {
    public:
        ITree(T* parent = 0)
            : m_parent(parent)
        {
            if (m_parent)
                m_parent->M_addChild(static_cast<T*>(this));
        }

        virtual ~ITree()
        {
            for (auto it : m_children)
                delete it;
        }

        T* parent() const
        { return m_parent; }

        size_t childrenCount() const
        { return m_children.size(); }

        T* child(size_t i) const
        {
            enj_assert(BadArgument, i < m_children.size());
            return m_children.at(i);
        }

        std::vector<T*> const& children() const
        { return m_children; }

    protected:
        void M_addChild(T* child)
        {
            enj_assert(BadArgument, child);
            enj_assert(AlreadyExists, std::find(m_children.begin(), m_children.end(), child) == m_children.end());

            m_children.push_back(child);
        }

        void M_detachChild(T* child)
        {
            enj_assert(BadArgument, child);
            auto it = std::find(m_children.begin(), m_children.end(), child);
            enj_assert(DoesntExists, it == m_children.end());
            m_children.erase(it);
        }

        void M_removeChild(T* child)
        {
            M_detachChild(child);
            delete child;
        }

    private:
        T* m_parent;
        std::vector<T*> m_children;
    };

    template <typename U, typename V>
    class ITreeMap
    {
    public:
        ITreeMap(U* left, V* right)
            : m_left(left)
            , m_right(right)
        {
            M_map(m_left, m_right);
        }

        virtual ~ITreeMap()
        {}

        U* forward(V* right) const
        {
            auto it = m_forward.find(right);
            enj_assert(DoesntExists, it != m_forward.end());
            return it->second;
        }

        V* backward(U* left) const
        {
            auto it = m_backward.find(left);
            enj_assert(DoesntExists, it != m_backward.end());
            return it->second;
        }

    private:
        void M_map(U* left, V* right)
        {
            enj_assert(TreeMismatch, left->childrenCount() == right->childrenCount());

            m_forward[left] = right;
            m_backward[right] = left;

            for (size_t i = 0; i < left->childrenCount(); ++i)
                M_map(left->child(i), right->child(i));
        }

    private:
        U* m_left;
        V* m_right;

        std::map<U*, V*> m_forward;
        std::map<V*, U*> m_backward;
    };

    template <typename T>
    class DPointer : public ITree<T>
    {
    public:
        DPointer(Blob* blob, T* parent = 0)
            : ITree<T>(parent)
            , m_blob(blob)
        {
            enj_assert(BadArgument, m_blob);
        }

        virtual ~DPointer()
        {}

        Blob* blob() const
        { return m_blob; }

        virtual size_t offset() const = 0;
        virtual size_t size() const = 0;

    protected:
        virtual void M_handleExpand(size_t offset, size_t size)
        { M_notifyExpand(offset, size); }

        virtual void M_handleShrink(size_t offset, size_t size)
        { M_notifyShrink(offset, size); }

        void M_notifyShrink(size_t offset, size_t size)
        {
            if (ITree<T>::parent())
                ITree<T>::parent()->M_handleExpand(offset, size);
        }

        void M_notifyExpand(size_t offset, size_t size)
        {
            if (ITree<T>::parent())
                ITree<T>::parent()->M_handleShrink(offset, size);
        }

    private:
        Blob* m_blob;
    };

    class DModel : public DPointer<DModel>
    {
    public:
        DModel(Blob* blob, DModel* parent = 0)
            : DPointer<DModel>(blob, parent)
        {}

        virtual ~DModel()
        {}
    };

    class DFixedModel : public DModel
    {
    public:
        DFixedModel(size_t offset, size_t size, Blob* blob, DModel* parent = 0)
            : DModel(blob, parent)
            , m_offset(offset)
            , m_size(size)
        {
            enj_assert(BadArgument, m_offset + m_size >= blob->size());
        }

        virtual ~DFixedModel()
        {}

        size_t offset() const override
        { return m_offset; }

        size_t size() const override
        { return m_size; }

    private:
        size_t m_offset;
        size_t m_size;
    };

    class DInt : public DFixedModel
    {
    public:
        DInt(size_t offset, size_t size, Blob* blob, DModel* parent)
            : DFixedModel(offset, size, blob, parent)
        {}

        size_t get() const
        {
            size_t value = 0;
            blob()->read(offset(), (uint8_t*) &value, size());
            return value;
        }

        void set(size_t value)
        {
            blob()->write(offset(), (uint8_t const*) &value, size());
        }
    };

    class DString : public DModel
    {
    public:
        DString(size_t offset, Blob* blob, DModel* parent)
            : DModel(blob, parent)
            , m_offset(offset)
        {
            enj_assert(BadArgument, m_offset < blob->size());
        }

        virtual size_t offset() const
        { return m_offset; }

        virtual size_t size() const
        {
            size_t len = 0;
            char c;
            do
            {
                blob()->read(m_offset + len, (uint8_t*) &len, 1);
                ++len;
            } while (c != '\0' && m_offset + len < blob()->size());

            return len;
        }

        std::string get() const
        {
            size_t len = size();
            char* c_str = new char[len];

            blob()->read(m_offset, (uint8_t*) c_str, len);

            std::string value = c_str;
            delete[] c_str;
            return value;
        }

        void set(std::string const& value)
        {
            size_t old_size = size();
            size_t new_size = value.size() + 1;

            if (old_size < new_size)
            {
                M_notifyExpand(m_offset + old_size, new_size - old_size);
                blob()->insert(m_offset + old_size, 0, new_size - old_size);
            }
            else if (old_size > new_size)
            {
                M_notifyShrink(m_offset + new_size, old_size - new_size);
                blob()->remove(m_offset + new_size, old_size - new_size);
            }

            blob()->write(m_offset, (uint8_t const*) value.c_str(), new_size);
        }

    private:
        size_t m_offset;
    };
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



        /* Write output file */

        fd = creat("bin/ls", 0777);
        file = new uint8_t[blob->size()];
        blob->read(0, file, blob->size());
        write(fd, file, blob->size());
        delete[] file;

        /* Cleanup */

        delete blob;
    }
    catch (enj::Error const& e)
    {
        std::cerr << "[" << e.location() << "]: " << e.description() << std::endl;
        return -1;
    }

    return 0;
}
