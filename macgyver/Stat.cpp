#include "Stat.h"

#include <boost/foreach.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_variance.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>
#include <boost/accumulators/statistics/weighted_median.hpp>

#include <cmath>
#include <iterator>
#include <stdexcept>
#include <numeric>

namespace Fmi
{
namespace Stat
{
#define MODULO_VALUE_360 360.0
typedef DataVector::iterator DataIterator;
typedef DataVector::const_iterator DataConstIterator;

using namespace boost::accumulators;
using namespace std;

bool comp_value(const DataItem& data1, const DataItem& data2) { return data1.value < data2.value; }
bool comp_time(const DataItem& data1, const DataItem& data2) { return data1.time < data2.time; }
Stat::Stat(double theMissingValue /*= numeric_limits<double>::quiet_NaN()*/)
    : itsMissingValue(theMissingValue), itsWeights(true), itsDegrees(false)
{
}

Stat::Stat(const DataVector& theData,
           double theMissingValue /*= numeric_limits<double>::quiet_NaN()*/)
    : itsData(theData), itsMissingValue(theMissingValue), itsWeights(true), itsDegrees(false)
{
  calculate_weights();
}

Stat::Stat(const std::vector<double>& theValues,
           double theMissingValue /*= std::numeric_limits<double>::quiet_NaN()*/)
    : itsMissingValue(theMissingValue), itsWeights(false), itsDegrees(false)
{
  addData(theValues);
}

Stat::Stat(const LocalDateTimeValueVector& theValues,
           double theMissingValue /*= std::numeric_limits<double>::quiet_NaN()*/)
    : itsMissingValue(theMissingValue), itsWeights(true), itsDegrees(false)

{
  BOOST_FOREACH (const LocalDateTimeValue& item, theValues)
  {
    itsData.push_back(DataItem(item.first.utc_time(), item.second));
  }
  calculate_weights();
}

Stat::Stat(const PosixTimeValueVector& theValues,
           double theMissingValue /*= std::numeric_limits<double>::quiet_NaN(*/)
    : itsMissingValue(theMissingValue), itsWeights(true), itsDegrees(false)
{
  BOOST_FOREACH (const PosixTimeValue& item, theValues)
  {
    itsData.push_back(DataItem(item.first, item.second));
  }
  calculate_weights();
}

void Stat::setData(const DataVector& theData)
{
  itsData = theData;
  calculate_weights();
}

void Stat::addData(double theValue)
{
  itsData.push_back(DataItem(not_a_date_time, theValue));
  calculate_weights();
}

void Stat::addData(const boost::local_time::local_date_time& theTime, double theValue)
{
  itsData.push_back(DataItem(theTime.utc_time(), theValue));
  calculate_weights();
}

void Stat::addData(const boost::posix_time::ptime& theTime, double theValue)
{
  itsData.push_back(DataItem(theTime, theValue));
  calculate_weights();
}

void Stat::addData(const vector<double>& theData)
{
  BOOST_FOREACH (double value, theData)
    itsData.push_back(DataItem(not_a_date_time, value));
  calculate_weights();
}

void Stat::addData(const DataItem& theData)
{
  itsData.push_back(theData);
  calculate_weights();
}

void Stat::clear() { itsData.clear(); }
// time in seconds is used as a weight
double Stat::integ(const ptime& startTime /*= not_a_date_time */,
                   const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "integ(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime)) return itsMissingValue;

  accumulator_set<double, stats<tag::sum>, double> acc;

  BOOST_FOREACH (const DataItem& item, subvector)
  {
    acc(item.value, weight = item.weight);
  }

  if (itsDegrees)
    return fmod(boost::accumulators::sum(acc), MODULO_VALUE_360) / 3600.0;
  else
    return boost::accumulators::sum(acc) / 3600.0;
}

// weight is alwaus 1.0
double Stat::sum(const ptime& startTime /*= not_a_date_time */,
                 const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "sum(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime)) return itsMissingValue;

  accumulator_set<double, stats<tag::sum>, double> acc;

  BOOST_FOREACH (const DataItem& item, subvector)
  {
    acc(item.value, weight = 1.0);
  }

  if (itsDegrees)
    return fmod(boost::accumulators::sum(acc), MODULO_VALUE_360);
  else
    return boost::accumulators::sum(acc);
}

double Stat::min(const ptime& startTime /*= not_a_date_time */,
                 const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "min(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime) || subvector.size() == 0)
    return itsMissingValue;

  return std::min_element(subvector.begin(), subvector.end(), comp_value)->value;
}

double Stat::mean(const ptime& startTime /*= not_a_date_time */,
                  const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "mean(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime)) return itsMissingValue;

  if (itsDegrees)
  {
    double previousDirection = 0;
    double directionSum = 0;
    for (unsigned int i = 0; i < subvector.size(); i++)
    {
      if (i == 0)
      {
        directionSum = subvector[i].value;
        previousDirection = subvector[i].value;
      }
      else
      {
        double diff = subvector[i].value - previousDirection;
        double direction = previousDirection + diff;

        if (diff < -MODULO_VALUE_360 / 2.0)
          direction += MODULO_VALUE_360;
        else if (diff > MODULO_VALUE_360 / 2.0)
          direction -= MODULO_VALUE_360;

        directionSum += direction;
        previousDirection = direction;
      }
    }

    double mean = directionSum / subvector.size();
    mean -= (MODULO_VALUE_360 * floor(mean / MODULO_VALUE_360));
    return mean;
  }

  accumulator_set<double, stats<tag::mean>, double> acc;
  BOOST_FOREACH (const DataItem& item, subvector)
  {
    acc(item.value, weight = (itsWeights ? item.weight : 1.0));
  }
  return boost::accumulators::mean(acc);
}

double Stat::max(const ptime& startTime /*= not_a_date_time */,
                 const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "max(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime)) return itsMissingValue;

  return std::max_element(subvector.begin(), subvector.end(), comp_value)->value;
}

double Stat::change(const ptime& startTime /*= not_a_date_time */,
                    const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "change(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime) || subvector.size() == 0)
    return itsMissingValue;

  if (itsDegrees)
  {
    double previousValue = 0;
    double cumulativeChange = 0;
    for (unsigned int i = 0; i < subvector.size(); i++)
    {
      if (i > 0)
      {
        double diff = subvector[i].value - previousValue;
        if (diff < -MODULO_VALUE_360 / 2.0)
          cumulativeChange += diff + MODULO_VALUE_360;
        else if (diff > MODULO_VALUE_360 / 2.0)
          cumulativeChange += diff - MODULO_VALUE_360;
        else
          cumulativeChange += diff;
      }
      previousValue = subvector[i].value;
    }
    return cumulativeChange;
  }

  double firstValue(subvector.begin()->value);
  double lastValue((subvector.end() - 1)->value);

  return lastValue - firstValue;
}

double Stat::trend(const ptime& startTime /*= not_a_date_time */,
                   const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "trend(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime) || subvector.size() <= 1)
    return itsMissingValue;

  long positiveChanges(0);
  long negativeChanges(0);
  double lastValue = 0;

  for (DataIterator iter = subvector.begin(); iter != subvector.end(); iter++)
  {
    if (iter != subvector.begin())
    {
      if (itsDegrees)
      {
        const double diff = iter->value - lastValue;
        if (diff < -MODULO_VALUE_360 / 2.0)
          ++positiveChanges;
        else if (diff > MODULO_VALUE_360 / 2.0)
          ++negativeChanges;
        else if (diff < 0)
          ++negativeChanges;
        else if (diff > 0)
          ++positiveChanges;
      }
      else
      {
        if (iter->value > lastValue)
          ++positiveChanges;
        else if (iter->value < lastValue)
          ++negativeChanges;
      }
    }
    lastValue = iter->value;
  }

  return static_cast<double>(positiveChanges - negativeChanges) /
         static_cast<double>(subvector.size() - 1) * 100.0;
}

unsigned int Stat::count(double lowerLimit,
                         double upperLimit,
                         const ptime& startTime /*= not_a_date_time */,
                         const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "trend(" << lowerLimit << ", " << upperLimit << ", " << startTime << ", " << endTime
            << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime))
    return static_cast<unsigned int>(itsMissingValue);

