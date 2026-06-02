# smartmet-library-macgyver — Feature List

A structured inventory of capabilities provided by the macgyver
utility library. Use as a checklist when drafting release notes. When
new functionality is added, append the new entry under the matching
section (and bump the *Last updated* line at the bottom).

`smartmet-library-macgyver` is the **foundational C++ utility library**
of SmartMet Server. Nearly every other repository in the ecosystem
depends on it. All code lives under the `Fmi::` namespace with
sub-namespaces such as `Fmi::Cache`, `Fmi::Astronomy`, and
`Fmi::date_time`. The library produces both shared
(`libsmartmet-macgyver.so`) and static (`libsmartmet-macgyver.a`)
artefacts.

---

## 1. Date & time

A self-contained datetime subsystem under `macgyver/date_time/` and
top-level helpers:

- **`Fmi::date_time::Date`** — date type.
- **`Fmi::date_time::DateTime`** — naive (UTC / unzoned) date-time.
- **`Fmi::date_time::LocalDateTime`** — timezone-aware date-time.
- **`Fmi::date_time::TimeDuration`** / **`TimePeriod`** — duration
  arithmetic.
- **`Fmi::date_time::TimeZonePtr`** — timezone handle.
- **Howard Hinnant's date library** bundled under
  `macgyver/date_time/date/` — used as a fallback when
  `std::chrono` lacks full timezone support (currently always used
  because `FMI_ENABLE_STD_CHRONO_ONLY=0`).
- **`USE_OS_TZDB`** wiring — the Makefile auto-detects and compiles
  `obj/tz.o` with the right flags so the IANA tzdata is consumed
  directly from the operating system.
- **`Fmi::TimeFormatter`** — format `DateTime` / `LocalDateTime` to
  ISO / SQL / RFC / epoch / locale-specific strings.
- **`Fmi::TimeParser`** — flexible parser accepting ISO 8601,
  partial timestamps, relative (`now-12h`), `data` keyword, etc.
- **`Fmi::DateTimeParser`** — higher-level wrapper over `TimeParser`.
- **`Fmi::TimeZoneFactory`** / **`Fmi::TimeZones`** /
  **`WorldTimeZones`** — timezone lookup and listing.

## 2. Caching

- **`Fmi::Cache::Cache<K, V>`** — striped LRU cache using
  `shared_mutex` per stripe; configurable size with runtime
  `resize()` support.
- **`Fmi::Cache::FileCache`** — bimap-backed variant for file-keyed
  caches.
- **`Fmi::Cache::CacheStats`** — per-cache hit/miss/eviction counters.
- **`Fmi::LRUCache<K, V>`** — simpler header-only LRU implementation
  for in-class caches.

## 3. Exception handling

- **`Fmi::Exception`** — wraps `std::exception` with:
  - Stack traces.
  - Chained causes (`addDetail`, `addParameter`).
  - Source file / line / function via `BCP` macro
    (`__FILE__, __LINE__, __FUNCTION__`).
- **`Fmi::Exception::Trace(BCP, msg)`** — idiomatic creation used
  across the SmartMet ecosystem.

## 4. Concurrency & async

- **`Fmi::AsyncTask`** — single async task with cancellation.
- **`Fmi::AsyncTaskGroup`** — coordinated group of async tasks with
  shared cancellation token.
- **`Fmi::AtomicSharedPtr<T>`** — lock-free shared-pointer container,
  used by SmartMet engines for hot-swappable metadata snapshots.
- **`Fmi::ThreadPool`** — fixed-size worker pool.
- **`Fmi::WorkerPool`** — alternative worker abstraction.
- **`Fmi::WorkQueue`** — producer/consumer queue.
- **`Fmi::Pool`** — generic resource pool.
- **`Fmi::StaticCleanup`** — ordered global destruction helper for
  shared-library teardown.

## 5. String utilities

- **`Fmi::Base64`** — encode / decode.
- **`Fmi::CharsetConverter`** / **`Fmi::CharsetTools`** — ICU-backed
  charset conversion and Unicode normalisation.
- **`Fmi::StringConversion`** — numeric ↔ string conversions with
  `double-conversion` for fast lossless float printing.
- **`Fmi::ValueFormatter`** — numeric value formatting (precision,
  width, missing-value text, locale).
- **`Fmi::Sanitizer`** — string sanitisation helpers.
- **`Fmi::Pretty`** — human-friendly printing helpers.
- **`Fmi::Join`** — container-to-string join.
- **`Fmi::AnsiEscapeCodes`** — terminal colour / styling codes.

## 6. CSV / file utilities

- **`Fmi::CsvReader`** — streaming CSV parser.
- **`Fmi::FileSystem`** — path / directory helpers.
- **`Fmi::DirectoryMonitor`** — inotify-based directory watch (used
  for hot-reloading config / Lua / mapping files across the
  ecosystem). Demo program in `examples/monitor.cpp`.
