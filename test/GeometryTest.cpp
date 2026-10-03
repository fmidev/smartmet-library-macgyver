#define BOOST_TEST_MODULE "GeometryTest"

#include "Geometry.h"
#include <boost/test/included/unit_test.hpp>
#include <cmath>

using namespace Fmi::Geometry;

BOOST_AUTO_TEST_CASE(radians_and_degrees)
{
  BOOST_CHECK_CLOSE(Radians(180), M_PI, 1e-12);
  BOOST_CHECK_CLOSE(Degrees(M_PI / 2), 90.0, 1e-12);
  BOOST_CHECK_CLOSE(Degrees(Radians(123.456)), 123.456, 1e-12);
}

BOOST_AUTO_TEST_CASE(cartesian_distance)
{
  BOOST_CHECK_CLOSE(Distance(0, 0, 3, 4), 5.0, 1e-12);
  BOOST_CHECK_CLOSE(Distance(1, 2, 3, 3, 4, 5), std::sqrt(4.0 + 4 + 4), 1e-12);
  BOOST_CHECK_EQUAL(Distance(1, 1, 1, 1), 0.0);
}

BOOST_AUTO_TEST_CASE(geo_distance)
{
  BOOST_CHECK_EQUAL(GeoDistance(24.94, 60.17, 24.94, 60.17), 0.0);

  // The distances are in meters. One degree along the equator or a meridian is about 111 km.
  BOOST_CHECK_CLOSE(GeoDistance(0, 0, 1, 0), 111200, 0.1);
  BOOST_CHECK_CLOSE(GeoDistance(25, 60, 25, 61), 111200, 0.1);
  // One degree of longitude at 60N is half of that
  BOOST_CHECK_CLOSE(GeoDistance(25, 60, 26, 60), 55600, 0.2);

  // Helsinki - Tallinn is about 82 km, matching the cross section plugin tests
  BOOST_CHECK_CLOSE(GeoDistance(24.9354, 60.1695, 24.7535, 59.4370), 82100, 0.2);

  // Symmetry and antipodal points
  BOOST_CHECK_CLOSE(GeoDistance(10, 20, 30, 40), GeoDistance(30, 40, 10, 20), 1e-9);
  BOOST_CHECK_CLOSE(GeoDistance(0, 0, 180, 0), M_PI * 6371220, 1e-9);
}

BOOST_AUTO_TEST_CASE(distance_from_line_segment)
{
  // Perpendicular distance to the segment
  BOOST_CHECK_CLOSE(DistanceFromLineSegment(5, 3, 0, 0, 10, 0), 3.0, 1e-12);
  // Beyond the end points the distance is to the nearer end point
  BOOST_CHECK_CLOSE(DistanceFromLineSegment(-3, 4, 0, 0, 10, 0), 5.0, 1e-12);
  BOOST_CHECK_CLOSE(DistanceFromLineSegment(13, 4, 0, 0, 10, 0), 5.0, 1e-12);
  // A degenerate segment is a point
  BOOST_CHECK_CLOSE(DistanceFromLineSegment(3, 4, 0, 0, 0, 0), 5.0, 1e-12);
  // A point on the segment
  BOOST_CHECK_SMALL(DistanceFromLineSegment(5, 5, 0, 0, 10, 10), 1e-12);
}

BOOST_AUTO_TEST_CASE(bearing)
{
  BOOST_CHECK_SMALL(Bearing(25, 60, 25, 61), 1e-9);
  BOOST_CHECK_CLOSE(Bearing(25, 60, 25, 59), 180.0, 1e-9);
  BOOST_CHECK_CLOSE(Bearing(0, 0, 1, 0), 90.0, 1e-9);
  BOOST_CHECK_CLOSE(Bearing(0, 0, -1, 0), 270.0, 1e-9);
  // Kumpula -> Kaisaniemi: about 0.92 km west and 3.10 km south
  BOOST_CHECK_CLOSE(Bearing(24.96131, 60.20307, 24.94459, 60.17523), 196.6, 0.05);
  // The result is always in the range 0...360
  for (double lon = -180; lon <= 180; lon += 37)
  {
    double b = Bearing(0, 0, lon, 10);
    BOOST_CHECK_GE(b, 0.0);
    BOOST_CHECK_LT(b, 360.0);
  }
}