  unsigned int occurances(0);

  BOOST_FOREACH (const DataItem& item, subvector)
  {
    if (item.value >= lowerLimit && item.value <= upperLimit) occurances++;
  }

  return occurances;
}

double Stat::percentage(double lowerLimit,
                        double upperLimit,
                        const ptime& startTime /*= not_a_date_time */,
                        const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "percentage(" << lowerLimit << ", " << upperLimit << ", " << startTime << ", "
            << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime)) return itsMissingValue;

  int occurances(0);
  int total_count(0);

  BOOST_FOREACH (const DataItem& item, subvector)
  {
    if (item.value >= lowerLimit && item.value <= upperLimit)
      occurances += (itsWeights ? item.weight : 1);
    total_count += (itsWeights ? item.weight : 1);
  }

  if (occurances == 0) return 0.0;

  return static_cast<double>((static_cast<double>(occurances) / static_cast<double>(total_count)) *
                             100.0);
}

double Stat::median(const ptime& startTime /*= not_a_date_time */,
                    const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "median(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime) || subvector.size() == 0)
    return itsMissingValue;
  else if (subvector.size() == 1)
    return subvector[0].value;

  vector<double> double_vector;

  BOOST_FOREACH (const DataItem& item, subvector)
  {
    double_vector.insert(
        double_vector.end(), static_cast<std::size_t>(itsWeights ? item.weight : 1.0), item.value);
  }

  std::size_t vector_size = double_vector.size();

  std::size_t middle_index = (vector_size / 2) + 1;

  auto middle_pos = double_vector.begin();
  std::advance(middle_pos, middle_index);
  partial_sort(double_vector.begin(), middle_pos, double_vector.end());

  if (double_vector.size() % 2 == 0)
  {
    return ((double_vector[static_cast<std::size_t>((vector_size / 2) - 1)] +
             double_vector[(vector_size / 2)]) /
            2.0);
  }

  return double_vector[static_cast<std::size_t>(((vector_size + 1) / 2) - 1)];
}

