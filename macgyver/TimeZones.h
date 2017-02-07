// ======================================================================
/*!
 * \file
 * \brief Interface of TimeZones class
 */
// ======================================================================

#pragma once

#include <boost/date_time/local_time/local_time.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Fmi
{
class TimeZones
{
 public:
  ~TimeZones();
  TimeZones();
  TimeZones(const std::string& regionFile, const std::string& coordinateFile);

  std::vector<std::string> region_list() const;

  boost::local_time::time_zone_ptr time_zone_from_string(const std::string& desc) const;
  boost::local_time::time_zone_ptr time_zone_from_region(const std::string& id) const;
  boost::local_time::time_zone_ptr time_zone_from_coordinate(double lon, double lat) const;
  std::string zone_name_from_coordinate(double lon, double lat) const;

 private:
  // Implementation hiding
  class Pimple;
  std::unique_ptr<Pimple> itsPimple;

};  // class TimeZones
}  // namespace Fmi

// ======================================================================
