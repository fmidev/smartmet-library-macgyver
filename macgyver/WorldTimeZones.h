// ======================================================================
/*!
 * \brief Interface of class WorldTimeZones
 *
 * A reader for rasters packed with shapepack (smartmet-shapetools). Despite
 * the name, this is no longer used for timezones: coordinate to timezone
 * resolution is done by Fmi::TimeZoneFinder in smartmet-library-gis.
 *
 * Constructed objects are immutable and hence thread safe
 */
// ======================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Fmi
{
class WorldTimeZones
{
 public:
  WorldTimeZones(const std::string& file);
  ~WorldTimeZones();
  WorldTimeZones(const WorldTimeZones&) = delete;
  WorldTimeZones& operator=(const WorldTimeZones&) = delete;

  const std::string& zone_name(float lon, float lat) const;
  std::vector<std::string> zones() const { return itsZones; }

 private:
  unsigned int itsWidth = 0;
  unsigned int itsHeight = 0;
  float itsLon1 = 0.0F;
  float itsLat1 = 0.0F;
  float itsLon2 = 0.0F;
  float itsLat2 = 0.0F;

  std::vector<std::string> itsZones;

  uint32_t itsSize = 0;
  char* itsData = nullptr;

};  // class WorldTimeZones
}  // namespace Fmi

// ======================================================================
