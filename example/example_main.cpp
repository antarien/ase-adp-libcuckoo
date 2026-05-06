/**
 * @file        example_main.cpp
 * @brief       Tiny self-test of the ase-adp-libcuckoo adapter
 * @description Spawns a handful of writer threads that all hammer the same
 *              concurrent map at once, then verifies from the main thread
 *              that every expected key landed. This is not a benchmark — it
 *              exists so we have one binary that proves the FetchContent
 *              pipeline + the namespace alias all hang together.
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

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads      = 8;
constexpr int kPerThread    = 10'000;
constexpr int kExpectedSize = kThreads * kPerThread;

}  // namespace

int main() {
    std::printf("ase-adp-libcuckoo v%s [%s]\n",
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
    std::printf("inserted: %zu / expected: %d (conflicts: %d)\n",
                size, kExpectedSize, conflicts.load());

    if (size != static_cast<size_t>(kExpectedSize)) {
        std::printf("FAIL: size mismatch\n");
        return 1;
    }

    int sample = -1;
    if (!map.find(std::string("t0_k42"), sample) || sample != 42) {
        std::printf("FAIL: lookup mismatch (got %d)\n", sample);
        return 1;
    }
    std::printf("OK\n");
    return 0;
}
