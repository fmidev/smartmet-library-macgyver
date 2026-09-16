#include "Cache.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <regression/tframe.h>
#include <atomic>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <thread>

// FIXME: unfortunatelly no similar function to boost::filesystem::unique_pth is
//        present in std::filesystem. As result we have to use boost::filesystem in tests

using namespace std;
using namespace Fmi::Cache;

namespace fs = std::filesystem;

static std::filesystem::path* testpaths[4] = {nullptr};

namespace CacheTest
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

void testfilecache()
{
  fs::path testdir(*testpaths[0]);

  FileCache cache(testdir, 100);

  std::vector<std::pair<std::size_t, std::string> > input = {
      {0, "zero"},
      {1, "first"},
      {2, "second"},
      {500, "five hundred"},
      {std::numeric_limits<std::size_t>::max(), "max"}};

  for (auto& data : input)
  {
    cache.insert(data.first, data.second);
  }

  for (auto& data : input)
  {
    auto result = cache.find(data.first);
    if (!result)
    {
      TEST_FAILED("Did not find key '" + std::to_string((unsigned long long)data.first) +
                  "' from the file cache");
    }
    if (*result != data.second)
    {
      TEST_FAILED("Content mismatch with key '" + std::to_string((unsigned long long)data.first) +
                  "'. Should be '" + data.second + "', got instead '" + *result + "'");
    }
  }

  TEST_PASSED();
}

void testfilecacheorder()
{
  fs::path testdir(*testpaths[1]);

  FileCache cache(testdir, 100);

  std::vector<std::pair<std::size_t, std::string> > input = {
      {100, "zero"},
      {200, "first"},
      {999, "second"},
      {1202, "five hundred"},
  };

  std::vector<std::size_t> correct_order = {

      200,
      100,
      1202,
      999,

  };

  for (auto& data : input)
  {
    cache.insert(data.first, data.second);
  }

  auto unused = cache.find(100);
  unused = cache.find(1202);
  unused = cache.find(999);

  auto content = cache.getContent();

  for (int i = 0; i < 4; ++i)
  {
    if (content[i] != correct_order[i])
    {
      std::ostringstream os;
      os << "Cache content at index '" << i << "' is '" << content[i] << "', should be '"
         << correct_order[i] << "'" << std::endl;
      TEST_FAILED(os.str());
    }
  }

  TEST_PASSED();
}

void testfilecachesize()
{
  fs::path testdir(*testpaths[2]);

  FileCache cache(testdir, 8);

  std::vector<std::pair<std::size_t, std::string> > input = {{1, "1"}, {2, "12"}, {3, "123"}};

  std::vector<std::size_t> correct_order = {

      3, 1, 4

  };

  for (auto& data : input)
  {
    cache.insert(data.first, data.second);
  }

  auto unused = cache.find(1);

  cache.insert(4, "1234");

  auto content = cache.getContent();

  for (int i = 0; i < 3; ++i)
  {
    if (content.at(i) != correct_order.at(i))
    {
      std::ostringstream os;
      os << "Cache content at index '" << i << "' is '" << content[i] << "', should be '"
         << correct_order[i] << "'" << std::endl;
      TEST_FAILED(os.str());
    }
  }

  if (content.size() != 3)
    TEST_FAILED("Incorrect number of cache items, should be 3");

  std::size_t size = cache.getSize();
  if (size != 8)
    TEST_FAILED("Incorrect cache size, should be 8");

  TEST_PASSED();
}

void testcustomtype()
{
  Cache<int, custom_type, custom_comparator, 1> thisCache(115);

  custom_type toInsert, toInsert2, toInsert3;
  toInsert.number = 10;
  toInsert.text = "Hello";
  toInsert2.number = 100;
  toInsert2.text = "HelloWorld";
  toInsert3.number = 6;
  toInsert3.text = "HelloAll";

  thisCache.insert(1, toInsert);
  thisCache.insert(2, toInsert2);

  auto test = thisCache.find(1);

  if (!test)
    TEST_FAILED("Did not find the inserted object.");

  test = thisCache.find(2);
  if (!test)
    TEST_FAILED("Did not find the inserted object.");

  thisCache.insert(3, toInsert3);

  test = thisCache.find(1);
  if (test)
    TEST_FAILED("Shouldn't have found the inserted object.");

  TEST_PASSED();
}

void testlru()
{
  Cache<int, string, TrivialSizeFunction<string>, 1> thisCache(5);

  thisCache.insert(1, "eka");
  thisCache.insert(2, "toka");
  thisCache.insert(3, "kolmas");
  thisCache.insert(4, "neljas");
  thisCache.insert(5, "viides");

  auto value = thisCache.find(4);
  value = thisCache.find(3);
  value = thisCache.find(2);
  value = thisCache.find(1);

  // This must remove the "5" entry from the cache since it is the least recently used
  thisCache.insert(6, "kuudes");

  std::string expected = "neljas,kolmas,toka,eka,kuudes";

  if (expected == thisCache.getTextContent())
    TEST_PASSED();

  TEST_FAILED("Wrong cache content:\"" + thisCache.getTextContent() + "\", expected \"" + expected +
              "\"");
}

