cmake_minimum_required(VERSION 3.18.1)
project(recreemulcream)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")

# Define minimal stub source files
set(STUB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/native-lib.cpp
)

# Main library for GBA and SNES
add_library(recreemulcream SHARED
    ${STUB_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/emulator_mgba.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/emulator_snes9x.cpp
)

# Separate DS library
add_library(ds-lib SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/emulator_melonds.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/file_utils.cpp
)

# Enable fallback mode to avoid relying on complex dependencies
target_compile_definitions(recreemulcream PRIVATE
    MINIMAL_IMPLEMENTATION=1
)

target_compile_definitions(ds-lib PRIVATE
    MINIMAL_IMPLEMENTATION=1
)

# Link with required Android libraries
target_link_libraries(recreemulcream
    android
    log
    jnigraphics
)

target_link_libraries(ds-lib
    android
    log
    jnigraphics
    z
)

# Add definitions
add_definitions(-DENABLE_FALLBACK_MODE=1)
add_definitions(-DENABLE_EMULATORS=1)