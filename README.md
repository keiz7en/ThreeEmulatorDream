# RecreEmulCream - Retro Game Emulator

Multi-system retro game emulator for Android supporting NES, SNES, and GBA.

## Current Status

The app is currently in development with a working UI framework but placeholder emulator cores. The current version
shows colored screens for demonstration purposes only.

## Integrating Native Emulator Cores

To integrate the actual emulator cores (melonDS, mGBA, SNES9x), follow these steps:

### 1. Enable Native Build

Uncomment the native build configurations in `app/build.gradle.kts`:

```kotlin
externalNativeBuild {
    cmake {
        cppFlags += "-std=c++17"
        arguments += "-DANDROID_STL=c++_shared"
    }
}

// ...

externalNativeBuild {
    cmake {
        path = file("src/main/cpp/CMakeLists.txt")
    }
}

ndkVersion = "21.4.7075529" // or another compatible version
```

### 2. Download Emulator Core Sources

Download and extract the source code for the emulator cores:

- mGBA: https://github.com/mgba-emu/mgba
- SNES9x: https://github.com/snes9xgit/snes9x
- MelonDS: https://github.com/melonDS-emu/melonDS

### 3. Update CMakeLists.txt

Edit `app/src/main/cpp/CMakeLists.txt` to include the emulator core sources:

```cmake
# mGBA for GBA
add_subdirectory(mgba)
target_include_directories(recreemulcream PRIVATE mgba/include)
target_link_libraries(recreemulcream mgba-core)

# SNES9x for SNES
add_subdirectory(snes9x)
target_include_directories(recreemulcream PRIVATE snes9x/src)
target_link_libraries(recreemulcream snes9x-lib)

# melonDS for DS
add_subdirectory(melonDS)
target_include_directories(recreemulcream PRIVATE melonDS/src)
target_link_libraries(recreemulcream melonds-core)
```

### 4. Implement Native Bridge Methods

Uncomment and implement the native methods in `EmulatorBridge.java` and their C++ counterparts in the native code files.

### 5. Configure Native Libraries

Enable the System.loadLibrary calls in MainActivity.java

## Button Customization

The app allows customizing the size and position of buttons through the Settings menu.

## Performance Tuning

- Enable frameskip for lower-end devices
- Use the FPS counter to monitor performance
- Configure audio latency for optimal experience

## License

This project is licensed under the MIT License - see the LICENSE file for details.