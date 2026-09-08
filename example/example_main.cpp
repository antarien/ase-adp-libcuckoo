/**
 * @file        example_main.cpp
 * @brief       Tiny self-test of the ase-adp-libcuckoo adapter
 * @description Spawns a handful of writer threads that all hammer the same
 *              concurrent map at once, then verifies from the main thread
 *              that every expected key landed. This is not a benchmark — it
 *              exists so we have one binary that proves the FetchContent
 *              pipeline + the namespace alias all hang together.
 *
 *              Output goes through ase-log, not printf: a direct write to
 *              stdout bypasses the category system, shows up in no --log
 *              selection and lands in no tier log file. Both failure paths
 *              carry ERR::CAT::VALUE_INVALID — the value is present and
 *              breaks the contract, and the run ends right there (return 1),
 *              which is what puts them on the error level rather than the
 *              warning level.
 *
 *              Build standalone:
 *                cmake -B build && cmake --build build
 *                ./build/ase-adp-libcuckoo-example
 *
 * @module      ase-adp-libcuckoo
 * @layer       adapter
 */

#include <ase/adp/libcuckoo/map.hpp>
#include <ase/adp/libcuckoo/version.hpp>

#include <ase/log/log.hpp>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads      = 8;
constexpr int kPerThread    = 10'000;
constexpr int kExpectedSize = kThreads * kPerThread;

}  // namespace

int main() {
    ase::log::info("[AdpLibcuckooExample] ase-adp-libcuckoo v{} [{}]",
                   ase::adp::libcuckoo::VERSION_STRING,
                   ase::adp::libcuckoo::VERSION_STATUS);

    ase::adp::libcuckoo::Map<std::string, int> map;

    std::atomic<int> conflicts{0};

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                std::string key = "t" + std::to_string(t) + "_k" + std::to_string(i);
                if (!map.insert(key, i)) {
                    conflicts.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& w : workers) w.join();

    const auto size = map.size();
    ase::log::info("[AdpLibcuckooExample] inserted: {} / expected: {} (conflicts: {})",
                   size, kExpectedSize, conflicts.load());

    if (size != static_cast<size_t>(kExpectedSize)) {
        ase::log::error(ase::log::ERR::CAT::VALUE_INVALID,
                        "AdpLibcuckooExample", "map_size",
                        static_cast<float>(size),
                        static_cast<float>(kExpectedSize));
        return 1;
    }

    int sample = -1;
    if (!map.find(std::string("t0_k42"), sample) || sample != 42) {
        ase::log::error(ase::log::ERR::CAT::VALUE_INVALID,
                        "AdpLibcuckooExample", "lookup_t0_k42",
                        static_cast<float>(sample));
        return 1;
    }
    ase::log::info("[AdpLibcuckooExample] OK");
    return 0;
}
