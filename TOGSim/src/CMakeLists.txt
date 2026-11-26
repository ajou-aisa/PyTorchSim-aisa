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