double Stat::variance(const ptime& startTime /*= not_a_date_time */,
                      const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "variance(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  DataVector subvector;

  if (!get_subvector(subvector, startTime, endTime) || subvector.size() == 0)
    return itsMissingValue;

  accumulator_set<double, stats<tag::variance>, double> acc;

  BOOST_FOREACH (const DataItem& item, subvector)
  {
    acc(item.value, weight = (itsWeights ? item.weight : 1.0));
  }

  return boost::accumulators::variance(acc);
}

double Stat::stddev(const ptime& startTime /*= not_a_date_time */,
                    const ptime& endTime /*= not_a_date_time */) const
{
#ifdef MYDEBUG
  std::cout << "stddev(" << startTime << ", " << endTime << ")" << std::endl;
#endif
  if (itsDegrees)
  {
    DataVector subvector;

    if (!get_subvector(subvector, startTime, endTime) || subvector.size() == 0)
      return itsMissingValue;

    double sum = 0;
    double squaredSum = 0;
    double previousDirection = 0;
    for (unsigned int i = 0; i < subvector.size(); i++)
    {
      if (i == 0)
      {
        sum = subvector[i].value;
        squaredSum = subvector[i].value * subvector[i].value;
        previousDirection = subvector[i].value;
      }
      else
      {
        double diff = subvector[i].value - previousDirection;
        double dir = previousDirection + diff;
        if (diff < -MODULO_VALUE_360 / 2.0)
        {
          while (dir < MODULO_VALUE_360 / 2.0)
            dir += MODULO_VALUE_360;
        }
        else if (diff > MODULO_VALUE_360 / 2.0)
        {
          while (dir > MODULO_VALUE_360 / 2.0)
            dir -= MODULO_VALUE_360;
        }
        sum += dir;
        squaredSum += dir * dir;
        previousDirection = dir;
      }
    }
    double tmp = squaredSum - sum * sum / subvector.size();
    if (tmp < 0)
      return 0.0;
    else
      return sqrt(tmp / subvector.size());
  }

  return sqrt(variance(startTime, endTime));
}

