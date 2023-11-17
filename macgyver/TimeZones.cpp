// ======================================================================
/*!
 * \brief Implementation of class TimeZones
 */
// ======================================================================

#include "TimeZones.h"
#include "Exception.h"
#include "StringConversion.h"
#include "WorldTimeZones.h"
#include <memory>
#include <stdexcept>
#include <unordered_map>

using namespace std;

const char* default_regions = "/usr/share/smartmet/timezones/date_time_zonespec.csv";
const char* default_coordinates = "/usr/share/smartmet/timezones/timezone.shz";

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
  Pimple(const std::string& regionsFile, const std::string& coordinatesFile)
      : itsCoordinates(coordinatesFile)
  {
    try
    {
      const DateTimeNS::tzdb& tzdb = DateTimeNS::get_tzdb();
      // Create all known timezones once for better access speed later on.
      std::for_each(
          tzdb.zones.begin(),
          tzdb.zones.end(),
          [this](const DateTimeNS::time_zone& tz) {
              const std::string& name = tz.name();
              TimeZonePtr tz_ptr(&tz);
              tz_names.emplace_back(name);
              itsKnownZones.emplace(name, tz_ptr);
          });

      std::for_each(
          tzdb.links.begin(),
          tzdb.links.end(),
          [this](const DateTimeNS::time_zone_link& tzl) {
              const std::string& name = tzl.name();
              TimeZonePtr tz_ptr(itsKnownZones.at(tzl.target()));
              tz_names.emplace_back(name);
              itsKnownZones.emplace(name, tz_ptr);
          });

      std::sort(tz_names.begin(), tz_names.end());
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }
  std::vector<std::string> tz_names;
  WorldTimeZones itsCoordinates;
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

TimeZones::TimeZones() : itsPimple(new Pimple(default_regions, default_coordinates)) {}
// ----------------------------------------------------------------------
/*!
 * \brief Constructor with alternate data sources
 */
// ----------------------------------------------------------------------

TimeZones::TimeZones(const std::string& regionsFile, const std::string& coordinatesFile)
    : itsPimple(new Pimple(regionsFile, coordinatesFile))
{
}

// ----------------------------------------------------------------------
/*!
 * \brief List the known databases
 */
// ----------------------------------------------------------------------

vector<string> TimeZones::region_list() const
{
    return itsPimple->tz_names;
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

    // Try POSIX TZ description (may throw) if region name is unknown
    return Fmi::TimeZonePtr(desc);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a coordinate
 */
// ----------------------------------------------------------------------

Fmi::TimeZonePtr TimeZones::time_zone_from_coordinate(double lon, double lat) const
{
  try
  {
    string tz = itsPimple->itsCoordinates.zone_name(lon, lat);
    Fmi::TimeZonePtr ptr = time_zone_from_string(tz);
    if (!ptr)
      throw Fmi::Exception(BCP,
                           "TimeZones could not convert given coordinate " + Fmi::to_string(lon) +
                               "," + Fmi::to_string(lat) + " to a valid time zone name");

    return ptr;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a coordinate
 */
// ----------------------------------------------------------------------

std::string TimeZones::zone_name_from_coordinate(double lon, double lat) const
{
  try
  {
    return itsPimple->itsCoordinates.zone_name(lon, lat);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Fmi
