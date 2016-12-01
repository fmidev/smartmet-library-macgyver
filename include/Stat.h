// ======================================================================
/*!
 * \brief Declaration of Stat class
 *
 * Stat class implements statistical functions.
 * Data can be passed in a constructor call or in addData, SetData functions.
 * Both plain double values and double-timestamp pairs are accepted as input.
 * Timestap can be either boost::local_time::local_date_time or boost::posix_time::ptime.
 * If valid value-timestamp pairs are passed weighted version of statistical function is called
 * unless explicitly denied by calling useWeights(false) function before.
 * Exception is sum and integ-functions. In integ-function weights are always used.
 * In sum-function weight for all values is 1.0.
 * If passed values contain missing value, statistics can not be calculated and value of
 * itsMissingValue
 * data member is returned. Missing value can be passed in constructor call or in
 * setMissingValue(double) function.
 * If useDegrees(true) is called values are handled as degrees, in that case result value is always
 * between 0...360.
 *
 */
// ======================================================================

#pragma once

#include <limits>

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace Fmi
{
namespace Stat
{
using namespace boost::posix_time;

struct DataItem
{
  DataItem(boost::posix_time::ptime tim = not_a_date_time,
           double val = std::numeric_limits<double>::quiet_NaN(),
           double wght = 1.0)
      : time(tim), value(val), weight(wght)
  {
  }
  boost::posix_time::ptime time;
  double value;
  double weight;
};

std::ostream& operator<<(std::ostream& os, const DataItem& item);

typedef std::vector<DataItem> DataVector;
typedef std::pair<boost::local_time::local_date_time, double> LocalDateTimeValue;
typedef std::vector<LocalDateTimeValue> LocalDateTimeValueVector;
typedef std::pair<boost::posix_time::ptime, double> PosixTimeValue;
typedef std::vector<PosixTimeValue> PosixTimeValueVector;

class Stat
{
 public:
  Stat(double theMissingValue = std::numeric_limits<double>::quiet_NaN());
  Stat(const std::vector<double>& theValues,
       double theMissingValue = std::numeric_limits<double>::quiet_NaN());
  Stat(const LocalDateTimeValueVector& theValues,
       double theMissingValue = std::numeric_limits<double>::quiet_NaN());
  Stat(const PosixTimeValueVector& theValues,
       double theMissingValue = std::numeric_limits<double>::quiet_NaN());
  Stat(const DataVector& theValues,
       double theMissingValue = std::numeric_limits<double>::quiet_NaN());

  void setData(const DataVector& theValues);
  void addData(double theValue);
  void addData(const boost::local_time::local_date_time& theTime, double theValue);
  void addData(const boost::posix_time::ptime& theTime, double theValue);
  void addData(const std::vector<double>& theValues);
  void addData(const DataItem& theValue);
  void setMissingValue(double theMissingValue) { itsMissingValue = theMissingValue; }
  void useWeights(bool theWeights = true) { itsWeights = theWeights; }
  void useDegrees(bool theDegrees = true) { itsDegrees = theDegrees; }
  void clear();

  double integ(const ptime& startTime = not_a_date_time,
               const ptime& endTime = not_a_date_time) const;
  double sum(const ptime& startTime = not_a_date_time,
             const ptime& endTime = not_a_date_time) const;
  double min(const ptime& startTime = not_a_date_time,
             const ptime& endTime = not_a_date_time) const;
  double mean(const ptime& startTime = not_a_date_time,
              const ptime& endTime = not_a_date_time) const;
  double max(const ptime& startTime = not_a_date_time,
             const ptime& endTime = not_a_date_time) const;
  double change(const ptime& startTime = not_a_date_time,
                const ptime& endTime = not_a_date_time) const;
  double trend(const ptime& startTime = not_a_date_time,
               const ptime& endTime = not_a_date_time) const;
  unsigned int count(double lowerLimit,
                     double upperLimit,
                     const ptime& startTime = not_a_date_time,
                     const ptime& endTime = not_a_date_time) const;
  double percentage(double lowerLimit,
                    double upperLimit,
                    const ptime& startTime = not_a_date_time,
                    const ptime& endTime = not_a_date_time) const;
  double median(const ptime& startTime = not_a_date_time,
                const ptime& endTime = not_a_date_time) const;
  double variance(const ptime& startTime = not_a_date_time,
                  const ptime& endTime = not_a_date_time) const;
  double stddev(const ptime& startTime = not_a_date_time,
                const ptime& endTime = not_a_date_time) const;

 private:
  bool get_subvector(DataVector& subvector,
                     const ptime& startTime = not_a_date_time,
                     const ptime& endTime = not_a_date_time) const;
  void calculate_weights();
  bool invalid_timestamps() const;

  DataVector itsData;
  double itsMissingValue;
  bool itsWeights;
  bool itsDegrees;
};

}  // namespace Stat
}  // namespace Fmi

// ======================================================================
