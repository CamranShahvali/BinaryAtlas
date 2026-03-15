include(Catch)

set(BINARYATLAS_FIXTURE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/fixtures/bin)

function(binaryatlas_add_fixture target source)
  add_executable(${target} ${source})
  set_target_properties(
    ${target}
    PROPERTIES
      OUTPUT_NAME ${target}
      RUNTIME_OUTPUT_DIRECTORY ${BINARYATLAS_FIXTURE_OUTPUT_DIR}
  )

  if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target} PRIVATE -g -fno-pie)
    target_link_options(${target} PRIVATE -no-pie)
  endif()
endfunction()

binaryatlas_add_fixture(straight_line fixtures/src/straight_line.c)
target_compile_options(straight_line PRIVATE -O0)

binaryatlas_add_fixture(straight_line_cpp fixtures/src/straight_line_cpp.cpp)
target_compile_options(straight_line_cpp PRIVATE -O0)

binaryatlas_add_fixture(if_else fixtures/src/if_else.c)
target_compile_options(if_else PRIVATE -O0)

binaryatlas_add_fixture(branching_cpp fixtures/src/branching_cpp.cpp)
target_compile_options(branching_cpp PRIVATE -O0)

binaryatlas_add_fixture(loop fixtures/src/loop.c)
target_compile_options(loop PRIVATE -O0)

binaryatlas_add_fixture(loop_cpp fixtures/src/loop_cpp.cpp)
target_compile_options(loop_cpp PRIVATE -O0)

binaryatlas_add_fixture(switch_case fixtures/src/switch_case.c)
target_compile_options(switch_case PRIVATE -O2)

binaryatlas_add_fixture(direct_calls fixtures/src/direct_calls.c)
target_compile_options(direct_calls PRIVATE -O0)

binaryatlas_add_fixture(indirect_call fixtures/src/indirect_call.c)
target_compile_options(indirect_call PRIVATE -O0)

binaryatlas_add_fixture(virtual_dispatch fixtures/src/virtual_dispatch.cpp)
target_compile_options(virtual_dispatch PRIVATE -O0)

binaryatlas_add_fixture(graphics_keywords fixtures/src/graphics_keywords.c)
target_compile_options(graphics_keywords PRIVATE -O0)

add_custom_command(
  OUTPUT ${BINARYATLAS_FIXTURE_OUTPUT_DIR}/direct_calls_stripped
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:direct_calls> ${BINARYATLAS_FIXTURE_OUTPUT_DIR}/direct_calls_stripped
  COMMAND ${CMAKE_STRIP} --strip-all ${BINARYATLAS_FIXTURE_OUTPUT_DIR}/direct_calls_stripped
  DEPENDS direct_calls
)
add_custom_target(binaryatlas_fixture_stripped ALL DEPENDS ${BINARYATLAS_FIXTURE_OUTPUT_DIR}/direct_calls_stripped)

add_executable(
  binaryatlas_tests
  unit/test_elf_parser.cpp
  unit/test_disassembler.cpp
  unit/test_graph_algorithms.cpp
  unit/test_json_dot.cpp
  unit/test_heuristics.cpp
  integration/test_integration_analysis.cpp
)

add_dependencies(
  binaryatlas_tests
  straight_line
  straight_line_cpp
  if_else
  branching_cpp
  loop
  loop_cpp
  switch_case
  direct_calls
  indirect_call
  virtual_dispatch
  graphics_keywords
  binaryatlas_fixture_stripped
)

target_include_directories(binaryatlas_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(
  binaryatlas_tests
  PRIVATE
    BinaryAtlas::binaryatlas
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)
target_compile_definitions(
  binaryatlas_tests
  PRIVATE
    BINARYATLAS_TEST_FIXTURE_DIR="${BINARYATLAS_FIXTURE_OUTPUT_DIR}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(binaryatlas_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

catch_discover_tests(binaryatlas_tests)
