# macgyver developer guide

This guide is for developers who change `smartmet-library-macgyver`, or use it in other
SmartMet code. macgyver is the bottom of the dependency tree: every other SmartMet library,
engine and plugin links against it, so a change here reaches the whole server.

[FEATURES.md](../FEATURES.md) lists every component; [CLAUDE.md](../CLAUDE.md) has the
build summary. This guide covers the components where how they behave matters more than
what they are: exceptions, date and time, caches, concurrency, pools, directory
monitoring, PostgreSQL, and the string and hash helpers.

## Contents

1. [Building and testing](#1-building-and-testing)
2. [Exceptions](#2-exceptions)
3. [Date and time](#3-date-and-time)
4. [Parsing times](#4-parsing-times)
5. [Caches](#5-caches)
6. [Concurrency](#6-concurrency)
7. [Object pools](#7-object-pools)
8. [Files and directories](#8-files-and-directories)
9. [PostgreSQL](#9-postgresql)
10. [Strings, numbers and hashes](#10-strings-numbers-and-hashes)
11. [Other components](#11-other-components)
12. [Compatibility](#12-compatibility)
13. [Known pitfalls](#13-known-pitfalls)

---

## 1. Building and testing

```bash
make                                   # libsmartmet-macgyver.so and .a
make test                              # every test/*Test.cpp
make -C test CacheTest && ./test/CacheTest
```

The tests use `regression/tframe.h` from `smartmet-library-regression`, not Boost.Test.
`PostgreSQLConnectionTest` needs a geonames database: with `CI` set, the test Makefile
creates a local one with `/usr/share/smartmet/test/db/create-local-db.sh`; otherwise it
connects to `smartmet-test`.

## 2. Exceptions

`Fmi::Exception` is the exception type used throughout SmartMet. It records the file,
line and function (`BCP` expands to `__FILE__, __LINE__, __PRETTY_FUNCTION__`), a message,
details, named parameters, and a link to the exception it wraps.

There are two ways to create one, and they are different:

| Call | Chains the exception being handled? |
|------|-------------------------------------|
| `Fmi::Exception(BCP, "message")` | **No.** It starts a new chain. |
| `Fmi::Exception::Trace(BCP, "message")` | **Yes.** Inside a `catch`, it wraps the current exception. A `std::exception` becomes an `Fmi::Exception` with the message `[type] what()`. |

The usual pattern is

```cpp
try
{
  ...
  throw Fmi::Exception(BCP, "Unknown producer").addParameter("producer", name);
}
catch (...)
{
  throw Fmi::Exception::Trace(BCP, "Operation failed!");
}
```

which gives a stack of locations when printed. Other things to know:

* `what()` returns the message of the **deepest** exception in the chain, not the
  outermost one.
* `SquashTrace()` returns a copy of the deepest `Fmi::Exception` only.
* `Trace()`, the constructor chaining and `Fmi::ignore_exceptions()` rethrow
  `boost::thread_interrupted`, so that `AsyncTask::cancel()` keeps working (§6). An empty
  `catch (...) {}` in a task swallows the interruption; call `Fmi::ignore_exceptions()`
  in it instead.
* `disableLogging()` and `disableStackTrace()` control printing; the flags propagate
  from a wrapped exception to the wrapper. `Exception::ForceStackTrace` (a scope guard)
  forces full traces for the current thread.
* `printError()` writes the trace to `std::cerr`; `operator<<` writes it to a stream.

## 3. Date and time

The time types are macgyver's own (`macgyver/date_time/`), built on `std::chrono` and
Howard Hinnant's date library, which is bundled under `date_time/date/`. They replaced
`boost::posix_time`, and keep a similar interface.

| Type | Meaning |
|------|---------|
| `Fmi::DateTime` | A time without a time zone, microsecond resolution. Can also be `NOT_A_DATE_TIME`, `POS_INFINITY` or `NEG_INFINITY`. By convention, it holds UTC. |
| `Fmi::Date`, `Fmi::TimeDuration` | Calendar date and duration. `Fmi::Seconds(n)`, `Minutes`, `Hours`, `Days` build durations. |
| `Fmi::LocalDateTime` | A `DateTime` with a `TimeZonePtr`. `utc_time()` and `local_time()` give the two views. |
| `Fmi::TimePeriod`, `Fmi::LocalTimePeriod`, `Fmi::DatePeriod` | Intervals. |
| `Fmi::SecondClock`, `Fmi::MicrosecClock` | `universal_time()` / `local_time()`. |

`FMI_ENABLE_STD_CHRONO_ONLY` in `date_time/Base.h` is 0, so the date library is always
used, even with C++20. With `USE_OS_TZDB=1` it reads the time zone rules from the
operating system's tzdata (`/usr/share/zoneinfo`), and `tz.cpp` is compiled with special
flags (`obj/tz.o` in the Makefile). Updating the system tzdata package updates the rules.

**Time zones.** `Fmi::TimeZoneFactory::instance()` is the process-wide factory:

* `time_zone_from_string("Europe/Helsinki")` looks the name up in the tzdata. POSIX TZ
  strings such as `EET-2EEST` are not supported.
* `time_zone_from_coordinate(lon, lat)` uses `/usr/share/smartmet/timezones/timezone.shz`
  (`WorldTimeZones`) to find the zone name.
* `set_region_file()` and `set_coordinate_file()` are deprecated and only print a
  warning.

Formatting is done with `Fmi::TimeFormatter` (a factory of named formats: `iso`, `sql`,
`xml`, `epoch`, `timestamp`, `http`, …) and the `Fmi::to_iso_string()` family in
`StringConversion.h`.

## 4. Parsing times

`Fmi::DateTimeParser` is the parser to use. It holds the compiled Boost.Spirit grammars,
so create it once and reuse it: it is copyable, and `parse()` is `const`. The older free
functions in `Fmi::TimeParser` are deprecated, but they are still used, and they offer
`looks()` and `looks_utc()`, which `DateTimeParser` does not.

`parse(str)` tries the formats in this order:

| Format | Examples | Result |
|--------|----------|--------|
| ISO 8601 | `2026-09-25T12:00:00Z`, `2026-09-25T12:00:00+03:00`, `20260925T1200` | UTC; an offset is applied. |
| FMI timestamp | `202609251200` | as written |
| ISO 8601 basic date | `20260925` | midnight of that date (an eight-digit string that is not a valid date is epoch seconds) |
| SQL | `2026-09-25 12:00:00` | as written |
| epoch | `1790000000` | UTC |
| offset | `0`, `+1h`, `-30m`, `+2d`, `PT6H` | now + offset, UTC, rounded down to the minute |

`parse(str, format)` forces one of `iso` (also `xml`, `timestamp`), `sql`, `epoch`,
`offset`. Offsets and FMI durations **need a sign** (`+30m`, not `30m`), except a plain
`0`. The units are `s`, `m`, `h`, `d`, `w` and `y`, and a number without a unit is minutes.
`parse_duration()` accepts FMI durations (`+30m`, `-6h`) and ISO 8601 durations
(`PT6H`); `parse_iso_duration()` accepts only the latter.

**Local times.** The overloads that take a `TimeZonePtr` return a `LocalDateTime`.
Epochs, relative offsets and ISO times with a `Z` or an explicit offset are UTC
instants; only times without zone information are taken as wall-clock time in that
zone. `Fmi::TimeParser::looks_utc(str)` tells the two cases apart.

## 5. Caches

### 5.1 `Fmi::Cache::Cache<Key, Value, SizeFunc, NumShards = 16>`

A thread-safe cache with CLOCK eviction, split into `NumShards` shards. Each shard has
its own `std::shared_mutex`, a ring of entries with a "hand", and a hash map.

* The maximum size is divided evenly between the shards
  (`ceil(maxSize / NumShards)` each), and each shard evicts on its own. With few, large
  items, the cache holds less than `maxSize`, and an item larger than one shard's share
  is **never** stored (`insert()` returns false).
* `SizeFunc::getSize(value)` gives an item's size; the default counts every item as 1.
  Use a size function for byte-limited caches.
* `insert()` does not replace an existing key; it returns false. `upsert()` replaces it.
* `find()` returns `std::optional<Value>`: a **copy** of the value. Store
  `std::shared_ptr`s for anything that is not small.
* `find()` takes only the shared lock and increments the entry's hit counter. On
  eviction the hand gives entries hit since its last pass a second chance.
* `insert()` and `upsert()` also take the value as an rvalue, which is moved into the
  cache.
* `statistics()` returns `CacheStats` (hits, misses, inserts, evictions, size), which
  engines and plugins report through spine's `getCacheStats()`.
* `resize()` changes the limit at runtime and evicts at once.
* The default constructor makes a cache of size **0**, which stores nothing until
  `resize()` is called.

### 5.2 Other caches

* `Fmi::LRUCache<T>`: a thin wrapper (`put` = `upsert`, `get` = `find`) over `Cache`
  with `size_t` keys and `shared_ptr<T>` values, kept for old call sites.
* `Fmi::Cache::FileCache`: a disk cache of strings keyed by a hash, with a byte limit.
  It is the second level of spine's `FileCache` / the frontend response cache.

## 6. Concurrency

### 6.1 `Fmi::AsyncTask`

```cpp
Fmi::AsyncTask task("reload", [&] { reload(); }, [&] { notify_done(); });
task.wait();        // joins; rethrows the task's exception
```

* The task starts at once, in its own `boost::thread`, named after the task (Linux
  truncates thread names to 15 characters).
* `cancel()` calls `boost::thread::interrupt()`. The task stops only at a Boost
  **interruption point**: `boost::this_thread::sleep_for`, waits on `boost::`
  condition variables, or `Fmi::AsyncTask::interruption_point()`. `std::this_thread::sleep_for`
  and `std::condition_variable` are **not** interruption points, so a loop that uses them
  cannot be cancelled.
* The destructor cancels and joins, and prints (unless `AsyncTask::silent`) the
  exception, if the task failed.
* The notify callback runs in the task's thread.
* `get_status()`: `none`, `active`, `ok`, `failed`, `interrupted`.

### 6.2 `Fmi::AsyncTaskGroup`

Runs many tasks, at most `max_paralell_tasks` (default 30) at a time: `add()` blocks
while the limit is reached. `wait()` waits for all of them and rethrows the first
exception; `stop()` cancels the running ones, and later `add()` calls are ignored.
`stop_on_error(true)` stops the group at the first failure. The last 100 exceptions are
kept (`get_exception_info()`), and signals `on_task_ended` / `on_task_error` report each
task. Spine initialises engines and plugins with it.

### 6.3 Other primitives

* `Fmi::AtomicSharedPtr<T>`: `load()`, `store()`, `exchange()` of a `shared_ptr` with
  `std::atomic_load` / `std::atomic_store`. Engines use it to publish immutable
  snapshots (a new repository, station list, …) that readers pick up without locking.
* `Fmi::ThreadPool::ThreadPool<Scheduler>`: a fixed-size pool with a bounded FIFO
  queue; the server's request pools are built on it.
* `Fmi::WorkQueue<T>`: a queue consumed by a fixed number of threads that all run the
  same function.

## 7. Object pools

`Fmi::Pool<PoolInitType, Item, Args...>` is a thread-safe pool of expensive objects,
typically database connections (the observation engine uses it).

```cpp
Fmi::Pool<Fmi::PoolInitType::Parallel, Connection, Options> pool(start, max, options);
auto conn = pool.get();                           // blocks until an item is free
auto conn = pool.get(Fmi::Seconds(5));            // or throws after the timeout
```

* `start_size` items are created at construction (in parallel with `Parallel`, using an
  `AsyncTaskGroup`); the pool grows on demand up to `max_size`, creating new items
  outside the lock. `start_size` is at least 2.
* Items are created with `Item(args...)` or a factory callback. The arguments are copied
  into the pool. If an argument is a reference or pointer, what it refers to must live
  as long as the pool, because new items can be created later.
* The arguments are passed in their original order, so several arguments may have
  the same type.
* `get()` returns a `Pool::Ptr`, a `unique_ptr` that returns the item to the pool when it
  is destroyed. The pool's state is shared with the outstanding `Ptr`s, so an item stays
  valid even if the pool is destroyed first.

`Fmi::WorkerPool<T>` is an older pool of default-constructed objects, still used by the
geonames engine and the WFS plugin. Prefer `Pool` in new code.

## 8. Files and directories

### 8.1 `Fmi::DirectoryMonitor`

Watches files by **polling** the modification times: it does not use inotify.

```cpp
Fmi::DirectoryMonitor monitor;
monitor.watch(dir, boost::regex(R"(.*\.sqd$)"), on_change, on_error, 10 /*s*/,
              Fmi::DirectoryMonitor::CREATE | Fmi::DirectoryMonitor::DELETE |
                  Fmi::DirectoryMonitor::MODIFY);
std::thread t([&] { monitor.run(); });   // run() loops until stop()
monitor.wait_until_ready();              // after the first scan of every watcher
```

* The listener gets a map from each changed path to its `Change` bits (`CREATE`,
  `DELETE`, `MODIFY`, `SCAN`, `ERROR`). On the first scan every existing file is
  reported as `CREATE`.
* If `MODIFY` is not in the mask, a scan is skipped when the directory's own
  modification time has not changed. Directory times have one-second resolution, and
  rewriting a file in place does not change them, so such changes can go unnoticed. Add
  `MODIFY` to the mask when files can be rewritten, or when one directory holds several
  data sets.
* `run()` blocks the calling thread; `stop()` ends it. The querydata engine, textgen and
  the WFS plugin use it.

### 8.2 `Fmi::MappedFile`

A `boost::iostreams::mapped_file` wrapper that marks the mapping `MADV_DONTDUMP`, so
mapped data files are left out of core dumps. Mind the modes: the constructor defaults
to read-only (`std::ios_base::in`), but `open()` defaults to `in | out` (read-write).

A mapped file must not be truncated or rewritten while it is mapped: accessing the lost
pages raises `SIGBUS`. Replace data files by writing a new file and renaming it.

### 8.3 Others

`Fmi::FileSystem` (`last_write_time()` variants that throw, set an error code or return
a default; `unique_path()`; `lookup_file()`; compressed stream helpers), `Fmi::CsvReader`, and `Fmi::TemplateFactory`, which caches compiled
CTPP2 templates **per thread** and reloads a template when its modification time
changes. `get()` returns a `TemplateFormatter` for the current thread; do not share it
between threads.

## 9. PostgreSQL

`Fmi::Database::PostgreSQLConnection` wraps a `pqxx::connection`.

* **Options** (`PostgreSQLConnectionOptions`): host, port, database, username,
  password, encoding, connect timeout, slow-query logging; or a connection string.
  `toString()` hides the password by default.
* **Statements.** `execute()` runs in the current transaction if one is active, and
  otherwise without one. `executeNonTransaction()` never uses a transaction. For
  explicit transactions, use `transaction()` (a `Transaction` object with `execute()`,
  `commit()` and `rollback()`; it rolls back if not committed).
* **Parameters.** Use `exec_params(sql, args...)` or prepared statements
  (`prepare()` / `exec_prepared()`) for values; `quote()` exists for building SQL text.
* **Reconnects.** When the connection breaks during a statement, the call waits (10 s
  at first, growing), reconnects and retries the statement, up to
  `queryRetryLimit` (2) more times. A request thread can therefore block for tens of
  seconds while the database is down. `PostgreSQLConnection::disableReconnect()` turns
  this off; `shutdownAll()` makes pending waits stop at shutdown.
* `cancel()` cancels the running statement from another thread.

## 10. Strings, numbers and hashes

* **Number to string.** `Fmi::to_string(int/long/…)` is exact. `Fmi::to_string(double)`
  uses `%g`, which gives **six significant digits** (`123456789.0` becomes
  `1.23457e+08`). Use `fmt::format("{}", x)` for the shortest exact representation, or the
  `to_string(fmt, value)` overloads for a fixed format.
* **String to number.** `Fmi::stoi`, `stol`, `stoul`, `stof`, `stod` require the whole
  string to be a number, and throw otherwise; the `_opt` variants return
  `std::nullopt`, also for values out of the target type's range. Infinities and NaNs
  throw in `stof_opt` and `stod_opt`.
* **`Fmi::numeric_cast<T>()`** throws on overflow or loss of range.
* **Hashes.** `Fmi::hash_value()` covers the built-in types, strings and the time types,
  and `Fmi::hash_combine(seed, value)` merges them. They are the basis of ETags and cache
  keys everywhere. `Fmi::bad_hash` is a sentinel meaning "not cacheable": combining
  anything with it gives `bad_hash` again, so one uncacheable input makes the whole key
  uncacheable. Strings hash with `std::hash`, which is stable only within one build of the
  standard library, so do not persist hashes as long-term keys across builds.
* **Charset.** `Fmi::utf8_to_latin1()`, `latin1_to_utf8()`, `is_utf8()`, the UTF-16
  converters and Nordic-aware `tolower`/`toupper` (`CharsetTools.h`); ICU-based
  conversion in `CharsetConverter`.

## 11. Other components

| Component | Use |
|-----------|-----|
| `Astronomy` | Sun and moon position, rise and set times, day length, lunar phase. Used by the timeseries library for the astronomical parameters. |
| `NearTree`, `NearTreeLatLon` | Nearest-neighbour search (stations, locations). |
| `TernarySearchTree` | Prefix lookup (older autocomplete code). |
| `DistanceParser` | `25km`, `12mi`, `8nmi`, `300m`, … to kilometres (a number without a unit is kilometres). |
| `Base64`, `Join`, `Pretty`, `AnsiEscapeCodes` | String helpers. |
| `TypeName`, `TypeMap`, `FunctionMap` | Demangled type names; type-keyed maps; name-to-function maps. |
| `StaticCleanup` + `StaticCleanup::AtExit` | Runs registered cleanups when a local `AtExit` in `main()` is destroyed, before static destruction. Used to release GDAL/PROJ-backed caches in the right order. |
| `ThreadName` | `Fmi::set_thread_name()` (see [thread-names.md](thread-names.md)). |

## 12. Compatibility

Everything in SmartMet links against macgyver, and many classes are templates or inline
in the headers (`Cache`, `Pool`, `AtomicSharedPtr`, `MappedFile`, the time types). A change
to a class layout, an inline function or a template changes the code compiled into every
dependant. Bump the spec version, and when dependants need the change, raise their
`BuildRequires`/`Requires` floors. When in doubt, rebuild and release the dependants
together.

## 13. Known pitfalls

* **`Fmi::Exception(BCP, …)` does not chain.** Inside a `catch`, use `Trace()`, or the
  original error is lost.
* **Swallowing `boost::thread_interrupted` makes tasks uncancellable.** Use
  `Fmi::ignore_exceptions()` instead of an empty `catch (...)`.
* **`std::` sleeps and condition variables are not interruption points** for
  `AsyncTask::cancel()`.
* **Unsigned offsets are errors.** `30m` is neither a time nor a duration; write `+30m`.
* **A default-constructed `Cache` has size 0** and stores nothing.
* **Items larger than one shard's share are never cached**, and a cache of few large
  items holds less than its nominal size (§5.1).
* **`Cache::find()` copies the value.** Store `shared_ptr`s.
* **`DirectoryMonitor` without `MODIFY` misses in-place rewrites** (§8.1).
* **`MappedFile::open()` defaults to read-write**, unlike the constructor.
* **`Fmi::to_string(double)` rounds to six significant digits.**
* **Database calls can block during outages** because of the reconnect waits (§9).
* **`std::regex` is not used in SmartMet.** Use `boost::regex`, as `DirectoryMonitor`
  does.
