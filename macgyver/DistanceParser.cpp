#include "DistanceParser.h"
#include "Exception.h"
#include "StringConversion.h"

namespace Fmi
{
namespace DistanceParser
{
namespace
{
const char* KM = "km";
const char* M = "m";
const char* CM = "cm";
const char* MM = "mm";
const char* MI = "mi";
const char* NMI = "nmi";
const char* YD = "yd";
const char* FT = "ft";
const char* IN = "in";
const char* KILOMETER = "kilometer";
const char* METER = "meter";
const char* CENTIMETER = "centimeter";
const char* MILLIMETER = "millimeter";
const char* MILE = "mile";
const char* NAUTICAL_MILE = "nautical_mile";
const char* YARD = "yard";
const char* FOOT = "foot";
const char* INCH = "inch";
const char* NUMBER_SIGNS = "+-. 1234567890";

// A unit conversion: value * factor, or value / factor
struct Conversion
{
  const char* unit;
  const char* format;
  double factor;
  bool divide;
};

double convert(double value, const std::string& unit, const std::string& format)
{
  static const Conversion conversions[] = {
      {KM, METER, 1000.0, false},
      {KM, CENTIMETER, 100000.0, false},
      {KM, MILLIMETER, 1000000.0, false},
      {KM, MILE, 0.621371, false},
      {KM, NAUTICAL_MILE, 0.539957, false},
      {KM, YARD, 1093.61, false},
      {KM, FOOT, 3280.84, false},
      {KM, INCH, 39370.1, false},
      {M, KILOMETER, 1000.0, true},
      {M, CENTIMETER, 0.01, true},
      {M, MILLIMETER, 0.001, true},
      {M, MILE, 0.000621371, false},
      {M, NAUTICAL_MILE, 0.000539957, false},
      {M, YARD, 1.09361, false},
      {M, FOOT, 3.28084, false},
      {M, INCH, 39.3701, false},
      {CM, METER, 100.0, true},
      {CM, KILOMETER, 100000.0, true},
      {CM, MILLIMETER, 10.0, false},
      {CM, MILE, 0.00000621371, false},
      {CM, NAUTICAL_MILE, 0.00000539957, false},
      {CM, YARD, 0.0109361, false},
      {CM, FOOT, 0.0328084, false},
      {CM, INCH, 0.393701, false},
      {MM, KILOMETER, 1000000.0, true},
      {MM, METER, 1000.0, true},
      {MM, CENTIMETER, 10.0, true},
      {MM, MILE, 0.000000621371, false},
      {MM, NAUTICAL_MILE, 0.000000539957, false},
      {MM, YARD, 0.00109361, false},
      {MM, FOOT, 0.00328084, false},
      {MM, INCH, 0.0393701, false},
      {MI, KILOMETER, 1.609344, false},
      {MI, METER, 1609.344, false},
      {MI, CENTIMETER, 160934.4, false},
      {MI, MILLIMETER, 1609344.0, false},
      {MI, NAUTICAL_MILE, 0.868976, false},
      {MI, YARD, 1760.0, false},
      {MI, FOOT, 5280.0, false},
      {MI, INCH, 63360, false},
      {NMI, KILOMETER, 1.852, false},
      {NMI, METER, 1852.0, false},
      {NMI, CENTIMETER, 185200.0, false},
      {NMI, MILLIMETER, 1852000.0, false},
      {NMI, MILE, 1.15078, false},
      {NMI, YARD, 2025.37, false},
      {NMI, FOOT, 6076.12, false},
      {NMI, INCH, 72913.4, false},
      {YD, KILOMETER, 0.0009144, false},
      {YD, METER, 0.9144, false},
      {YD, CENTIMETER, 91.44, false},
      {YD, MILLIMETER, 914.40, false},
      {YD, MILE, 0.000568182, false},
      {YD, NAUTICAL_MILE, 0.000493737, false},
      {YD, FOOT, 3.0, false},
      {YD, INCH, 36.0, false},
      {FT, KILOMETER, 0.0003048, false},
      {FT, METER, 0.3048, false},
      {FT, CENTIMETER, 30.48, false},
      {FT, MILLIMETER, 304.80, false},
      {FT, MILE, 0.000189394, false},
      {FT, NAUTICAL_MILE, 0.000164579, false},
      {FT, YARD, 0.333333, false},
      {FT, INCH, 12.0, false},
      {IN, KILOMETER, 0.0000254, false},
      {IN, METER, 0.0254, false},
      {IN, CENTIMETER, 2.54, false},
      {IN, MILLIMETER, 25.4, false},
      {IN, MILE, 0.0000157828, false},
      {IN, NAUTICAL_MILE, 0.0000137149, false},
      {IN, YARD, 0.0277778, false},
      {IN, FOOT, 0.0833333, false},
  };

  if (unit.empty())
    return value;

  bool known_unit = false;
  for (const auto& c : conversions)
  {
    if (unit != c.unit)
      continue;
    known_unit = true;
    if (format == c.format)
      return (c.divide ? value / c.factor : value * c.factor);
  }

  if (!known_unit)
    throw Fmi::Exception(BCP, "Invalid unit: " + unit);

  return value;
}
}  // namespace

double parse_kilometer(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, KILOMETER);
  }

  return Fmi::stod(str);
}

double parse_meter(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, METER);
  }

  return Fmi::stod(str);
}

double parse_centimeter(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, CENTIMETER);
  }

  return Fmi::stod(str);
}

double parse_millimeter(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, MILLIMETER);
  }

  return Fmi::stod(str);
}

double parse_mile(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, MILE);
  }

  return Fmi::stod(str);
}

double parse_nautical_mile(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, NAUTICAL_MILE);
  }

  return Fmi::stod(str);
}

double parse_foot(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, FOOT);
  }

  return Fmi::stod(str);
}

double parse_yard(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, YARD);
  }

  return Fmi::stod(str);
}

double parse_inch(const std::string& str)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);

  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));

    return convert(value, unit, INCH);
  }

  return Fmi::stod(str);
}

double parse(const std::string& str, const std::string& format)
{
  std::size_t unit_pos = str.find_first_not_of(NUMBER_SIGNS);
  if (unit_pos != std::string::npos)
  {
    std::string unit = str.substr(unit_pos);
    double value = Fmi::stod(str.substr(0, unit_pos));
    return convert(value, unit, format);
  }

  return Fmi::stod(str);
}

}  // namespace DistanceParser
}  // namespace Fmi
