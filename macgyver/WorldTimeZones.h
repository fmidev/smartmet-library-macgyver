// ======================================================================
/*!
 * \brief Interface of class WorldTimeZones
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

  const std::string& zone_name(float lon, float lat) const;
  std::vector<std::string> zones() const { return itsZones; }

 private:
  unsigned int itsWidth = 0;
  unsigned int itsHeight = 0;
  float itsLon1 = 0;
  float itsLat1 = 0;
  float itsLon2 = 0;
  float itsLat2 = 0;

  std::vector<std::string> itsZones;

  uint32_t itsSize = 0;
  char* itsData = nullptr;

};  // class WorldTimeZones
}  // namespace Fmi

// ======================================================================
