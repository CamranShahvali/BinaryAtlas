cmake_minimum_required(VERSION 3.24)

if(
  NOT DEFINED CMAKE_C_COMPILER
  AND NOT DEFINED ENV{CC}
  AND (
    NOT DEFINED CMAKE_GENERATOR
    OR NOT CMAKE_GENERATOR MATCHES "Visual Studio"
  )
)
  find_program(BINARYATLAS_PREFERRED_C_COMPILER NAMES clang gcc)
  if(BINARYATLAS_PREFERRED_C_COMPILER)
    set(
      CMAKE_C_COMPILER
      "${BINARYATLAS_PREFERRED_C_COMPILER}"
      CACHE FILEPATH "Preferred C compiler for BinaryAtlas" FORCE
    )
  endif()
endif()

if(
  NOT DEFINED CMAKE_CXX_COMPILER
  AND NOT DEFINED ENV{CXX}
  AND (
    NOT DEFINED CMAKE_GENERATOR
    OR NOT CMAKE_GENERATOR MATCHES "Visual Studio"
  )
)
  find_program(BINARYATLAS_PREFERRED_CXX_COMPILER NAMES clang++ g++)
  if(BINARYATLAS_PREFERRED_CXX_COMPILER)
    set(
      CMAKE_CXX_COMPILER
      "${BINARYATLAS_PREFERRED_CXX_COMPILER}"
      CACHE FILEPATH "Preferred C++ compiler for BinaryAtlas" FORCE
    )
  endif()
endif()

project(
  BinaryAtlas
  VERSION 0.1.0
  DESCRIPTION "Static binary analysis engine for recovering structure from native executables."
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BINARYATLAS_BUILD_TESTS "Build BinaryAtlas tests" ON)
option(BINARYATLAS_ENABLE_CLANG_TIDY "Enable clang-tidy for BinaryAtlas targets" OFF)
option(
  BINARYATLAS_USE_SYSTEM_DEPS
  "Prefer system-installed dependencies before falling back to FetchContent"
  ON
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchContent)

if(BINARYATLAS_USE_SYSTEM_DEPS)
  find_package(capstone QUIET)
  find_package(nlohmann_json 3.11 QUIET CONFIG)
endif()

if(BINARYATLAS_USE_SYSTEM_DEPS AND BINARYATLAS_BUILD_TESTS)
  find_package(Catch2 3 QUIET CONFIG)
endif()

if(NOT TARGET capstone::capstone AND NOT TARGET capstone AND NOT TARGET capstone-static)
  set(CAPSTONE_ARCHITECTURE_DEFAULT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "" FORCE)
  set(CAPSTONE_ARM_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_ARM64_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_MIPS_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_PPC_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_SPARC_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_SYSZ_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_XCORE_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_TMS320C64X_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_M680X_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_EVM_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_MOS65XX_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_WASM_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_BPF_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_RISCV_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_SH_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_TRICORE_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_ALPHA_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_HPPA_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_LOONGARCH_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_XTENSA_SUPPORT OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_BUILD_CSTOOL OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_BUILD_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    capstone
    GIT_REPOSITORY https://github.com/capstone-engine/capstone.git
    GIT_TAG 5.0.3
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(capstone)
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

if(BINARYATLAS_BUILD_TESTS AND NOT TARGET Catch2::Catch2WithMain)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(Catch2)
endif()

if(TARGET capstone::capstone)
  set(BINARYATLAS_CAPSTONE_TARGET capstone::capstone)
elseif(TARGET capstone)
  set(BINARYATLAS_CAPSTONE_TARGET capstone)
elseif(TARGET capstone-static)
  set(BINARYATLAS_CAPSTONE_TARGET capstone-static)
elseif(TARGET capstone_shared)
  set(BINARYATLAS_CAPSTONE_TARGET capstone_shared)
else()
  message(FATAL_ERROR "Capstone target could not be resolved")
endif()

if(NOT TARGET capstone)
  if(TARGET capstone-static)
    add_library(capstone ALIAS capstone-static)
  elseif(TARGET capstone_shared)
    add_library(capstone ALIAS capstone_shared)
  endif()
endif()

message(STATUS "BinaryAtlas C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "BinaryAtlas C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "BinaryAtlas Capstone target: ${BINARYATLAS_CAPSTONE_TARGET}")

set(BINARYATLAS_PUBLIC_HEADERS
    include/binaryatlas/core/analyzer.hpp
    include/binaryatlas/core/binary_image.hpp
    include/binaryatlas/core/error.hpp
    include/binaryatlas/core/result.hpp
    include/binaryatlas/core/types.hpp
    include/binaryatlas/format/binary_format.hpp
    include/binaryatlas/format/elf_parser.hpp
    include/binaryatlas/disasm/disassembler.hpp
    include/binaryatlas/disasm/instruction.hpp
    include/binaryatlas/analysis/analysis_result.hpp
    include/binaryatlas/analysis/basic_block.hpp
    include/binaryatlas/analysis/function.hpp
    include/binaryatlas/analysis/graph_algorithms.hpp
    include/binaryatlas/analysis/metrics.hpp
    include/binaryatlas/analysis/recovery.hpp
    include/binaryatlas/graph/dot_exporter.hpp
    include/binaryatlas/heuristics/detector.hpp
    include/binaryatlas/heuristics/finding.hpp
    include/binaryatlas/output/json_writer.hpp
    include/binaryatlas/output/terminal_renderer.hpp
    include/binaryatlas/util/byte_reader.hpp
    include/binaryatlas/util/file_io.hpp
    include/binaryatlas/util/string.hpp
)

set(BINARYATLAS_SOURCES
    src/core/analyzer.cpp
    src/core/binary_image.cpp
    src/core/error.cpp
    src/core/types.cpp
    src/format/elf_parser.cpp
    src/disasm/disassembler.cpp
    src/analysis/graph_algorithms.cpp
    src/analysis/metrics.cpp
    src/analysis/recovery.cpp
    src/graph/dot_exporter.cpp
    src/heuristics/detector.cpp
    src/output/json_writer.cpp
    src/output/terminal_renderer.cpp
    src/util/file_io.cpp
)

add_library(binaryatlas_core STATIC ${BINARYATLAS_PUBLIC_HEADERS} ${BINARYATLAS_SOURCES})
add_library(BinaryAtlas::binaryatlas ALIAS binaryatlas_core)

target_include_directories(
  binaryatlas_core
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(
  binaryatlas_core
  PUBLIC
    ${BINARYATLAS_CAPSTONE_TARGET}
  PRIVATE
    nlohmann_json::nlohmann_json
)

target_compile_features(binaryatlas_core PUBLIC cxx_std_20)

target_compile_definitions(binaryatlas_core PUBLIC BINARYATLAS_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(
    binaryatlas_core
    PUBLIC
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Wshadow
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Woverloaded-virtual
      -Wnull-dereference
      -Wdouble-promotion
      -Wformat=2
  )
endif()

if(BINARYATLAS_ENABLE_CLANG_TIDY)
  find_program(BINARYATLAS_CLANG_TIDY NAMES clang-tidy)
  if(BINARYATLAS_CLANG_TIDY)
    set_target_properties(
      binaryatlas_core
      PROPERTIES CXX_CLANG_TIDY "${BINARYATLAS_CLANG_TIDY}"
    )
  endif()
endif()

add_executable(binaryatlas src/cli/main.cpp)
target_link_libraries(binaryatlas PRIVATE BinaryAtlas::binaryatlas)

install(TARGETS binaryatlas binaryatlas_core)
install(DIRECTORY include/ DESTINATION include)

if(BINARYATLAS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
