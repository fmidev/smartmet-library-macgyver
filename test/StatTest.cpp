// ======================================================================
/*!
 * \file
 * \brief Regression tests for CharsetTools
 */
// ======================================================================

#include "Stat.h"
#include <regression/tframe.h>
#include <cmath>
#include <string>

using std::string;
using namespace Fmi::Stat;

#ifndef _MSC_VER
using std::isnan;
#endif

// Protection against conflicts with global functions
namespace StatTest
{
double precision(double dbl, unsigned int decimals)
{
  return (floor(round(dbl * pow(10, decimals))) / pow(10, decimals));
}

Fmi::Stat::DataVector get_data_vector()
{
  Fmi::Stat::DataVector data_vector;

  data_vector.push_back(DataItem(not_a_date_time, 2.0));
  data_vector.push_back(DataItem(not_a_date_time, 1.0));
  data_vector.push_back(DataItem(not_a_date_time, 5.0));
  data_vector.push_back(DataItem(not_a_date_time, 3.0));
  data_vector.push_back(DataItem(not_a_date_time, 4.0));

  return data_vector;
}

Fmi::Stat::DataVector get_data_vector_t()
{
  Fmi::Stat::DataVector data_vector;

  data_vector.push_back(DataItem(time_from_string("2013-12-02 14:00:00"), 2.0));
  data_vector.push_back(DataItem(time_from_string("2013-12-02 15:00:00"), 1.0));
  data_vector.push_back(DataItem(time_from_string("2013-12-02 17:00:00"), 5.0));
  data_vector.push_back(DataItem(time_from_string("2013-12-02 20:00:00"), 3.0));
  data_vector.push_back(DataItem(time_from_string("2013-12-02 22:00:00"), 4.0));

  return data_vector;
}

void integ()
{
  // sum of whole time series
  Stat stat;

  stat.addData(time_from_string("2013-12-02 12:00:00"), 2.0);
  stat.addData(time_from_string("2013-12-02 13:00:00"), 3.0);
  stat.addData(time_from_string("2013-12-02 14:00:00"), 1.0);

  // #1 Precipitation of 2 hours: 1/2h 2mm/h, 1h 3.0mm/h, 1/2h 1.0mm/h
  double sum(stat.integ());
  if (sum != 4.5)
  {
    std::stringstream ss;
    ss << "precipitation sum of {2.0, 3.0, 1.0} between 12:00-14:00 is 4.5, not " << sum;
    TEST_FAILED(ss.str());
  }

  // #2 Precipitation of 1 1/2 hours: 1/2h 2mm/h, 1h 3.0mm/h
  sum =
      stat.integ(time_from_string("2013-12-02 12:00:00"), time_from_string("2013-12-02 13:30:00"));

  if (sum != 4.0)
  {
    std::stringstream ss;
    ss << "precipitation sum of {2.0, 3.0}  between 12:00-13:30 is 4.0, not " << sum;
    TEST_FAILED(ss.str());
  }

  // #3 Precipitation of 1 hours: 1/2h 2mm/h, 1/2h 3.0mm/h
  sum =
      stat.integ(time_from_string("2013-12-02 12:00:00"), time_from_string("2013-12-02 13:00:00"));

  if (sum != 2.5)
  {
    std::stringstream ss;
    ss << "precipitation sum of {2.0, 3.0}  between 12:00-13:00 is 2.5, not " << sum;
    TEST_FAILED(ss.str());
  }

  // #4 Precipitation of 1h45min: 1/2h 2mm/h, 1h 3.0mm/h, 15 min 1.0mm/h
  sum =
      stat.integ(time_from_string("2013-12-02 12:15:00"), time_from_string("2013-12-02 13:15:00"));

  if (sum != 2.75)
  {
    std::stringstream ss;
    ss << "precipitation sum of {2.0, 3.0, 1.0}  between 12:15-13:15 is 2.75, not " << sum;
    TEST_FAILED(ss.str());
  }

  // #5 Precipitation of 30min: 15min 2mm/h, 15min 3.0mm/h
  sum =
      stat.integ(time_from_string("2013-12-02 12:45:00"), time_from_string("2013-12-02 13:15:00"));

  if (sum != 1.50)
  {
    std::stringstream ss;
    ss << "precipitation sum of {3.0}  between 12:45-13:15 is 1.50, not " << sum;
    TEST_FAILED(ss.str());
  }

  // #6 Precipitation of 45min: 15min 2mm/h, 15min 3.0mm/h
  sum =
      stat.integ(time_from_string("2013-12-02 12:15:00"), time_from_string("2013-12-02 12:45:00"));

  if (sum != 1.25)
  {
    std::stringstream ss;
    ss << "precipitation sum of (2.0, 3.0}  between 12:15-12:45 is 1.25, not " << sum;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void sum()
{
  // sum of whole time series
  Stat stat(get_data_vector());
  stat.useWeights(false);

  double sum(stat.sum());
  if (sum != 15)
  {
    std::stringstream ss;
    ss << "sum(2.0, 1.0, 5.0, 3.0, 4.0) is 15, not " << sum;
    TEST_FAILED(ss.str());
  }

  // sum of whole time series by using timestamps
  stat.setData(get_data_vector_t());
  sum = stat.sum(time_from_string("2013-12-02 14:00:00"), time_from_string("2013-12-02 22:00:00"));

  if (sum != 15)
  {
    std::stringstream ss;
    ss << "sum(2.0, 1.0, 5.0, 3.0, 4.0) is 15, not " << sum;
    TEST_FAILED(ss.str());
  }

  // sum in the middle of time series, note! starttime between timesteps
  sum = stat.sum(boost::posix_time::time_from_string("2013-12-02 14:05:00"),
                 boost::posix_time::time_from_string("2013-12-02 20:00:00"));
  if (sum != 9)
  {
    std::stringstream ss;
    ss << "sum(1.0, 5.0, 3.0) is 9, not " << sum;
    TEST_FAILED(ss.str());
  }

  // sum in the middle of time series, note! endtime between timesteps
  sum = stat.sum(boost::posix_time::time_from_string("2013-12-02 15:00:00"),
                 boost::posix_time::time_from_string("2013-12-02 21:00:00"));
  if (sum != 9)
  {
    std::stringstream ss;
    ss << "sum(1.0, 5.0, 3.0) is 9, not " << sum;
    TEST_FAILED(ss.str());
  }

  // just one timestamp
  sum = stat.sum(boost::posix_time::time_from_string("2013-12-02 15:00:00"),
                 boost::posix_time::time_from_string("2013-12-02 15:00:00"));
  if (sum != 1)
  {
    std::stringstream ss;
    ss << "sum(1.0) is 1, not " << sum;
    TEST_FAILED(ss.str());
  }

  // sum from the start till 17:00
  sum = stat.sum(boost::posix_time::not_a_date_time,
                 boost::posix_time::time_from_string("2013-12-02 17:00:00"));
  if (sum != 8)
  {
    std::stringstream ss;
    ss << "sum(2.0, 1.0, 5.0) is 8, not " << sum;
    TEST_FAILED(ss.str());
  }

  // sum from 15:00 to the end of time series
  sum = stat.sum(boost::posix_time::time_from_string("2013-12-02 15:00:00"),
                 boost::posix_time::not_a_date_time);

  if (sum != 13)
  {
    std::stringstream ss;
    ss << "sum(1,0, 5.0, 3.0, 4.0) is 13, not " << sum;
    TEST_FAILED(ss.str());
  }

  // starttime later than endtime
  sum = stat.sum(boost::posix_time::time_from_string("2013-12-02 17:00:00"),
                 boost::posix_time::time_from_string("2013-12-02 15:00:00"));
  if (!std::isnan(sum))
  {
    std::stringstream ss;
    ss << "when starttime is later than endtime result must be nan, not " << sum;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void min()
{
  Stat stat(get_data_vector());

  double minimum(stat.min());

  if (minimum != 1.0)
  {
    std::stringstream ss;
    ss << "Minimum value of {2.0, 1.0, 5.0, 3.0, 4.0} is 1.0, not " << minimum;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void max()
{
  Stat stat(get_data_vector());

  double maximum(stat.max());

  if (maximum != 5.0)
  {
    std::stringstream ss;
    ss << "Maximum value of {2.0, 1.0, 5.0, 3.0, 4.0} is 5.0, not " << maximum;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void mean()
{
  Stat stat(get_data_vector_t());
  stat.useWeights(false);

  double mean(stat.mean());

  // mean #1
  if (mean != 3.0)
  {
    std::stringstream ss;
    ss << "Mean value of {2.0, 1.0, 5.0, 3.0, 4.0} is 3.0, not " << mean;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void weighted_mean()
{
  Stat stat(get_data_vector_t());

  // weighted mean
  double mean = stat.mean();

  if (mean != 3.3125)
  {
    std::stringstream ss;
    ss << "Weighted mean value of {2.0, 1.0, 5.0, 3.0, 4.0} and times "
          "{14:00,15:00,17:00,20:00,22:00} is 3.3125, not "
       << mean;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void change()
{
  Stat stat(get_data_vector_t());

  // change between start and end
  double change = stat.change();
  if (change != 2)
  {
    std::stringstream ss;
    ss << "Change of {2.0, 1.0, 5.0, 3.0, 4.0} is 2, not " << change;
    TEST_FAILED(ss.str());
  }

  // change between 15:00 and 20:00
  change = stat.change(boost::posix_time::time_from_string("2013-12-02 15:00:00"),
                       boost::posix_time::time_from_string("2013-12-02 17:00:00"));
  if (change != 4)
  {
    std::stringstream ss;
    ss << "Change of {1.0, 5.0} is 4, not " << change;
    TEST_FAILED(ss.str());
  }

  // change from start till 17:00
  change = stat.change(boost::posix_time::not_a_date_time,
                       boost::posix_time::time_from_string("2013-12-02 17:00:00"));
  if (change != 3)
  {
    std::stringstream ss;
    ss << "Change of {2.0, 1.0, 5.0} is 3, not " << change;
    TEST_FAILED(ss.str());
  }

  // change from 20:00 to the end
  change = stat.change(boost::posix_time::time_from_string("2013-12-02 20:00:00"),
                       boost::posix_time::not_a_date_time);

  if (change != 1)
  {
    std::stringstream ss;
    ss << "Change of {3.0, 4.0} is 1, not " << change;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void trend()
{
  Stat stat(get_data_vector_t());
  stat.useWeights(false);

  // trend during whole period
  double trend = stat.trend();
  if (trend != 0)
  {
    std::stringstream ss;
    ss << "Trend of {2.0, 1.0, 5.0, 3.0, 4.0} is 0, not " << trend;
    TEST_FAILED(ss.str());
  }

  // trend from start till 20
  trend = stat.trend(boost::posix_time::not_a_date_time,
                     boost::posix_time::time_from_string("2013-12-02 20:00:00"));

  if (trend != -((1.0 / 3.0) * 100.0))
  {
    std::stringstream ss;
    ss << "Trend of {2.0, 1.0, 5.0, 3.0} is -33.3333, not " << trend;
    TEST_FAILED(ss.str());
  }

  // trend from 20:00 to the end
  trend = stat.trend(boost::posix_time::time_from_string("2013-12-02 20:00:00"),
                     boost::posix_time::not_a_date_time);

  if (trend != 100.0)
  {
    std::stringstream ss;
    ss << "Trend of {3.0, 4.0} is 100.0, not " << trend;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void count()
{
  Stat stat(get_data_vector_t());
  stat.useWeights(false);

  // count of whole time series
  double count = stat.count(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
  if (count != 5)
  {
    std::stringstream ss;
    ss << "Count of {2.0, 1.0, 5.0, 3.0, 4.0} is 5, not " << count;
    TEST_FAILED(ss.str());
  }

  // count from start till 17:00
  count = stat.count(std::numeric_limits<double>::min(),
                     std::numeric_limits<double>::max(),
                     boost::posix_time::not_a_date_time,
                     boost::posix_time::time_from_string("2013-12-02 17:00:00"));
  if (count != 3)
  {
    std::stringstream ss;
    ss << "Count of {2.0, 1.0, 5.0} is 3, not " << count;
    TEST_FAILED(ss.str());
  }

  // count from 20:00 to the end
  count = stat.count(std::numeric_limits<double>::min(),
                     std::numeric_limits<double>::max(),
                     boost::posix_time::time_from_string("2013-12-02 20:00:00"),
                     boost::posix_time::not_a_date_time);

  if (count != 2)
  {
    std::stringstream ss;
    ss << "Count of {3.0, 4.0} is 2, not " << count;
    TEST_FAILED(ss.str());
  }

  // count of occurances in whole time series where value is between [2.9, 4.1]
  count = stat.count(2.9, 4.1);

  if (count != 2)
  {
    std::stringstream ss;
    ss << "Count of {2.0, 1.0, 5.0, 3.0, 4.0} where value is between [2.9, 4.1] is 2, not "
       << count;
    TEST_FAILED(ss.str());
  }

  // count of occurances between 15:00 and 20:00 where value is between [2.9, 4.1]
  count = stat.count(2.9,
                     4.1,
                     boost::posix_time::time_from_string("2013-12-02 15:00:00"),
                     boost::posix_time::time_from_string("2013-12-02 20:00:00"));

  if (count != 1)
  {
    std::stringstream ss;
    ss << "Count of {1.0, 5.0, 3.0} where value is between [2.9, 4.1] is 1, not " << count;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void percentage()
{
  Stat stat(get_data_vector_t());

  // #1 weighted percentage (calculation is weighted by default)
  // weighted percentage of occurances in the whole time series where value is between 4.5 and 5.5
  double percentage = stat.percentage(4.5, 5.5);

  if (percentage != 31.25)
  {
    std::stringstream ss;
    ss << "Percentage of {2.0, 1.0, 5.0, 3.0, 4.0} where value is between [4.5, 5.5] is 31.25, not "
       << percentage;
    TEST_FAILED(ss.str());
  }

  // #2 weighted percentage of occurances between 17:00 and 22:00  where value is between 3.5 and
  // 5.5
  percentage = stat.percentage(3.5,
                               5.5,
                               boost::posix_time::time_from_string("2013-12-02 17:00:00"),
                               boost::posix_time::time_from_string("2013-12-02 22:00:00"));

  if (percentage != 50.0)
  {
    std::stringstream ss;
    ss << "Percentage of {5.0, 3.0, 4.0} where value is between [3.5, 5.5] is 50.0, not "
       << percentage;
    TEST_FAILED(ss.str());
  }

  // raw percentage
  stat.useWeights(false);

  // #3 raw percentage of occurances in the whole time series where value is between 4.5 and 5.5
  percentage = stat.percentage(4.5, 5.5);

  if (percentage != 20)
  {
    std::stringstream ss;
    ss << "Percentage of {2.0, 1.0, 5.0, 3.0, 4.0} where value is between [4.5, 5.5] is 20, not "
       << percentage;
    TEST_FAILED(ss.str());
  }

  // #3 raw percentage of occurances between 17:00 and 22:00  where value is between 3.5 and 5.5
  percentage = stat.percentage(3.5,
                               5.5,
                               boost::posix_time::time_from_string("2013-12-02 17:00:00"),
                               boost::posix_time::time_from_string("2013-12-02 22:00:00"));

  if (percentage != ((2.0 / 3.0) * 100.0))
  {
    std::stringstream ss;
    ss << "Percentage of {5.0, 3.0, 4.0} where value is between [3.5, 5.5] is 67.6667, not "
       << percentage;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void median()
{
  Stat stat(get_data_vector_t());

  // #1 weighted median of whole time series
  double median = stat.median();

  if (median != 3)
  {
    std::stringstream ss;
    ss << "Median of {2.0, 1.0, 5.0, 3.0, 4.0} is 3, not " << median;
    TEST_FAILED(ss.str());
  }

  // #2 weighted median between 14:00-17:00
  median = stat.median(boost::posix_time::time_from_string("2013-12-02 14:00:00"),
                       boost::posix_time::time_from_string("2013-12-02 17:00:00"));

  if (median != 1.5)
  {
    std::stringstream ss;
    ss << "Median of {2.0, 1.0, 5.0} is 1.5, not " << median;
    TEST_FAILED(ss.str());
  }

  // #3 weighted median between 14:00-20:00
  median = stat.median(boost::posix_time::time_from_string("2013-12-02 14:00:00"),
                       boost::posix_time::time_from_string("2013-12-02 20:00:00"));

  if (median != 3)
  {
    std::stringstream ss;
    ss << "Median of {2.0, 1.0, 5.0, 3.0} is 3, not " << median;
    TEST_FAILED(ss.str());
  }

  stat.useWeights(false);
  // #4 raw median of whole time series
  median = stat.median();

  if (median != 3)
  {
    std::stringstream ss;
    ss << "Median of {2.0, 1.0, 5.0, 3.0, 4.0} is 3, not " << median;
    TEST_FAILED(ss.str());
  }

  stat.useWeights(false);
  // #5 raw median between 14:00-17:00
  median = stat.median(boost::posix_time::time_from_string("2013-12-02 14:00:00"),
                       boost::posix_time::time_from_string("2013-12-02 17:00:00"));

  if (median != 2)
  {
    std::stringstream ss;
    ss << "Median of {2.0, 1.0, 5.0} is 2, not " << median;
    TEST_FAILED(ss.str());
  }

  // #6 raw median between 14:00-20:00
  median = stat.median(boost::posix_time::time_from_string("2013-12-02 14:00:00"),
                       boost::posix_time::time_from_string("2013-12-02 20:00:00"));

  if (median != 2.5)
  {
    std::stringstream ss;
    ss << "Median of {2.0, 1.0, 5.0, 3.0} is 2.5, not " << median;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void variance()
{
  Stat stat(get_data_vector_t());

  stat.useWeights(false);

  double variance = stat.variance();

  // #1 raw variances
  if (precision(variance, 5) != 2.0)
  {
    std::stringstream ss;
    ss << "Variance of {2.0, 1.0, 5.0, 3.0, 4.0} is 2, not " << variance;
    TEST_FAILED(ss.str());
  }

  // #2 raw varince between 14:00-17:00
  variance = stat.variance(boost::posix_time::time_from_string("2013-12-02 14:00:00"),
                           boost::posix_time::time_from_string("2013-12-02 17:00:00"));

  if (precision(variance, 5) != 2.88889)
  {
    std::stringstream ss;
    ss << "Varince of {2.0, 1.0, 5.0} is 2.88889, not " << variance;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void stddev()
{
  Stat stat(get_data_vector_t());

  stat.useWeights(false);

  double stddev = stat.stddev();

  // #1 raw standard deviation
  if (stddev != sqrt(2))
  {
    std::stringstream ss;
    ss << "Standard deviation of {2.0, 1.0, 5.0, 3.0, 4.0} is 1.41421, not " << stddev;
    TEST_FAILED(ss.str());
  }

  // #2 raw standard deviation between 14:00-17:00
  stddev = stat.stddev(boost::posix_time::time_from_string("2013-12-02 14:00:00"),
                       boost::posix_time::time_from_string("2013-12-02 17:00:00"));

  if (stddev != sqrt((pow(5.0 / 3.0, 2.0) + pow(2.0 / 3.0, 2.0) + pow(7.0 / 3.0, 2.0)) / 3.0))
  {
    std::stringstream ss;
    ss << "Standard deviation of {2.0, 1.0, 5.0} is 1.69967, not " << stddev;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void degrees()
{
  Stat stat;

  stat.addData(350);
  stat.addData(90);

  // #1 Sum of degrees
  stat.useDegrees(true);
  double value = stat.sum();

  if (value != 80)
  {
    std::stringstream ss;
    ss << "Sum of 90° and 350° is 80° , not " << value << "°";
    TEST_FAILED(ss.str());
  }

  // #2 Mean
  value = stat.mean();

  if (value != 40)
  {
    std::stringstream ss;
    ss << "Mean of 90° and 350° is 40° , not " << value << "°";
    TEST_FAILED(ss.str());
  }

  // #3 Standard deviation
  value = stat.stddev();

  if (value != 50)
  {
    std::stringstream ss;
    ss << "Standard deviation of 90° and 350° is 50° , not " << value << "°";
    TEST_FAILED(ss.str());
  }

  // #4 Trend
  value = stat.trend();

  if (value != 100.0)
  {
    std::stringstream ss;
    ss << "Trend of 90° and 350° is 100 , not " << value;
    TEST_FAILED(ss.str());
  }

  // #5 count
  value = stat.count(89, 90);
  if (value != 1.0)
  {
    std::stringstream ss;
    ss << "Count of occurances between [89,90] is 1 , not " << value;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

void nearest()
{
  Stat stat(get_data_vector_t());

  // Before timeseries nearest is the first timestep
  double value = stat.nearest(boost::posix_time::time_from_string("2013-12-02 13:00:00"));

  if (value != 2.0)
    TEST_FAILED("Nearest value should be 2.0");

  value = stat.nearest(boost::posix_time::time_from_string("2013-12-02 16:00:00"));

  if (value != 1.0)
    TEST_FAILED("Nearest value should be 1.0");

  value = stat.nearest(boost::posix_time::time_from_string("2013-12-02 16:00:01"));

  if (value != 5.0)
    TEST_FAILED("Nearest value should be 5.0");

  value = stat.nearest(boost::posix_time::time_from_string("2013-12-02 15:59:59"));

  if (value != 1.0)
    TEST_FAILED("Nearest value should be 1.0");

  value = stat.nearest(boost::posix_time::time_from_string("2013-12-02 18::00:00"));

  if (value != 5.0)
    TEST_FAILED("Nearest value should be 5.0");

  value = stat.nearest(boost::posix_time::time_from_string("2013-12-02 19:00:00"));

  if (value != 3.0)
    TEST_FAILED("Nearest value should be 3.0");

  // Exact timestep found  in timeseries
  value = stat.nearest(boost::posix_time::time_from_string("2013-12-02 20:00:00"));

  if (value != 3.0)
    TEST_FAILED("Nearest value should be 3.0");

  value = stat.nearest(boost::posix_time::time_from_string("2013-12-02 21:30:00"));

  if (value != 4.0)
    TEST_FAILED("Nearest value should be 4.0");

  // After the timeseries nearest is the last timestep
  value = stat.nearest(boost::posix_time::time_from_string("2013-12-03 00:30:00"));

  if (value != 4.0)
    TEST_FAILED("Nearest value should be 4.0");

  TEST_PASSED();
}

void interpolate()
{
  Stat stat(get_data_vector_t());

  // Before timeseries -> extrapolate to the past
  double value = stat.interpolate(boost::posix_time::time_from_string("2013-12-02 10:30:00"));

  if (value != 5.5)
    TEST_FAILED("Interpolated value should be 5.5");

  value = stat.interpolate(boost::posix_time::time_from_string("2013-12-02 13:00:00"));

  if (value != 3.0)
    TEST_FAILED("Interpolated value should be 3.0");

  // Exact timestep found  in timeseries
  value = stat.interpolate(boost::posix_time::time_from_string("2013-12-02 15:00:00"));

  if (value != 1.0)
    TEST_FAILED("Interpolated value should be 1.0");

  // Interpolation between values
  value = stat.interpolate(boost::posix_time::time_from_string("2013-12-02 17:10:00"));
  value = precision(value, 2);

  if (value != 4.89)
    TEST_FAILED("Interpolated value should be 4.89");

  value = stat.interpolate(boost::posix_time::time_from_string("2013-12-02 18:00:00"));
  value = precision(value, 2);

  if (value != 4.33)
    TEST_FAILED("Interpolated value should be 4.33");

  value = stat.interpolate(boost::posix_time::time_from_string("2013-12-02 19:30:00"));
  value = precision(value, 2);

  if (value != 3.33)
    TEST_FAILED("Interpolated value should be 3.33");

  // After timeseries -> extrapolate to the future
  value = stat.interpolate(boost::posix_time::time_from_string("2013-12-02 22:30:00"));
  value = precision(value, 2);

  if (value != 4.25)
    TEST_FAILED("Interpolated value should be 4.25");

  value = stat.interpolate(boost::posix_time::time_from_string("2013-12-03 02:00:00"));
  value = precision(value, 2);

  if (value != 6.0)
    TEST_FAILED("Interpolated value should be 6.0");

  TEST_PASSED();
}

void missing_value()
{
  Stat stat;

  stat.addData(boost::posix_time::time_from_string("2013-12-02 14:00:00"), 1);
  stat.addData(boost::posix_time::time_from_string("2013-12-02 15:00:00"), 2);
  stat.addData(boost::posix_time::time_from_string("2013-12-02 17:00:00"), 3);

  // #1 startTime after endTime, default missing value is NaN
  double value = stat.percentage(1,
                                 10,
                                 boost::posix_time::time_from_string("2013-12-02 17:00:00"),
                                 boost::posix_time::time_from_string("2013-12-02 14:00:00"));

  if (!isnan(value))
  {
    std::stringstream ss;
    ss << "Default missing value should be nan, not " << value;
    TEST_FAILED(ss.str());
  }

  // #2 missing value changed to 32700, it is added to the data set
  stat.setMissingValue(32700.0);
  stat.addData(32700.0);
  // percentage of values between 1...10 of whole data set
  value = stat.percentage(1, 10);
  if (value != 32700.0)
  {
    std::stringstream ss;
    ss << "Missing value should be 32700, not " << value;
    TEST_FAILED(ss.str());
  }

  // #3 missing value changed to 100
  stat.setMissingValue(100.0);
  // percentage of values between 1...10 of whole data set
  value = stat.percentage(1, 10);
  if (value != 75.0)
  {
    std::stringstream ss;
    ss << "Share of values between 1...10 is 75% , not " << value;
    TEST_FAILED(ss.str());
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * The actual test suite
 */
// ----------------------------------------------------------------------

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }
  void test(void)
  {
    TEST(sum);
    TEST(integ);
    TEST(min);
    TEST(max);
    TEST(mean);
    TEST(weighted_mean);
    TEST(change);
    TEST(trend);
    TEST(count);
    TEST(percentage);
    TEST(median);
    TEST(stddev);
    TEST(variance);
    TEST(missing_value);
    TEST(degrees);
    TEST(nearest);
    TEST(interpolate);
  }
};

}  // namespace StatTest

//! The main program
int main(void)
{
  using namespace std;
  cout << endl << "Stat tester" << endl << "===================" << endl;
  StatTest::tests t;
  return t.run();
}

// ======================================================================
