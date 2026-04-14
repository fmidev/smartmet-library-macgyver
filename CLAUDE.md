# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`smartmet-library-macgyver` — the foundational C++ utility library for SmartMet Server (FMI). Everything in the SmartMet ecosystem depends on this. All code lives under the `Fmi` namespace, with sub-namespaces like `Fmi::Cache`, `Fmi::Astronomy`, `Fmi::date_time`.

## Build commands

```bash
make                  # Build libsmartmet-macgyver.so
make test             # Build and run all tests
make format           # clang-format (Google style, Allman braces, 100-col)
make clean            # Clean build artifacts and tests
make rpm              # Build RPM package
make install          # Install to system (PREFIX=/usr)
```

### Running a single test

Tests are individual executables compiled from `test/*Test.cpp`:

```bash
make -C test CacheTest && ./test/CacheTest       # Build and run one test
make -C test CacheTest HashTest                   # Build specific tests only
```

## Test framework

Tests use FMI's own `regression/tframe.h` framework (from `smartmet-library-regression`), **not** Boost.Test. Each test file defines a `tests` class inheriting from `tframe::tests`, registers test functions via `TEST(name)`, and runs via `t.run()` from `main()`. Test macros: `TEST_PASSED()`, `TEST_FAILED(msg)`.

## Source layout

- `macgyver/` — library source (`.cpp`, `.h`)
- `macgyver/date_time/` — datetime subsystem (Date, DateTime, LocalDateTime, TimeDuration, TimePeriod)
- `macgyver/date_time/date/` — Howard Hinnant's date library (fallback when `std::chrono` lacks timezone support)
- `test/` — test executables, one per `*Test.cpp`
- `examples/` — example programs (monitor, trigger)

## Key design details

**DateTime subsystem**: `macgyver/date_time/Base.h` controls whether the library uses `std::chrono` (C++20) or the bundled Date library for timezone support. Currently `FMI_ENABLE_STD_CHRONO_ONLY` is hardcoded to 0, so the Date library is always used. The Makefile auto-detects this and sets `USE_OS_TZDB` and `TZ_CFLAGS` accordingly. The `obj/tz.o` target gets special compiler flags.

**Cache**: `Fmi::Cache::Cache<K,V>` is a striped LRU cache using `shared_mutex` per stripe. `Fmi::Cache::FileCache` is a bimap-based variant. Both track hit/miss statistics via `CacheStats`.

**Exception**: `Fmi::Exception` wraps `std::exception` with stack traces and chained causes. Created via `Fmi::Exception::Trace(BCP, msg)` where `BCP` is a macro expanding to `__FILE__, __LINE__, __FUNCTION__`.

**PostgreSQL**: `PostgreSQLConnection` uses the pimpl pattern (`PostgreSQLConnectionImpl`). Tests require a geonames database — in CI this is created locally; otherwise it connects to `smartmet-test:5444`.

## Dependencies

pkg-config: `libpqxx`, `icu-i18n`, `fmt` (12.x), `ctpp2`, `filesystem`. Also links Boost (regex, serialization, chrono, iostreams, thread) and double-conversion.

## CI

CircleCI builds and tests RPMs on RHEL 8 and RHEL 10 Docker images (`fmidev/smartmet-cibase-{8,10}`). Uses `ci-build deps/rpm/testprep/test` commands.