void testsize()
{
  Cache<int, string, TrivialSizeFunction<string>, 1> thisCache(3);

  thisCache.insert(1, "moi");
  thisCache.insert(2, "hei");
  thisCache.insert(3, "heippa");
  thisCache.insert(4, "terve");
  thisCache.insert(5, "heihei");

  if (thisCache.size() != 3)
    TEST_FAILED("Wrong cache size: " + boost::lexical_cast<string>(thisCache.size()) +
                ", should be 3");

  TEST_PASSED();
}

void testtagless()
{
  Cache<int, string, TrivialSizeFunction<string>, 1> thisCache(4);

  thisCache.insert(1, "moi");
  thisCache.insert(2, "hei");
  thisCache.insert(3, "heippa");
  thisCache.insert(4, "terve");
  thisCache.insert(5, "heihei");

  auto value1 = thisCache.find(1);

  if (value1)
    TEST_FAILED("Entry 1 should not be in cache after eviction");

  TEST_PASSED();
}

void testevictionvector()
{
  Cache<int, string, TrivialSizeFunction<string>, 1> thisCache(3);

  std::vector<std::pair<int, string> > evictedItems;

  thisCache.insert(1, "moi", evictedItems);
  if (!evictedItems.empty())
  {
    TEST_FAILED("Case '1' : 'moi': Evicted items is not empty.");
  }
  thisCache.insert(2, "hei", evictedItems);
  if (!evictedItems.empty())
  {
    TEST_FAILED("Case '2' : 'hei': Evicted items is not empty.");
  }
  thisCache.insert(3, "data", evictedItems);
  if (!evictedItems.empty())
  {
    TEST_FAILED("Case '3' : 'data': Evicted items is not empty.");
  }

  thisCache.insert(4, "terve", evictedItems);
  if (evictedItems.size() != 1)
  {
    TEST_FAILED("Case '4' : 'terve': Evicted item vector not of size 1.");
  }
  if (evictedItems[0].first != 1 || evictedItems[0].second != "moi")
  {
    std::ostringstream os;
    os << "Incorrect item in eviction vector: '" << evictedItems[0].first << " : "
       << evictedItems[0].second << "'";
    TEST_FAILED(os.str());
  }

  TEST_PASSED();
}

void testcounters()
{
  Cache<int, string, TrivialSizeFunction<string>, 1> thisCache(5);

  auto stats = thisCache.statistics();
  if (stats.size != 0)
    TEST_FAILED("Empty cache size should be zero");
  if (stats.inserts != 0)
    TEST_FAILED("Empty cache insert count should be zero");
  if (stats.hits != 0)
    TEST_FAILED("Empty cache hits should be zero");
  if (stats.misses != 0)
    TEST_FAILED("Empty cache misses should be zero");

  auto value = thisCache.find(1);
  stats = thisCache.statistics();
  if (stats.hits != 0)
    TEST_FAILED("Empty cache hits should be zero after one find call");
  if (stats.misses != 1)
    TEST_FAILED("Empty cache misses should be one after one find call");

  thisCache.insert(1, "one");
  thisCache.insert(2, "two");
  thisCache.insert(3, "three");
  thisCache.insert(4, "four");
  thisCache.insert(5, "five");

  stats = thisCache.statistics();
  if (stats.inserts != 5)
    TEST_FAILED("Cache insert could should be five after five inserts");

  value = thisCache.find(3);
  stats = thisCache.statistics();

  if (stats.hits != 1)
    TEST_FAILED("Cache hits should be one after one succesful find call");
  if (stats.misses != 1)
    TEST_FAILED("Cache misses should be one after one failed find call");

  value = thisCache.find(4);
  value = thisCache.find(-1);
  stats = thisCache.statistics();

  if (stats.hits != 2)
    TEST_FAILED("Cache hits should be two after two succesful find calls");
  if (stats.misses != 2)
    TEST_FAILED("Cache misses should be two after two failed find calls");

  TEST_PASSED();
}

