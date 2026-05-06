#pragma once

/**
 * @file        version.hpp
 * @brief       Version constants for ase-adp-libcuckoo
 * @description Pure-constexpr identity tag for the adapter. Matches the
 *              VERSION SSOT at the repo root and lets consumers static_assert
 *              their expected API surface.
 *
 * @module      ase-adp-libcuckoo
 * @layer       adapter
 */

#include <cstdint>

namespace ase::adp::libcuckoo {

constexpr uint8_t  VERSION_MAJOR = 0;
constexpr uint8_t  VERSION_MINOR = 0;
constexpr uint8_t  VERSION_PATCH = 1;
constexpr uint32_t VERSION_BUILD = 1;

constexpr const char* VERSION_STRING = "00.00.01.00001";
constexpr const char* VERSION_STATUS = "seed";

}  // namespace ase::adp::libcuckoo