bool extract_subvector(const DataVector& itsData,
                       DataVector& subvector,
                       const ptime& startTime,
                       const ptime& endTime,
                       double itsMissingValue,
                       bool itsWeights)
{
  // if startTime is later than endTime return empty vector
  if ((startTime != not_a_date_time && endTime != not_a_date_time) && startTime > endTime)
    return false;

  ptime firstTimestamp(
      (startTime == not_a_date_time || startTime < itsData[0].time) ? itsData[0].time : startTime);
  ptime lastTimestamp((endTime == not_a_date_time || endTime > itsData[itsData.size() - 1].time)
                          ? itsData[itsData.size() - 1].time
                          : endTime);

  time_period query_period(firstTimestamp, lastTimestamp + microseconds(1));

  // if there is only one element in the vector
  if (itsData.size() == 1)
  {
    if (query_period.contains(itsData[0].time))
    {
      if (itsData[0].value == itsMissingValue) return false;
      subvector.push_back(DataItem(itsData[0].time, itsData[0].value, 1.0));
    }
    return true;
  }

#ifdef MYDEBUG
  std::cout << "query_period: " << query_period << std::endl;
#endif

  for (DataConstIterator iter = itsData.begin(); iter != itsData.end(); iter++)
  {
    if (iter->value == itsMissingValue)
    {
      subvector.clear();
      return false;
    }

    if (!itsWeights)
    {
      if (query_period.contains(iter->time)) subvector.push_back(*iter);
      continue;
    }

    // iterate through the data vector and sort out periods and corresponding weights
    if ((iter + 1) != itsData.end())
    {
      // period between two timesteps
      time_period timestep_period(iter->time, (iter + 1)->time + microseconds(1));
      // if timestep period is inside the queried period handle it
      if (query_period.intersects(timestep_period))
      {
        // first find out intersecting period
        time_period intersection_period(query_period.intersection(timestep_period));
        if (intersection_period.length().total_seconds() == 0) continue;

        // value changes halfway of timestep, so we have to handle first half and second half of
        // separately
        ptime halfway_time(timestep_period.begin() +
                           seconds(timestep_period.length().total_seconds() / 2));
        if (intersection_period.contains(halfway_time))
        {
          time_period first_part_period(intersection_period.begin(),
                                        halfway_time + microseconds(1));
          time_period second_part_period(halfway_time, intersection_period.end());
          subvector.push_back(
              DataItem(iter->time, iter->value, first_part_period.length().total_seconds()));
          subvector.push_back(DataItem(
              (iter + 1)->time, (iter + 1)->value, second_part_period.length().total_seconds()));
        }
        else
        {
          if (intersection_period.begin() >
              halfway_time)  // intersection_period is in the second half
            subvector.push_back(DataItem(
                (iter + 1)->time, (iter + 1)->value, intersection_period.length().total_seconds()));
          else  // intersection_period must be in the first half
            subvector.push_back(
                DataItem(iter->time, iter->value, intersection_period.length().total_seconds()));
        }
      }
    }
  }

  if (subvector.size() == 0) return false;

#ifdef MYDEBUG
  BOOST_FOREACH (const DataItem& item, subvector)
  {
    std::cout << item << std::endl;
  }
#endif

  return true;
}

bool Stat::get_subvector(DataVector& subvector,
                         const ptime& startTime /*= not_a_date_time*/,
                         const ptime& endTime /*= not_a_date_time*/) const
{
  if (invalid_timestamps())
  {
    if (startTime != not_a_date_time || endTime != not_a_date_time)
    {
      throw runtime_error(
          "Error: startTime or endTime defined, but data contains invalid timestamps!");
    }
    else
    {
      for (DataConstIterator iter = itsData.begin(); iter != itsData.end(); iter++)
      {
        if (iter->value == itsMissingValue) return false;
        subvector.push_back(*iter);
      }
      return true;
    }
  }

  return extract_subvector(itsData, subvector, startTime, endTime, itsMissingValue, itsWeights);
}

void Stat::calculate_weights()
{
  if (itsData.size() == 1)
  {
    itsData[0].weight = 1.0;
    return;
  }

  bool invalidTimestampsFound = invalid_timestamps();

  if (!invalidTimestampsFound) sort(itsData.begin(), itsData.end(), comp_time);

  for (unsigned int i = 0; i < itsData.size(); i++)
  {
    if (invalidTimestampsFound)
    {
      itsData[i].weight = 1.0;
    }
    else
    {
      if (i == 0)
      {
        time_duration dur(itsData[i + 1].time - itsData[i].time);
        itsData[i].weight = dur.total_seconds() * 0.5;
      }
      else if (i == itsData.size() - 1)
      {
        time_duration dur(itsData[i].time - itsData[i - 1].time);
        itsData[i].weight = dur.total_seconds() * 0.5;
      }
      else
      {
        time_duration dur_prev(itsData[i].time - itsData[i - 1].time);
        time_duration dur_next(itsData[i + 1].time - itsData[i].time);
        itsData[i].weight = (dur_prev.total_seconds() * 0.5) + (dur_next.total_seconds() * 0.5);
      }
    }
  }
}

bool Stat::invalid_timestamps() const
{
  BOOST_FOREACH (const DataItem& item, itsData)
  {
    if (item.time == not_a_date_time)
    {
      return true;
    }
  }

  return false;
}

std::ostream& operator<<(std::ostream& os, const DataItem& item)
{
  os << "timestamp = " << item.time << std::endl
     << "value = " << item.value << std::endl
     << "weight = " << item.weight << std::endl;

  return os;
}

}  // namespace Stat
}  // namespace Fmi