- **`Fmi::MappedFile`** — RAII wrapper over mmap.

## 7. Astronomy

- **`Fmi::Astronomy::SolarPosition`** — sun position computation.
- **`Fmi::Astronomy::SolarTime`** — sunrise / sunset.
- **`Fmi::Astronomy::LunarPhase`** — moon phase.
- **`Fmi::Astronomy::LunarTime`** — moonrise / moonset.
- **`Fmi::Astronomy::JulianTime`** — Julian day conversions.
- **Helper functions** — `AstronomyHelperFunctions.h`.

## 8. Geometry & math

- **`Fmi::Geometry`** — basic 2-D geometry helpers.
- **`Fmi::HelmertTransformation`** — Helmert datum transformations.
- **`Fmi::ReferenceEllipsoid`** — WGS84 / GRS80 / spherical earth
  constants and conversions.
- **`Fmi::Matrix<T>`** — fixed-size matrix template.
- **`Fmi::DistanceParser`** — parse `25km` / `12mi` / `8nm` etc.
- **`Fmi::FastMath`** — fast approximations of trig / log / exp.

## 9. Spatial indexes

- **`Fmi::NearTree`** — generic nearest-neighbour search tree.
- **`Fmi::NearTreeLatLon`** — specialised great-circle variant.
- **`Fmi::LatLonTree`** — alternative lat/lon index.
- **`Fmi::TernarySearchTree`** — prefix-indexed string lookup.

## 10. Caching & lookup primitives

- **`Fmi::TypeMap`** — `std::type_info`-keyed associative container.
- **`Fmi::TypeName`** — runtime type-name extraction.
- **`Fmi::TypeTraits`** — extra type traits beyond `<type_traits>`.
- **`Fmi::FunctionMap`** — name-to-`std::function` registry.
- **`Fmi::Optional`** — lightweight nullable wrapper (pre-C++17 use,
  retained for compatibility).
- **`Fmi::NumericCast`** — checked numeric casts that throw on
  range loss.

## 11. PostgreSQL integration

- **`Fmi::PostgreSQLConnection`** — pimpl-based wrapper around
  `libpqxx`, used by the observation engine and other database-
  backed components.
- **Connection pooling** through the wrapper.
- **Tests** require a geonames database — created locally in CI or
  provided by `smartmet-test:5444` for developers.

## 12. Hashing

- **`Fmi::Hash`** — composable hash helpers (`hash_combine`,
  `hash_merge`, type-agnostic accumulators) used everywhere in the
  ecosystem for cache keys and dependency tracking.

## 13. Templates

- **`Fmi::TemplateFactory`** — per-thread CTPP2 template cache (used
  by the WMS, EDR, and cross-section plugins).
- **`Fmi::TemplateFormatter`** — convenience formatter.

## 14. Debugging

- **`Fmi::DebugTools`** — printing / breakpoint / assertion
  helpers used during development.
- **`BCP`** macro — `__FILE__, __LINE__, __FUNCTION__` triple.
- **Stack traces** are attached automatically by `Fmi::Exception`.

## 15. Examples

`examples/`:

- **`monitor.cpp`** — demonstrates `DirectoryMonitor`.
- **`trigger.cpp`** — companion that triggers monitor events.

## 16. Testing

- **~53 test executables** under `test/*Test.cpp`, each built as a
  standalone binary.
- **Framework**: FMI's own `regression/tframe.h` (from
  `smartmet-library-regression`) — *not* Boost.Test.
- **Test macros**: `TEST(name)`, `TEST_PASSED()`, `TEST_FAILED(msg)`.
- **Per-test build**: `make -C test CacheTest && ./test/CacheTest`.
- **PostgreSQL tests** require a reachable geonames database.

## 17. Build & integration

- **Library outputs**: `libsmartmet-macgyver.so` (shared) and
  `libsmartmet-macgyver.a` (static).
- **Build**: `make` (and `make` inside `test/` for tests).
- **CMake support**: a `CMakeLists.txt` is provided alongside the
  hand-written `Makefile`.
- **Install**: `make install` (default `PREFIX=/usr`).
- **RPM**: `make rpm`.
- **Format**: `make format` runs clang-format (Google style, Allman
  braces, 100-col).
- **pkg-config requirements**: `libpqxx`, `icu-i18n`, `fmt` (12.x),
  `ctpp2`, `filesystem`.
- **Boost components linked**: `regex`, `serialization`, `chrono`,
  `iostreams`, `thread`.
- **Also links**: `double-conversion`.
- **CI**: CircleCI on RHEL 8 / RHEL 10 with the
  `fmidev/smartmet-cibase-{8,10}` Docker images via the standard
  `ci-build` workflow (`deps`, `rpm`, `testprep`, `test`).
- **Public headers** installed under
  `/usr/include/smartmet/macgyver/`.

---

*Last updated: 2026-06-01.*
