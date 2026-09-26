#include "ClockCache.h"

#include <regression/tframe.h>
#include <atomic>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace Fmi::Cache;

namespace ClockCacheTest
{

struct custom_type
{
  unsigned int number;
  string text;
};

struct custom_comparator
{
  static std::size_t getSize(const custom_type& object) { return object.number; }
};

template <typename T>
std::set<int> keys(const T& cache)
{
  std::set<int> result;
  for (const auto& item : cache.getContent())
    result.insert(item.itsKey);
  return result;
}

std::string str(const std::set<int>& values)
{
  std::string result;
  for (int value : values)
  {
    if (!result.empty())
      result += ',';
    result += std::to_string(value);
  }
  return result;
}

void testcustomtype()
{
  ClockCache<int, custom_type, custom_comparator, 1> thisCache(115);

  custom_type first = {20, "first"};
  custom_type second = {50, "second"};
  custom_type third = {50, "third"};

  thisCache.insert(1, first);
  thisCache.insert(2, second);
  thisCache.insert(3, third);  // size 120 > 115, the first entry must go

  if (!thisCache.find(2))
    TEST_FAILED("Did not find the second object.");
  if (!thisCache.find(3))
    TEST_FAILED("Did not find the third object.");
  if (thisCache.find(1))
    TEST_FAILED("Shouldn't have found the first object.");
  if (thisCache.size() != 100)
    TEST_FAILED("Cache size should be 100, not " + std::to_string(thisCache.size()));

  custom_type huge = {200, "huge"};
  if (thisCache.insert(4, huge))
    TEST_FAILED("An object larger than the cache should not be inserted");

  TEST_PASSED();
}

// An entry which has been found since the hand last passed it gets a second chance
void testsecondchance()
{
  ClockCache<int, string, TrivialSizeFunction<string>, 1> thisCache(3);

  thisCache.insert(1, "one");
  thisCache.insert(2, "two");
  thisCache.insert(3, "three");
  thisCache.find(1);

  thisCache.insert(4, "four");  // 1 is referenced, 2 is not => 2 is evicted
  if (str(keys(thisCache)) != "1,3,4")
    TEST_FAILED("Expected keys 1,3,4 after the first eviction, got " + str(keys(thisCache)));

  thisCache.insert(5, "five");  // the hand is now at 3 which is unreferenced
  if (str(keys(thisCache)) != "1,4,5")
    TEST_FAILED("Expected keys 1,4,5 after the second eviction, got " + str(keys(thisCache)));

  thisCache.insert(6, "six");  // 1 used its second chance and has not been found since
  if (str(keys(thisCache)) != "4,5,6")
    TEST_FAILED("Expected keys 4,5,6 after the third eviction, got " + str(keys(thisCache)));

  TEST_PASSED();
}

// Entries which are found repeatedly survive a stream of one-off inserts
void testscanresistance()
{
  ClockCache<int, int, TrivialSizeFunction<int>, 1> thisCache(10);

  for (int i = 0; i < 5; i++)
    thisCache.insert(i, i);

  for (int i = 100; i < 1000; i++)
  {
    for (int j = 0; j < 5; j++)
      thisCache.find(j);
    thisCache.insert(i, i);
  }

  for (int j = 0; j < 5; j++)
    if (!thisCache.find(j))
      TEST_FAILED("Frequently used key " + std::to_string(j) + " was evicted");

  if (thisCache.size() != 10)
    TEST_FAILED("Cache size should be 10, not " + std::to_string(thisCache.size()));

  TEST_PASSED();
}

void testsize()
{
  ClockCache<int, string, TrivialSizeFunction<string>, 1> thisCache(3);

  for (int i = 0; i < 10; i++)
    thisCache.insert(i, "value");

  if (thisCache.size() != 3)
    TEST_FAILED("Wrong cache size: " + std::to_string(thisCache.size()) + ", should be 3");
  if (thisCache.maxSize() != 3)
    TEST_FAILED("Wrong max size: " + std::to_string(thisCache.maxSize()) + ", should be 3");
  if (str(keys(thisCache)) != "7,8,9")
    TEST_FAILED("Expected the newest keys 7,8,9, got " + str(keys(thisCache)));

  if (thisCache.insert(9, "again"))
    TEST_FAILED("Inserting an existing key should fail");
  if (*thisCache.find(9) != "value")
    TEST_FAILED("Inserting an existing key should not change the value");

  TEST_PASSED();
}

void testevictionvector()
{
  ClockCache<int, string, TrivialSizeFunction<string>, 1> thisCache(3);

  ClockCache<int, string, TrivialSizeFunction<string>, 1>::ItemVector evictedItems;

  thisCache.insert(1, "moi", evictedItems);
  thisCache.insert(2, "hei", evictedItems);
  thisCache.insert(3, "data", evictedItems);
  if (!evictedItems.empty())
    TEST_FAILED("Evicted items should be empty while the cache is not full");

  thisCache.insert(4, "terve", evictedItems);
  if (evictedItems.size() != 1)
    TEST_FAILED("Evicted item vector should have one element, not " +
                std::to_string(evictedItems.size()));
  if (evictedItems[0].first != 1 || evictedItems[0].second != "moi")
    TEST_FAILED("Incorrect item in eviction vector: '" + std::to_string(evictedItems[0].first) +
                " : " + evictedItems[0].second + "'");

  TEST_PASSED();
}

void testcounters()
{
  ClockCache<int, string, TrivialSizeFunction<string>, 1> thisCache(5);

  auto stats = thisCache.statistics();
  if (stats.size != 0 || stats.inserts != 0 || stats.hits != 0 || stats.misses != 0)
    TEST_FAILED("Empty cache statistics should be zero");
  if (stats.maxsize != 5)
    TEST_FAILED("Max size should be 5, not " + std::to_string(stats.maxsize));

  thisCache.find(1);
  for (int i = 1; i <= 5; i++)
    thisCache.insert(i, "value");

  std::size_t hits = 0;
  thisCache.find(3, hits);
  if (hits != 1)
    TEST_FAILED("Entry hit count should be 1 after one find, not " + std::to_string(hits));
  thisCache.find(3, hits);
  if (hits != 2)
    TEST_FAILED("Entry hit count should be 2 after two finds, not " + std::to_string(hits));
  thisCache.find(-1);

  thisCache.insert(6, "value");
  thisCache.insert(7, "value");

  stats = thisCache.statistics();
  if (stats.inserts != 7)
    TEST_FAILED("Insert count should be 7, not " + std::to_string(stats.inserts));
  if (stats.hits != 2)
    TEST_FAILED("Hit count should be 2, not " + std::to_string(stats.hits));
  if (stats.misses != 2)
    TEST_FAILED("Miss count should be 2, not " + std::to_string(stats.misses));
  if (stats.evictions != 2)
    TEST_FAILED("Eviction count should be 2, not " + std::to_string(stats.evictions));
  if (stats.size != 5)
    TEST_FAILED("Size should be 5, not " + std::to_string(stats.size));

  for (const auto& item : thisCache.getContent())
    if (item.itsKey == 3 && item.itsHits != 2)
      TEST_FAILED("Reported hit count of key 3 should be 2, not " + std::to_string(item.itsHits));

  TEST_PASSED();
}

void testupsert()
{
  ClockCache<int, custom_type, custom_comparator, 1> thisCache(100);

  thisCache.upsert(1, custom_type{30, "first"});
  thisCache.upsert(2, custom_type{30, "second"});
  thisCache.upsert(1, custom_type{50, "replaced"});

  auto value = thisCache.find(1);
  if (!value || value->text != "replaced")
    TEST_FAILED("Upsert did not replace the value");
  if (thisCache.size() != 80)
    TEST_FAILED("Size should be 80 after the upsert, not " + std::to_string(thisCache.size()));

  auto stats = thisCache.statistics();
  if (stats.inserts != 3)
    TEST_FAILED("Upsert should count as an insert");
  if (stats.evictions != 0)
    TEST_FAILED("Replacing a value should not count as an eviction");

  if (thisCache.upsert(1, custom_type{200, "huge"}))
    TEST_FAILED("Upsert of a value larger than the cache should fail");
  if (thisCache.find(1))
    TEST_FAILED("A failed upsert removes the old value, as in Cache");

  TEST_PASSED();
}

// Removing the entry the hand points to must move the hand to the next entry
void testupsertathand()
{
  ClockCache<int, string, TrivialSizeFunction<string>, 1> thisCache(3);

  thisCache.insert(1, "one");
  thisCache.insert(2, "two");
  thisCache.insert(3, "three");
  thisCache.find(1);
  thisCache.insert(4, "four");  // evicts 2, the hand is left at 3

  thisCache.upsert(3, "THREE");  // removes the entry at the hand and adds it again
  thisCache.insert(5, "five");

  if (thisCache.size() != 3)
    TEST_FAILED("Size should be 3, not " + std::to_string(thisCache.size()));
  if (keys(thisCache).size() != 3)
    TEST_FAILED("Content should have 3 keys, got " + str(keys(thisCache)));
  if (!thisCache.find(5))
    TEST_FAILED("The newest entry should be in the cache");

  TEST_PASSED();
}

void testresize()
{
  ClockCache<int, int, TrivialSizeFunction<int>, 1> thisCache(10);
  for (int i = 0; i < 10; i++)
    thisCache.insert(i, i);

  ClockCache<int, int, TrivialSizeFunction<int>, 1>::ItemVector evictedItems;
  thisCache.resize(4, evictedItems);
  if (thisCache.size() != 4 || evictedItems.size() != 6)
    TEST_FAILED("Resize to 4 should leave 4 entries and evict 6, got " +
                std::to_string(thisCache.size()) + " and " + std::to_string(evictedItems.size()));

  thisCache.resize(20);
  for (int i = 100; i < 120; i++)
    thisCache.insert(i, i);
  if (thisCache.size() != 20)
    TEST_FAILED("Size should be 20 after growing, not " + std::to_string(thisCache.size()));

  thisCache.clear();
  if (thisCache.size() != 0 || !thisCache.getContent().empty())
    TEST_FAILED("Cache should be empty after clear");
  thisCache.insert(1, 1);
  if (!thisCache.find(1))
    TEST_FAILED("Cache should work after clear");

  TEST_PASSED();
}

void testshards()
{
  ClockCache<int, int> thisCache(1600);  // 16 shards

  for (int i = 0; i < 10000; i++)
    thisCache.insert(i, i);

  if (thisCache.size() > 1600)
    TEST_FAILED("Size " + std::to_string(thisCache.size()) + " exceeds the capacity");
  if (thisCache.getContent().size() != thisCache.size())
    TEST_FAILED("Content size does not match the reported size");

  for (const auto& item : thisCache.getContent())
    if (item.itsKey != item.itsValue)
      TEST_FAILED("Wrong value for key " + std::to_string(item.itsKey));

  TEST_PASSED();
}

// Hammer a single shard from several threads with a mix of hits, misses, inserts,
// upserts and evictions, and check the bookkeeping afterwards.
void testconcurrent()
{
  const int capacity = 64;
  const int keyspace = 2 * capacity;  // roughly half of the lookups miss
  const int nthreads = 8;
  const int iterations = 50000;

  ClockCache<int, int, TrivialSizeFunction<int>, 1> cache(capacity);
  for (int i = 0; i < capacity; ++i)
    cache.insert(i, i * 10);

  std::atomic<int> errors{0};
  std::atomic<std::size_t> finds{0};

  std::vector<std::thread> threads;
  for (int t = 0; t < nthreads; ++t)
  {
    threads.emplace_back(
        [&, t]()
        {
          std::mt19937 rng(t);
          for (int i = 0; i < iterations; ++i)
          {
            int key = static_cast<int>(rng() % keyspace);
            auto value = cache.find(key);
            finds.fetch_add(1);
            if (value)
            {
              if (*value != key * 10)
                errors.fetch_add(1);
            }
            else if (i % 7 == 0)
            {
              cache.upsert(key, key * 10);
            }
            else
            {
              cache.insert(key, key * 10);
            }
          }
        });
  }
  for (auto& thread : threads)
    thread.join();

  if (errors.load() != 0)
    TEST_FAILED("Concurrent finds returned " + std::to_string(errors.load()) + " wrong values");

  auto stats = cache.statistics();
  if (stats.hits + stats.misses != finds.load())
    TEST_FAILED("Hits (" + std::to_string(stats.hits) + ") + misses (" +
                std::to_string(stats.misses) + ") != finds (" + std::to_string(finds.load()) + ")");
  if (stats.hits == 0 || stats.misses == 0)
    TEST_FAILED("Test should produce both hits and misses");
  if (cache.size() > static_cast<std::size_t>(capacity))
    TEST_FAILED("Cache size " + std::to_string(cache.size()) + " exceeds capacity");

  auto content = cache.getContent();
  if (content.size() != cache.size())
    TEST_FAILED("Cache content size does not match reported size");
  for (const auto& item : content)
    if (item.itsValue != item.itsKey * 10)
      TEST_FAILED("Cache content corrupted for key " + std::to_string(item.itsKey));

  TEST_PASSED();
}

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }
  void test(void)
  {
    TEST(testcustomtype);
    TEST(testsecondchance);
    TEST(testscanresistance);
    TEST(testsize);
    TEST(testevictionvector);
    TEST(testcounters);
    TEST(testupsert);
    TEST(testupsertathand);
    TEST(testresize);
    TEST(testshards);
    TEST(testconcurrent);
  }
};
}  // namespace ClockCacheTest

int main(void)
{
  cout << endl << "ClockCache" << endl << "==========" << endl;
  ClockCacheTest::tests t;
  return t.run();
}
