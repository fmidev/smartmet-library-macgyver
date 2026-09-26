// ======================================================================
/*!
 * \brief Implementation of singleton TimeZoneFactory
 */
// ======================================================================

#include "TimeZoneFactory.h"
#include "Exception.h"
#include "StringConversion.h"
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace std;

namespace Fmi
{
// ----------------------------------------------------------------------
/*!
 * \brief Implementation hiding pimple
 */
// ----------------------------------------------------------------------

class TimeZoneFactory::Impl
{
 public:
  Impl();

  const date::tzdb& m_Regions;
};

// ----------------------------------------------------------------------
/*!
 * \brief Implementation details constructor
 *
 * Note: Since we're using the static singleton pattern this makes everything
 * thread safe.
 */
// ----------------------------------------------------------------------

TimeZoneFactory::Impl::Impl() : m_Regions(date::get_tzdb()) {}

// ----------------------------------------------------------------------
/*!
 * \brief Private constructor for instance()
 */
// ----------------------------------------------------------------------

TimeZoneFactory::TimeZoneFactory() : m_Impl(new Impl()) {}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

TimeZoneFactory::~TimeZoneFactory() = default;

// ----------------------------------------------------------------------
/*!
 * \brief List the known databases
 */
// ----------------------------------------------------------------------

vector<string> TimeZoneFactory::region_list()
{
  try
  {
    return date_time::TimeZonePtr::get_region_list();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a region name
 */
// ----------------------------------------------------------------------

Fmi::TimeZonePtr TimeZoneFactory::time_zone_from_region(const string& id)
{
  try
  {
    Fmi::TimeZonePtr ptr(date::locate_zone(id));

    if (!ptr)
       throw Fmi::Exception(BCP, "TimeZoneFactory does not recognize region '" + id + "'");

    return ptr;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a string (region name or posix description)
 */
// ----------------------------------------------------------------------

Fmi::TimeZonePtr TimeZoneFactory::time_zone_from_string(const string& desc)
{
  try
  {
    // Try region name at first
    Fmi::TimeZonePtr ptr(date::locate_zone(desc));

    // FIXME: POSIX TZ are currently not supported
    //if (!ptr)
    //{
    ////Region name not found: try POSIX TZ description (may throw exception)
    //ptr.reset(new boost::local_time::posix_time_zone(desc));
    //}

    return ptr;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the TimeZoneFactory instance
 */
// ----------------------------------------------------------------------

TimeZoneFactory& TimeZoneFactory::instance()
{
  try
  {
    static TimeZoneFactory obj;
    return obj;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Fmi
