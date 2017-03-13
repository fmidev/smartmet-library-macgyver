// ======================================================================
/*!
 * \brief Implementation of class TimeZones
 */
// ======================================================================

#include "TimeZones.h"
#include "StringConversion.h"
#include "WorldTimeZones.h"

#include <memory>
#include <stdexcept>

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
  Pimple(const std::string& regionsFile, const std::string coordinatesFile)
      : itsRegions(), itsCoordinates(coordinatesFile)
  {
    itsRegions.load_from_file(regionsFile);
  }

  boost::local_time::tz_database itsRegions;
  WorldTimeZones itsCoordinates;

};  // Pimple

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

TimeZones::~TimeZones() {}
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

vector<string> TimeZones::region_list() const { return itsPimple->itsRegions.region_list(); }
// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a region name
 */
// ----------------------------------------------------------------------

boost::local_time::time_zone_ptr TimeZones::time_zone_from_region(const string& id) const
{
  boost::local_time::time_zone_ptr ptr = itsPimple->itsRegions.time_zone_from_region(id);
  if (!ptr) throw runtime_error("TimeZones does not recognize region '" + id + "'");
  return ptr;
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a string (region name or posix description)
 */
// ----------------------------------------------------------------------

boost::local_time::time_zone_ptr TimeZones::time_zone_from_string(const string& desc) const
{
  // Try region name at first
  boost::local_time::time_zone_ptr ptr = itsPimple->itsRegions.time_zone_from_region(desc);
  if (!ptr)
  {
    // Region name not found: try POSIX TZ description (may throw exception)
    ptr.reset(new boost::local_time::posix_time_zone(desc));
  }

  return ptr;
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a coordinate
 */
// ----------------------------------------------------------------------

boost::local_time::time_zone_ptr TimeZones::time_zone_from_coordinate(double lon, double lat) const
{
  string tz = itsPimple->itsCoordinates.zone_name(lon, lat);
  boost::local_time::time_zone_ptr ptr = time_zone_from_string(tz);
  if (!ptr)
    throw runtime_error("TimeZones could not convert given coordinate " + Fmi::to_string(lon) +
                        "," + Fmi::to_string(lat) + " to a valid time zone name");

  return ptr;
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a coordinate
 */
// ----------------------------------------------------------------------

std::string TimeZones::zone_name_from_coordinate(double lon, double lat) const
{
  return itsPimple->itsCoordinates.zone_name(lon, lat);
}

}  // namespace Fmi
