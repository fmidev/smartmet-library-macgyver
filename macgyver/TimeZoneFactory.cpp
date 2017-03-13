// ======================================================================
/*!
 * \brief Implementation of singleton TimeZoneFactory
 */
// ======================================================================

#include "TimeZoneFactory.h"
#include "StringConversion.h"
#include "WorldTimeZones.h"

#ifdef FMI_MULTITHREAD
#include <boost/thread.hpp>
#endif

#include <memory>
#include <stdexcept>

using namespace std;

static std::string& default_regions =
    *new std::string("/usr/share/smartmet/timezones/date_time_zonespec.csv");
static std::string& default_coordinates =
    *new std::string("/usr/share/smartmet/timezones/timezone.shz");

namespace Fmi
{
#ifdef FMI_MULTITHREAD
typedef boost::shared_mutex MutexType;
typedef boost::shared_lock<MutexType> ReadLock;
typedef boost::unique_lock<MutexType> WriteLock;
#else
struct MutexType
{
};
struct ReadLock
{
  ReadLock(const MutexType& /* mutex */) {}
};
struct WriteLock
{
  WriteLock(const MutexType& /* mutex */) {}
};
#endif

// ----------------------------------------------------------------------
/*!
 * \brief Implementation hiding pimple
 */
// ----------------------------------------------------------------------

class TimeZoneFactory::Pimple
{
 public:
  // database for tz names
  string itsRegionsFile;
  std::unique_ptr<boost::local_time::tz_database> itsRegions;
  MutexType itsRegionsMutex;

  // database for coordinates to tz conversion
  string itsCoordinatesFile;
  std::unique_ptr<WorldTimeZones> itsCoordinates;
  MutexType itsCoordinatesMutex;

};  // Pimple

// ----------------------------------------------------------------------
/*!
 * \brief Private constructor for instance()
 */
// ----------------------------------------------------------------------

TimeZoneFactory::TimeZoneFactory() : itsPimple(new Pimple()) {}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

TimeZoneFactory::~TimeZoneFactory()
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Set the time zone database filename
 */
// ----------------------------------------------------------------------

void TimeZoneFactory::set_region_file(const string& file)
{
  WriteLock lock(itsPimple->itsRegionsMutex);

  // Double checked locking pattern
  if (itsPimple->itsRegionsFile == file) return;

  itsPimple->itsRegionsFile = file;
  itsPimple->itsRegions.reset(new boost::local_time::tz_database());
  itsPimple->itsRegions->load_from_file(itsPimple->itsRegionsFile);
}

// ----------------------------------------------------------------------
/*!
 * \brief Set the time zone coordinate database filename
 */
// ----------------------------------------------------------------------

void TimeZoneFactory::set_coordinate_file(const string& file)
{
  WriteLock lock(itsPimple->itsCoordinatesMutex);

  // Double checked locking pattern
  if (itsPimple->itsCoordinatesFile == file) return;

  itsPimple->itsCoordinatesFile = file;
  itsPimple->itsCoordinates.reset(new WorldTimeZones(file));
}

// ----------------------------------------------------------------------
/*!
 * \brief List the known databases
 */
// ----------------------------------------------------------------------

vector<string> TimeZoneFactory::region_list()
{
  if (itsPimple->itsRegions == 0) set_region_file(default_regions);

  ReadLock lock(itsPimple->itsRegionsMutex);
  return itsPimple->itsRegions->region_list();
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a region name
 */
// ----------------------------------------------------------------------

boost::local_time::time_zone_ptr TimeZoneFactory::time_zone_from_region(const string& id)
{
  if (itsPimple->itsRegions == 0) set_region_file(default_regions);

  ReadLock lock(itsPimple->itsRegionsMutex);

  boost::local_time::time_zone_ptr ptr = itsPimple->itsRegions->time_zone_from_region(id);
  if (ptr == 0) throw runtime_error("TimeZoneFactory does not recognize region '" + id + "'");
  return ptr;
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a string (region name or posix description)
 */
// ----------------------------------------------------------------------

boost::local_time::time_zone_ptr TimeZoneFactory::time_zone_from_string(const string& desc)
{
  if (itsPimple->itsRegions == 0) set_region_file(default_regions);

  ReadLock lock(itsPimple->itsRegionsMutex);

  // Try region name at first
  boost::local_time::time_zone_ptr ptr = itsPimple->itsRegions->time_zone_from_region(desc);
  if (ptr == 0)
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

boost::local_time::time_zone_ptr TimeZoneFactory::time_zone_from_coordinate(float lon, float lat)
{
  if (itsPimple->itsCoordinates == 0) set_coordinate_file(default_coordinates);

  string tz = itsPimple->itsCoordinates->zone_name(lon, lat);
  boost::local_time::time_zone_ptr ptr = time_zone_from_string(tz);
  if (ptr == 0)
    throw runtime_error("TimeZoneFactory could not convert given coordinate " +
                        Fmi::to_string(lon) + "," + Fmi::to_string(lat) +
                        " to a valid time zone name");

  return ptr;
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a coordinate
 */
// ----------------------------------------------------------------------

std::string TimeZoneFactory::zone_name_from_coordinate(float lon, float lat)
{
  if (itsPimple->itsCoordinates == 0) set_coordinate_file(default_coordinates);

  return itsPimple->itsCoordinates->zone_name(lon, lat);
}

}  // namespace Fmi
