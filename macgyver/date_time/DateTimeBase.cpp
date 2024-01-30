#include "DateTimeBase.h"
#include "../Exception.h"

std::string Fmi::date_time::Base::as_string() const
{
    switch(m_type)
    {
    case POS_INFINITY:
        return "PINF";
    case NEG_INFINITY:
        return "NINF";
    case NOT_A_DATE_TIME:
        return "NOT-A-DATE-TIME";
    default:
        throw Fmi::Exception(BCP, "INTERNAL ERROR: invalid type");
    }
}

bool Fmi::date_time::Base::operator == (const Base& other) const
{
    assert_supported(other);
    return m_type == other.m_type;
}

bool Fmi::date_time::Base::operator != (const Base& other) const
{
    assert_supported(other);
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

std::string Fmi::detail::handle_parse_remainder(std::istringstream& is)
{
    std::string remaining;
    const std::string src = is.str();
    std::copy(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>(),
        std::back_inserter(remaining));

    const std::size_t rLen = remaining.length();
    const std::size_t srcLen = src.length();
    if ((rLen > srcLen) || (src.substr(srcLen - rLen) != remaining))
        throw Fmi::Exception(BCP, "INTERNAL ERROR: incorrect use");
    return src.substr(0, srcLen - rLen) + "-->" + remaining;
}

