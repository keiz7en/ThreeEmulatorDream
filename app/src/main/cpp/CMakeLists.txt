cmake_minimum_required(VERSION 3.18.1)
project(recreemulcream)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")

# Define source files
set(SOURCES
    native-lib.cpp
    file_utils.cpp
    emulator_snes9x.cpp
    emulator_mgba.cpp
    emulator_melonds.cpp
)

# Define main library target
add_library(recreemulcream SHARED ${SOURCES})

# Create DS emulation library
add_library(ds-lib SHARED
    emulator_melonds.cpp
)

# Link libraries for the main target
target_link_libraries(recreemulcream
    android
    log
    jnigraphics
    EGL
    GLESv2
    z
)

# Link libraries for DS emulation
target_link_libraries(ds-lib
    android
    log
    jnigraphics
    z
)

# Add definitions
add_definitions(-DENABLE_FALLBACK_MODE=1)
add_definitions(-DENABLE_EMULATORS=1)