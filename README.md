# ase-adp-libcuckoo

[![Layer](https://img.shields.io/badge/Layer-Adapter-orange.svg)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![Kind](https://img.shields.io/badge/Kind-Header--only%20vendor-yellow.svg)]()
[![Status](https://img.shields.io/badge/Status-seed-lightgrey.svg)]()

> Header-only ASE adapter that re-exports CMU's [libcuckoo](https://github.com/efficient/libcuckoo) `cuckoohash_map` as `ase::adp::libcuckoo::Map<K, V>` — a lock-free concurrent hash map with O(1)-avg insert / find / erase and per-bucket fine-grained locks.

Part of [ASE — Antares Simulation Engine](https://github.com/antarien).

---

## Why this adapter exists

ASE's git-status scanner (`clients/ase-client-explorer`) needs to track the working-tree state of **~145 nested submodules** in real time, with per-file dirty markers updated as each scan completes. The naive design — one big snapshot map under a `std::shared_ptr` swapped atomically when a scan finishes — forces an **O(M) full-map rebuild** on every scan event, where M is the total number of dirty files across all repos. With 145 submodules each running independent scans, that is the wrong shape.

What is needed:

| Property | Requirement |
|---|---|
| Reader path (UI bind callback) | O(1) lookup, **no locks** at all |
| Writer path (scanner threads) | O(1) point-update, **no global mutex** between writers |
| Update granularity | per-key, not per-snapshot |
| Multi-writer concurrency | safe with N hardware threads writing different keys at once |

This is precisely what cuckoo hashing with fine-grained per-bucket locks gives. CMU's `libcuckoo` (PI: Andrew Pavlo et al.) is the canonical implementation and is used in production by Memcached and others. It is **header-only**, MIT/Apache-licensed, and ~3 k LoC — no runtime, no allocator surprises.

## What this adapter does

- Pulls the upstream `efficient/libcuckoo` headers in via CMake `FetchContent` so consumers do **not** need a system-installed libcuckoo. No vendoring of upstream sources into this repo.
- Exposes a single namespace alias `ase::adp::libcuckoo::Map<K, V>` that resolves to `::libcuckoo::cuckoohash_map<K, V>`. Same type, ASE-native include path.
- Ships a tiny multi-writer self-test (`example/example_main.cpp`) so the build pipeline is verifiable end-to-end without pulling in an ASE consumer.

## Public API

```cpp
#include <ase/adp/libcuckoo/map.hpp>

ase::adp::libcuckoo::Map<std::string, int> map;

// O(1) insert, multi-writer safe
map.insert("foo", 42);

// O(1) read, no lock on this path
int out = 0;
if (map.find("foo", out)) {
    // out == 42
}

// Compare-and-swap style update
map.update_fn("foo", [](int& v) { v += 1; });

// Insert if missing, update if present
map.upsert("bar", [](int& v) { v += 1; }, /*initial*/ 1);
```

The full upstream API is available — `cuckoohash_map.hh` ships every public method (`erase`, `clear`, `size`, `empty`, locked-table iteration, etc.). The alias is just a re-export, no surface trimming.

## Architecture & layer

`adapter/` is a top-level isolation layer that sits orthogonal to the `L0..L5` ECS stack. This adapter is consumed by `L1+` code that needs a concurrent O(1) map — primarily the explorer's git-status scanner, but the type is fully general-purpose. There is no validator whitelist needed: libcuckoo is pure C++, header-only, no `extern "C"`, no inheritance the ASE validator would flag.

## Build

Standalone (also runs the example):

```bash
cd adapter/ase-adp-libcuckoo
cmake -B build -G Ninja
ninja -C build
./build/ase-adp-libcuckoo-example
```

As a transitive dep from the ASE root build: linked automatically when a consumer adds `ase::adp::libcuckoo` to its `target_link_libraries`. The first configure of the root project will FetchContent the upstream headers once into the shared `_deps` cache.

## Adding more concurrent-map shapes

If a future workload needs a different concurrency profile (e.g. striped reader-preferred, or a stripe-sized multi-shard variant), add a sibling adapter under `adapter/` rather than extending this one. Each adapter pins its own upstream library and keeps its own ABI. This adapter intentionally does **not** abstract away the underlying cuckoo-hashing semantics behind a generic `ConcurrentMap` interface — that abstraction would force the slowest common denominator and erase the very property we are buying libcuckoo for.

## License

Source code in this adapter (the wrapper headers + CMake glue + example) is released under the AOW Developer License — see [`LIC_AOW_ADG_DE.md`](LIC_AOW_ADG_DE.md). The fetched upstream `libcuckoo` headers remain under their own terms (Apache-2.0 + MIT), as carried in the upstream tree under `_deps/libcuckoo_upstream-src/LICENSE`.

Project participation is governed by the AOW ADG project agreements — see [`PJV_AOW_ADG_DE.md`](PJV_AOW_ADG_DE.md) (DE), [`PJV_AOW_ADG_EN.md`](PJV_AOW_ADG_EN.md) (EN), [`PJV_AOW_ADG_PT.md`](PJV_AOW_ADG_PT.md) (PT).
