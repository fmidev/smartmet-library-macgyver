# SmartMet thread names

This is the authoritative registry of the OS-level thread names used across the
SmartMet ecosystem. The names show up in `top`/`htop` (`H` to show threads, or
`top -H`), in `ps -L`, and in `gdb` (`info threads`), and make it possible to
tell at a glance which subsystem a thread belongs to.

This table is kept in `macgyver` because macgyver owns the naming mechanism and
every other repository depends on it. Other modules should **not** duplicate
their thread lists — add new threads here instead.

## Mechanism

Set a thread's name from inside the thread (or from a thread entry function)
with the helper in [`ThreadName.h`](../macgyver/ThreadName.h):

```cpp
#include <macgyver/ThreadName.h>
Fmi::set_thread_name("upd-flash");
```

`Fmi::ThreadPool` can name its workers automatically — construct it with a name
and the workers become `<name>-1 … <name>-N`:

```cpp
Fmi::ThreadPool::ThreadPool<> pool(n, schedulerSize, "trax");  // -> trax-1 … trax-N
```

`Fmi::AsyncTask` and `Fmi::AsyncTaskGroup` use the task name as the thread name,
so keep task names within the limit too.

## Naming scheme

Linux caps a thread name at **16 bytes including the terminating NUL, i.e. 15
visible characters**; longer names are truncated. `Fmi::set_thread_name` warns
(once per distinct name) when a name is too long. Keep names short, lowercase,
ASCII, and built from these functional prefixes:

| Prefix | Meaning |
|---|---|
| `ini-` | one-shot startup / initialization |
| `upd-` | recurring update loop or watcher |
| `ld-p-` / `ld-e-` | plugin / engine load (lifecycle) |
| `sd-` , `sd-p-` / `sd-e-` | shutdown (watcher, plugin, engine) |
| `<sub>-…` | subsystem-specific singleton (e.g. `geo-io`, `cs-maint`) |
| `<prefix>-N` | worker in a pool (monotonic index) |

Below, literal names are fixed, `N` is a monotonic worker index, and `<x>` is a
value substituted at runtime.

## Pools (indexed `<prefix>-N`)

| Name | Module | Purpose |
|---|---|---|
| `srv-wrk-NNNN` | server | ASIO network worker pool |
| `trax-N` | trax | contour band-parallel workers |
| `front-be-N` | plugins/frontend | backend ASIO IO pool |
| `mmap-flt-N` | grid-files | memory-mapper fault processing pool |
| `qd-fillgrid-N` | newbase | parallel grid fill (multi/single time) |
| `grib2qd-N` | qdtools | GRIB→querydata conversion workers |
| `calctopo-N` | fmitools | coastal-distance workers |
| `sounding-N` | smarttools | sounding-index workers |

## `ini-` — startup / init

| Name | Module | Purpose |
|---|---|---|
| `ini-reactor` | server | `Reactor::init` |
| `ini-dem` | engines/geonames | DEM init |
| `ini-landcover` | engines/geonames | land-cover init |
| `ini-suggest` | engines/geonames | autocomplete index init |
| `ini-stations` | engines/observation | initial station load |
| `ini-obscache` | engines/observation | observation cache init |
| `ini-wdqc` | engines/observation | weather-data-QC cache init |
| `ini-flash` | engines/observation | flash cache init |
| `ini-netatmo` | engines/observation | NetAtmo cache init |
| `ini-roadcloud` | engines/observation | road-cloud cache init |
| `ini-fmi_iot` | engines/observation | fmi_iot cache init |
| `ini-tapsi_qc` | engines/observation | tapsi_qc cache init |
| `ini-magnetom` | engines/observation | magnetometer cache init |
| `ini-oracle` | engines/observation | Oracle driver init + fmi_iot fetch |
| `ini-drv-<name>` | engines/observation | per-DB-driver init (dynamic) |

## `upd-` — recurring loops / watchers