// Hammer a single shard from several threads with a mix of hits, misses,
// inserts and LRU promotions. Verifies that find() with a shared lock plus a
// separate exclusive lock for the splice never returns a wrong value, never
// corrupts the LRU bookkeeping and keeps the hit/miss counters consistent.
void testconcurrentfind()
{
  const int capacity = 64;
  const int keyspace = 2 * capacity;  // roughly half of the lookups miss
  const int nthreads = 8;
  const int iterations = 50000;

  Cache<int, int, TrivialSizeFunction<int>, 1> cache(capacity);
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
                std::to_string(stats.misses) + ") should equal the number of finds (" +
                std::to_string(finds.load()) + ")");
  if (stats.hits == 0 || stats.misses == 0)
    TEST_FAILED("Test should produce both hits and misses");

  if (cache.size() > static_cast<std::size_t>(capacity))
    TEST_FAILED("Cache size " + std::to_string(cache.size()) + " exceeds capacity");

  auto content = cache.getContent();
  if (content.size() != cache.size())
    TEST_FAILED("Cache content size does not match reported size");
  for (const auto& item : content)
  {
    if (item.itsValue != item.itsKey * 10)
      TEST_FAILED("Cache content corrupted for key " + std::to_string(item.itsKey));
  }

  TEST_PASSED();
}

// Find of a non-MRU entry must promote it even though the shared lock is
// released before the exclusive lock is taken.
void testfindpromotes()
{
  Cache<int, string, TrivialSizeFunction<string>, 1> cache(3);
  cache.insert(1, "one");
  cache.insert(2, "two");
  cache.insert(3, "three");

  // Promote the LRU entry, then insert a new one: 2 should be evicted, not 1
  if (!cache.find(1))
    TEST_FAILED("Key 1 should be found");
  cache.insert(4, "four");

  if (cache.find(2))
    TEST_FAILED("Key 2 should have been evicted");
  if (!cache.find(1) || !cache.find(3) || !cache.find(4))
    TEST_FAILED("Keys 1, 3 and 4 should remain in the cache");

  TEST_PASSED();
}

// Same as testconcurrentfind but for the FileCache, whose find() reads the
// file under a shared lock and relocates the entry under an exclusive lock.
void testfilecacheconcurrentfind()
{
  fs::path testdir(*testpaths[3]);

  const std::size_t nkeys = 20;
  const int nthreads = 4;
  const int iterations = 500;

  // Each value is 4 bytes so 16 of them fit in the cache; keys >= 16 cause cleanups
  FileCache cache(testdir, 64);
  auto valueOf = [](std::size_t key)
  { return "v" + std::string(3 - std::to_string(key).size(), '0') + std::to_string(key); };

  for (std::size_t key = 0; key < 16; ++key)
    cache.insert(key, valueOf(key));

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
            std::size_t key = rng() % nkeys;
            auto value = cache.find(key);
            finds.fetch_add(1);
            if (value)
            {
              if (*value != valueOf(key))
                errors.fetch_add(1);
            }
            else
            {
              cache.insert(key, valueOf(key));
            }
          }
        });
  }
  for (auto& thread : threads)
    thread.join();

  if (errors.load() != 0)
    TEST_FAILED("Concurrent FileCache finds returned " + std::to_string(errors.load()) +
                " wrong values");

  auto stats = cache.statistics();
  if (stats.hits + stats.misses != finds.load())
    TEST_FAILED("FileCache hits (" + std::to_string(stats.hits) + ") + misses (" +
                std::to_string(stats.misses) + ") should equal the number of finds (" +
                std::to_string(finds.load()) + ")");

  if (cache.getSize() > 64)
    TEST_FAILED("FileCache size " + std::to_string(cache.getSize()) + " exceeds capacity");

  TEST_PASSED();
}

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }
  void test(void)
  {
    TEST(testfilecache);
    TEST(testfilecacheorder);
    TEST(testfilecachesize);
    TEST(testcustomtype);
    TEST(testsize);
    TEST(testlru);
    TEST(testtagless);
    TEST(testevictionvector);
    TEST(testcounters);
    TEST(testfindpromotes);
    TEST(testconcurrentfind);
    TEST(testfilecacheconcurrentfind);
  }
};
}  // namespace CacheTest

static void atexit_handler()
{
  for (unsigned int i = 0; i < sizeof(testpaths) / sizeof(*testpaths); i++)
  {
    if (testpaths[i] != nullptr)
    {
      std::filesystem::remove_all(*testpaths[i]);
      delete testpaths[i];
    }
  }
}

int main(void)
{
  using namespace std;
  cout << endl << "Cache" << endl << "=========" << endl;
  atexit(atexit_handler);  // Remove this if you need to debug test directory contents after test
  for (unsigned int i = 0; i < sizeof(testpaths) / sizeof(*testpaths); i++)
  {
    testpaths[i] = new std::filesystem::path(
        boost::filesystem::unique_path(std::filesystem::temp_directory_path().string() + "/" +
                                       "MacGyver_CacheTest_" + to_string(i) + "_%%%%%%%%")
            .string());
    // cout << "Testpath " << i << " is " << *testpaths[i] << endl;
  }
  CacheTest::tests t;
  return t.run();
}
