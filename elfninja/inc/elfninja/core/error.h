#ifndef __ELFNINJA_ERROR_H__
#define __ELFNINJA_ERROR_H__

#include <string>
#include <sstream>
#include <cstddef>

#define enj_error(name) throw (enj::name ## Error(false, __FUNCTION__, __FILE__, __LINE__))
#define enj_assert(name, expr) (void) ((expr) || (enj_error(name), false))

#define enj_internal_error(name) throw (enj::name ## Error(true, __FUNCTION__, __FILE__, __LINE__))
#define enj_internal_assert(name, expr) (void) ((expr) || (enj_internal_error(name), false))

namespace enj
{
    class Error
    {
    public:
        Error(bool internal, std::string desc, std::string const& fun = "", std::string const& file = "", size_t line = 0)
            : m_internal(internal), m_desc(desc), m_fun(fun), m_file(file), m_line(line)
        {}

        bool internal() const
        {
            return m_internal;
        }

        std::string const& description() const
        {
            return m_desc;
        }

        std::string location() const
        {
            if (!m_fun.size())
                return "";
            std::ostringstream ss;
            ss << "in " << m_fun << " at " << m_file << ":" << m_line;
            return ss.str();
        }

    private:
        bool m_internal;
        std::string m_desc;
        std::string m_fun;
        std::string m_file;
        size_t m_line;
    };

    #define DEF_ERROR(name, desc) \
    class name ## Error : public Error \
    { \
    public: \
        name ## Error(bool internal, std::string const& fun = "", std::string const& file = "", size_t line = 0) \
            : Error(internal, (desc), fun, file, line) {} \
    };

    #include "error.def"
}

#endif // __ELFNINJA_ERROR_H__
