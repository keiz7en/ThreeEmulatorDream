cmake_minimum_required(VERSION 3.10)
project("recreemulcream")

# Set compile flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../mgba-master/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../mgba-master/src
)

# Create main library with basic files
add_library(recreemulcream SHARED
    native-lib.cpp
    emulator_mgba.cpp
)

# Basic libraries to link against
target_link_libraries(recreemulcream
    android
    log
    EGL
    GLESv2
    jnigraphics
)

# Configure mGBA build - Simplified approach
# Create a minimalist mGBA Core
file(GLOB MGBA_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../mgba-master/src/core/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../mgba-master/src/gba/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../mgba-master/src/arm/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../mgba-master/src/util/*.c
)

add_library(mgba-core STATIC ${MGBA_CORE_SOURCES})

target_compile_definitions(mgba-core PRIVATE
    MINIMAL_CORE=1
    M_CORE_GBA=1
    COLOR_16_BIT=1
    DISABLE_THREADING=1
)

# Link to our main library
target_link_libraries(recreemulcream mgba-core)