| Name | Module | Purpose |
|---|---|---|
| `upd-qd-mon` | engines/querydata | directory monitor |
| `upd-qd-exp` | engines/querydata | data expiration loop |
| `upd-qd-cfg` | engines/querydata | config-file watch |
| `upd-qd` | engines/querydata | producer data (re)load |
| `upd-grid` | engines/grid | engine update thread |
| `upd-auth` | engines/authentication | mapping refresh |
| `upd-edr` | plugins/edr | update loop |
| `upd-edr-meta` | plugins/edr | metadata update loop |
| `upd-wfs` | plugins/wfs | update loop |
| `upd-wfs-sq` | plugins/wfs | stored-query directory monitor |
| `upd-tgen-cfg` | plugins/textgen | config update watch |
| `upd-wms-caps` | plugins/wms | capabilities update loop |
| `upd-logclean` | spine | access-log cleaner |
| `upd-stations` | engines/observation | station cache loop / runtime reload |
| `upd-obscache` | engines/observation | observation cache update loop |
| `upd-wdqc` | engines/observation | weather-data-QC cache update loop |
| `upd-flash` | engines/observation | flash cache update loop |
| `upd-netatmo` | engines/observation | NetAtmo cache update loop |
| `upd-roadcloud` | engines/observation | road-cloud cache update loop |
| `upd-fmi_iot` | engines/observation | fmi_iot cache update loop |
| `upd-tapsi_qc` | engines/observation | tapsi_qc cache update loop |
| `upd-magnetom` | engines/observation | magnetometer cache update loop |
| `upd-qs` | grid-content | queryServer update thread |
| `upd-cqs` | tools-grid | corbaQueryServer update thread |
| `upd-cgs` | tools-grid | corbaGridServer update thread |

## `ld-` / `sd-` — plugin & engine lifecycle (spine, dynamic)

| Name | Module | Purpose |
|---|---|---|
| `ld-p-<plugin>` | spine | plugin load / init |
| `ld-e-<engine>` | spine | engine instance load / init |
| `sd-p-<plugin>` | spine | plugin shutdown |
| `sd-e-<engine>` | spine | engine shutdown |
| `sd-watch` | spine | shutdown-watch thread |

`<plugin>`/`<engine>` are the configured names, lowercased. To stay within 15
characters, long names are shortened via a fixed tag map:

| Full name | Tag |
|---|---|
| `observation` | `obs` |
| `authentication` | `auth` |
| `timeseries` | `tseries` |
| `autocomplete` | `acomp` |
| `cross_section` | `xsect` |

All other names are used verbatim (lowercased). Keep this map in sync with
`short_tag()` in `spine/spine/Reactor.cpp`.

## Per-subsystem singletons

| Name | Module | Purpose |
|---|---|---|
| `srv-run` | server | AsyncServer run task |
| `geo-io` | engines/geonames | ASIO IO service |
| `sputnik-io` | engines/sputnik | ASIO IO service |
| `q3-health` | plugins/q3 | health-check thread |
| `q3-poll` | plugins/q3 | tracker polling thread |
| `cs-maint` | grid-content | ContentServer corba maintenance |
| `cs-cache-evt` | grid-content | ContentServer cache event processing |
| `cs-mem-sync` | grid-content | ContentServer memory sync processing |
| `cs-merge-evt` | grid-content | ContentServer merge event processing |
| `ds-maint` | grid-content | DataServer corba maintenance |
| `ds-event` | grid-content | DataServer event processing |
| `ds-cache` | grid-content | DataServer cache processing |
| `qs-maint` | grid-content | QueryServer corba Server maintenance |
| `qs-gmaint` | grid-content | QueryServer corba GridServer maintenance |
| `mmap-fault` | grid-files | memory-mapper fault handler |
| `smt-gridblk` | smarttools | grid calculation block worker |
| `smt-specblk` | smarttools | special-type calculation block worker |
| `smt-grid` | smarttools | grid calculation worker |
| `smt-spec` | smarttools | special-type calculation worker |

## Adding a new thread

1. Pick a name that fits the scheme above and is **≤ 15 characters**.
2. Call `Fmi::set_thread_name(...)` at the top of the thread function (or pass a
   name to `Fmi::ThreadPool` / `Fmi::AsyncTask`).
3. Add a row to the appropriate table here.

The calling thread is never renamed implicitly — name only the threads you spawn.
