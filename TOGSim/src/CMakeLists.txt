cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 23)

# Collect existing TOGSim sources.
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
)

# Separate the CLI entry point from the engine.
set(CLI_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/main.cc")

list(REMOVE_ITEM SRC_FILES "${CLI_SOURCE}")

# ISA-driven NPU sources live in their own library (below).
list(FILTER SRC_FILES EXCLUDE REGEX "/src/npu/")

# Reusable simulation engine.
add_library(togsim_engine STATIC ${SRC_FILES})

target_compile_definitions(togsim_engine PUBLIC
    _GLIBCXX_USE_CXX11_ABI=0
)

# Preserve the original CLI executable.
add_executable(Simulator "${CLI_SOURCE}")

target_link_libraries(Simulator PRIVATE
    togsim_engine
)

# Standalone engine API smoke test.
add_executable(engine_smoke EXCLUDE_FROM_ALL
    "${CMAKE_SOURCE_DIR}/tests/engine_smoke.cc"
)

target_link_libraries(engine_smoke PRIVATE
    togsim_engine
)

# ISA-driven NPU library: no ONNX, STONNE, BookSim, Ramulator or YAML link.
# The root CMake still applies the old GNU C++ ABI globally.
add_library(togsim_npu STATIC
    npu/MemoryPort.cc
)

target_include_directories(togsim_npu PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
)

set_target_properties(togsim_npu PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
)

add_executable(memory_port_smoke EXCLUDE_FROM_ALL
    "${CMAKE_SOURCE_DIR}/tests/memory_port_smoke.cc"
)

target_link_libraries(memory_port_smoke PRIVATE
    togsim_npu
)

add_executable(npu_config_smoke EXCLUDE_FROM_ALL
    "${CMAKE_SOURCE_DIR}/tests/npu_config_smoke.cc"
)

target_link_libraries(npu_config_smoke PRIVATE
    togsim_npu
)