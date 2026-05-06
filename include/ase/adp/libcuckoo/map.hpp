#pragma once

/**
 * @file        map.hpp
 * @brief       Lock-free concurrent hash map alias for ASE
 * @description Thin re-export of libcuckoo::cuckoohash_map under the
 *              ase::adp::libcuckoo namespace. The upstream type carries
 *              the full API; this header exists so consumers can write
 *              `ase::adp::libcuckoo::Map<K, V>` and never include the
 *              raw upstream path directly. That keeps the dependency
 *              surface uniform across the codebase and gives us a
 *              single place to swap the implementation if we ever
 *              migrate (e.g. to TBB concurrent_hash_map).
 *
 *              libcuckoo's API in one paragraph:
 *                - find(key, out)        : O(1) avg, returns bool
 *                - insert(key, value)    : O(1) avg
 *                - erase(key)            : O(1) avg
 *                - update_fn(key, fn)    : compare-and-swap style update
 *                - upsert(key, fn, init) : update or insert
 *                - All operations are safe under concurrent multi-writer
 *                  / multi-reader access without a global lock.
 *
 *              See https://github.com/efficient/libcuckoo for the full
 *              API surface and CMU's cuckoo-hashing paper for the
 *              underlying algorithm.
 *
 * @module      ase-adp-libcuckoo
 * @layer       adapter (third-party isolation)
 */

#include <libcuckoo/cuckoohash_map.hh>

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace ase::adp::libcuckoo {

/**
 * Concurrent O(1)-avg hash map. Direct alias of CMU's cuckoohash_map.
 *
 * Template parameters mirror the upstream type:
 *   Key       — key type, must be hashable + equality-comparable
 *   T         — value type
 *   Hash      — hasher, defaults to std::hash<Key>
 *   KeyEqual  — equality, defaults to std::equal_to<Key>
 *   Allocator — allocator, defaults to std::allocator<std::pair<const Key, T>>
 *   SLOT_PER_BUCKET — bucket capacity, defaults to upstream default (4)
 *
 * The alias does not introduce a new type — `Map<K,V>` and
 * `::libcuckoo::cuckoohash_map<K,V>` are the same type.
 */
template <
    typename Key,
    typename T,
    typename Hash      = std::hash<Key>,
    typename KeyEqual  = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>,
    std::size_t SLOT_PER_BUCKET = ::libcuckoo::DEFAULT_SLOT_PER_BUCKET>
using Map = ::libcuckoo::cuckoohash_map<Key, T, Hash, KeyEqual, Allocator, SLOT_PER_BUCKET>;

}  // namespace ase::adp::libcuckoo
