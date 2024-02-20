#include "Base.h"
#include "Internal.h"
#include "../Exception.h"
#include <cctype>

std::string Fmi::date_time::Base::as_string() const
{
    switch(m_type)
    {
    case POS_INFINITY:
        return "PINF";
    case NEG_INFINITY:
        return "NINF";
    case NOT_A_DATE_TIME:
        return "not-a-date-time";
    default:
        throw Fmi::Exception(BCP, "INTERNAL ERROR: invalid type");
    }
}

bool Fmi::date_time::Base::operator == (const Base& other) const
{
    if (!is_special() && !other.is_special())
        throw Fmi::Exception(BCP, "INTERNAL ERROR: operation not supported for normal values");
    return m_type == other.m_type;
}

bool Fmi::date_time::Base::operator != (const Base& other) const
{
    if (!is_special() && !other.is_special())
        throw Fmi::Exception(BCP, "INTERNAL ERROR: operation not supported for normal values");
    return m_type != other.m_type;
}

bool Fmi::date_time::Base::operator < (const Base& other) const
{
    assert_supported(other);
    return m_type < other.m_type;
}

bool Fmi::date_time::Base::operator <= (const Base& other) const
{
    assert_supported(other);
    return m_type <= other.m_type;
}

bool Fmi::date_time::Base::operator > (const Base& other) const
{
    assert_supported(other);
    return m_type > other.m_type;
}

bool Fmi::date_time::Base::operator >= (const Base& other) const
{
    assert_supported(other);
    return m_type >= other.m_type;
}

void Fmi::date_time::Base::assert_supported() const
{
    if (is_not_a_date_time())
        throw Fmi::Exception(BCP, "Operation not supported for NOT_A_DATE_TIME");

    if (!is_special())
        throw Fmi::Exception(BCP, "INTERNAL ERROR: operation not supported for normal values");
}

void Fmi::date_time::Base::assert_supported(const Base& other) const
{
    if (is_not_a_date_time() || other.is_not_a_date_time())
        throw Fmi::Exception(BCP, "Operation not supported for NOT_A_DATE_TIME");

    if (!is_special() && !other.is_special())
        throw Fmi::Exception(BCP, "INTERNAL ERROR: operation not supported for normal values");
}

std::string
Fmi::date_time::internal::handle_parse_remainder(std::istringstream& is)
{
    std::string remaining;
    const std::string src = is.str();
    std::copy(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>(),
        std::back_inserter(remaining));

    const std::size_t rLen = remaining.length();
    const std::size_t srcLen = src.length();
    return "'" + src.substr(0, srcLen - rLen) + "' <--> '" + remaining + "'";
}

// Check parse status:
// - If assume_eoi is true, then the remaining part of stream may only contain space
// - If assume_eoi is false, then there must be at least one addional spec or EOF in the stream
void Fmi::date_time::internal::check_parse_status(std::istream& is, bool assume_eoi, const char* name)
{
    int num_space = 0;
    while (!is.eof() && std::isspace(is.peek()))
    {
        is.get();
        ++num_space;
    }

    const bool had_trailing_space = num_space > 0;
    if (!is.eof())
    {
        if ((assume_eoi && !is.eof()) || !had_trailing_space)
        {
            std::string message = "Failed to parse " + std::string(name) + " from string";
            throw Fmi::Exception(BCP, message);
        }
    }
}

std::string Fmi::date_time::internal::remove_trailing_zeros(const std::string& str)
{
    if (*str.rbegin() == '0' || *str.rbegin() == '.')
    {
        std::string::size_type pos = str.find_last_not_of('.');
        if (pos != std::string::npos)
        {
            if (str[pos] == '.')
                pos--;
            return str.substr(0, pos + 1);
        }
    }

    return str;
}
