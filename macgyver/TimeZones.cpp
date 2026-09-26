// ======================================================================
/*!
 * \brief Implementation of class TimeZones
 */
// ======================================================================

#include "TimeZones.h"
#include "Exception.h"
#include "StringConversion.h"
#include <memory>
#include <stdexcept>
#include <unordered_map>

using namespace std;

namespace Fmi
{
// ----------------------------------------------------------------------
/*!
 * \brief Implementation hiding pimple
 */
// ----------------------------------------------------------------------

class TimeZones::Pimple
{
 public:
  Pimple() : itsRegions(date::get_tzdb())
  {
    try
    {
      // Create all known timezones once for better access speed later on.
      auto regions = date_time::TimeZonePtr::get_region_list();
      for (const auto& id : regions)
      {
        date_time::TimeZonePtr ptr(id);
        if (!ptr)
          throw Fmi::Exception(BCP, "Unknown timezone definition").addParameter("ID", id);

        itsKnownZones[id] = ptr;
      }
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }
  const date::tzdb& itsRegions;
  std::unordered_map<std::string, Fmi::TimeZonePtr> itsKnownZones;
};

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

TimeZones::~TimeZones() = default;
// ----------------------------------------------------------------------
/*!
 * \brief Default constructor
 */
// ----------------------------------------------------------------------

TimeZones::TimeZones() : itsPimple(new Pimple()) {}
// ----------------------------------------------------------------------
/*!
 * \brief List the known databases
 */
// ----------------------------------------------------------------------

vector<string> TimeZones::region_list() const
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

Fmi::TimeZonePtr TimeZones::time_zone_from_region(const string& id) const
{
  try
  {
    auto value = itsPimple->itsKnownZones.find(id);
    if (value != itsPimple->itsKnownZones.end())
      return value->second;

    throw Fmi::Exception(BCP, "TimeZones does not recognize region '" + id + "'");
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

Fmi::TimeZonePtr TimeZones::time_zone_from_string(const string& desc) const
{
  try
  {
    // Try region name at first
    auto value = itsPimple->itsKnownZones.find(desc);
    if (value != itsPimple->itsKnownZones.end())
      return value->second;

    // FIXME: POSIX TZ descriptions are not supported currently
    // Try POSIX TZ description (may throw) if region name is unknown
    //return Fmi::TimeZonePtr(new boost::local_time::posix_time_zone(desc));

    return Fmi::TimeZonePtr();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Fmi
