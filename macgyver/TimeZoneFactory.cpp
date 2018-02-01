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

namespace Fmi
{
static const char* default_regions = "/usr/share/smartmet/timezones/date_time_zonespec.csv";
static const char* default_coordinates = "/usr/share/smartmet/timezones/timezone.shz";

#ifdef FMI_MULTITHREAD
typedef boost::shared_mutex MutexType;
typedef boost::shared_lock<MutexType> ReadLock;
typedef boost::unique_lock<MutexType> WriteLock;
typedef boost::upgrade_lock<MutexType> UpgradeReadLock;
typedef boost::upgrade_to_unique_lock<MutexType> UpgradeWriteLock;
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
struct UpgradeReadLock
{
  UpgradeReadLock(const MutexType& /* mutex */) {}
};
struct UpgradeWriteLock
{
  UpgradeWriteLock(const MutexType& /* mutex */) {}
};
#endif

// ----------------------------------------------------------------------
/*!
 * \brief Implementation hiding pimple
 */
// ----------------------------------------------------------------------

class TimeZoneFactory::Impl
{
 public:
  void load_default_regions();
  void load_default_coordinates();

  mutable MutexType mRegionsMutex;
  mutable MutexType mCoordinatesMutex;

  std::unique_ptr<WorldTimeZones> mCoordinates;
  std::unique_ptr<boost::local_time::tz_database> mRegions;
};

// ----------------------------------------------------------------------
/*!
 * \brief Load the default regions file while having a lock
 */
// ----------------------------------------------------------------------

void TimeZoneFactory::Impl::load_default_regions()
{
  mRegions.reset(new boost::local_time::tz_database());
  mRegions->load_from_file(default_regions);
}

// ----------------------------------------------------------------------
/*!
 * \brief Load the default coordinates file while having a lock
 */
// ----------------------------------------------------------------------

void TimeZoneFactory::Impl::load_default_coordinates()
{
  mCoordinates.reset(new WorldTimeZones(default_coordinates));
}

// ----------------------------------------------------------------------
/*!
 * \brief Private constructor for instance()
 */
// ----------------------------------------------------------------------

TimeZoneFactory::TimeZoneFactory() : mImpl(new Impl()) {}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

TimeZoneFactory::~TimeZoneFactory() {}

// ----------------------------------------------------------------------
/*!
 * \brief Set the time zone coordinate database filename
 */
// ----------------------------------------------------------------------

void TimeZoneFactory::set_coordinate_file(const string& file)
{
  WriteLock lock(mImpl->mCoordinatesMutex);
  mImpl->mCoordinates.reset(new WorldTimeZones(file));
}

// ----------------------------------------------------------------------
/*!
 * \brief Set the time region database filename
 */
// ----------------------------------------------------------------------

void TimeZoneFactory::set_region_file(const string& file)
{
  WriteLock lock(mImpl->mRegionsMutex);
  mImpl->mRegions.reset(new boost::local_time::tz_database());
  mImpl->mRegions->load_from_file(file);
}

// ----------------------------------------------------------------------
/*!
 * \brief List the known databases
 */
// ----------------------------------------------------------------------

vector<string> TimeZoneFactory::region_list()
{
  UpgradeReadLock lock(mImpl->mRegionsMutex);
  if (!mImpl->mRegions)
  {
    UpgradeWriteLock lock2(lock);
    mImpl->load_default_regions();
  }

  return mImpl->mRegions->region_list();
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a region name
 */
// ----------------------------------------------------------------------

boost::local_time::time_zone_ptr TimeZoneFactory::time_zone_from_region(const string& id)
{
  UpgradeReadLock lock(mImpl->mRegionsMutex);
  if (!mImpl->mRegions)
  {
    UpgradeWriteLock lock2(lock);
    mImpl->load_default_regions();
  }

  boost::local_time::time_zone_ptr ptr = mImpl->mRegions->time_zone_from_region(id);
  if (!ptr) throw runtime_error("TimeZoneFactory does not recognize region '" + id + "'");
  return ptr;
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a time zone given a string (region name or posix description)
 */
// ----------------------------------------------------------------------

boost::local_time::time_zone_ptr TimeZoneFactory::time_zone_from_string(const string& desc)
{
  UpgradeReadLock lock(mImpl->mRegionsMutex);
  if (!mImpl->mRegions)
  {
    UpgradeWriteLock lock2(lock);
    mImpl->load_default_regions();
  }

  // Try region name at first
  boost::local_time::time_zone_ptr ptr = mImpl->mRegions->time_zone_from_region(desc);
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

boost::local_time::time_zone_ptr TimeZoneFactory::time_zone_from_coordinate(float lon, float lat)
{
  UpgradeReadLock lock(mImpl->mCoordinatesMutex);
  if (!mImpl->mCoordinates)
  {
    UpgradeWriteLock lock2(lock);
    mImpl->load_default_coordinates();
  }

  string tz = mImpl->mCoordinates->zone_name(lon, lat);
  boost::local_time::time_zone_ptr ptr = time_zone_from_string(tz);
  if (!ptr)
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
  UpgradeReadLock lock(mImpl->mCoordinatesMutex);
  if (!mImpl->mCoordinates)
  {
    UpgradeWriteLock lock2(lock);
    mImpl->load_default_coordinates();
  }
  return mImpl->mCoordinates->zone_name(lon, lat);
}

}  // namespace Fmi
