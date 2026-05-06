cmake_minimum_required(VERSION 3.22)

#[[
ASE LIBCUCKOO ADAPTER BUILD CONFIGURATION

@file        CMakeLists.txt
@brief       Build configuration for ase-adp-libcuckoo
@description Header-only adapter that re-exports CMU's libcuckoo
             (efficient/libcuckoo) cuckoohash_map and connects it to the
             ASE namespace as ase::adp::libcuckoo::Map<K, V>. The upstream
             headers are pulled in via FetchContent at configure time so
             consumers do not need a system-installed libcuckoo.

             Why this adapter exists:
               libcuckoo offers O(1) average-case insert / find / erase
               with fine-grained per-bucket locks, allowing many writer
               threads to publish point-updates concurrently without a
               global mutex on the read path. ASE's git-status scanner
               (clients/ase-client-explorer) needs exactly that profile
               for ~145 nested submodules; the alternative — atomic
               shared_ptr-snapshot swap — forces an O(M) full-map rebuild
               per scan, which does not scale.

             Whitelist:
               No validator whitelist is needed — libcuckoo is pure C++
               header-only, no extern "C", no inheritance the validator
               would flag.

@module      ase-adp-libcuckoo
@layer       adapter (3rd-party isolation, consumed by L1+ code)
@created     2026-05-06
@modified    2026-05-06
@version     00.00.01.00001 [seed]
]]

# Parse local VERSION (SSOT) if not building from root
if(NOT DEFINED ASE_ADP_LIBCUCKOO_VERSION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
        message(FATAL_ERROR "VERSION file not found: ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    endif()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _VERSION_LINES)
    foreach(_LINE ${_VERSION_LINES})
        if(_LINE MATCHES "^ASE_ADP_LIBCUCKOO=(.*)$")
            set(ASE_ADP_LIBCUCKOO_VERSION "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT DEFINED ASE_ADP_LIBCUCKOO_VERSION)
        message(FATAL_ERROR "ASE_ADP_LIBCUCKOO version not found in VERSION file")
    endif()
endif()

project(ase-adp-libcuckoo VERSION ${ASE_ADP_LIBCUCKOO_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

# ── Fetch upstream libcuckoo (header-only) ──────────────────────────
# Upstream pinned to a commit of master that ships cuckoohash_map.hh
# without the optional CityHash dependency. We disable upstream's tests
# and examples so configure stays fast.
include(FetchContent)
set(BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_STRESS_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIVERSAL_BENCHMARK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(libcuckoo_upstream
    GIT_REPOSITORY https://github.com/efficient/libcuckoo.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(libcuckoo_upstream)

# ── Adapter (INTERFACE library) ─────────────────────────────────────
add_library(${PROJECT_NAME} INTERFACE)
add_library(ase::adp::libcuckoo ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Re-export the upstream INTERFACE target so consumers transitively get
# its include path. libcuckoo's CMakeLists adds an INTERFACE library
# called `libcuckoo`.
target_link_libraries(${PROJECT_NAME} INTERFACE libcuckoo)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
)
install(DIRECTORY include/ase
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ── Standalone example (only built when this CMakeLists is the top) ──
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_executable(ase-adp-libcuckoo-example example/example_main.cpp)
    target_link_libraries(ase-adp-libcuckoo-example PRIVATE ${PROJECT_NAME})
endif()
