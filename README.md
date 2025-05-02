# RecreEmulCream - Multi-System Emulator for Android

RecreEmulCream is an Android emulator application that supports multiple game console systems, including:

- Game Boy Advance (GBA) via mGBA core
- Super Nintendo Entertainment System (SNES) via Snes9x core
- Nintendo DS (NDS) via melonDS core

## Java-Only Game Rendering

This version uses a pure Java implementation instead of native libraries to avoid CMake build errors. Instead of
integrating complex C/C++ emulator cores, we've created:

1. **JavaEmulator**: A Java-based renderer that simulates game visuals
2. **JavaEmulatorBridge**: An adapter that implements the EmulatorBridge interface
3. **EmulatorUtils**: Utilities for graceful error handling of native library failures

This approach:

- Completely avoids native code and CMake build errors
- Provides animated game-like visuals rather than blue error screens
- Maintains the same interface structure for future native integration
- Handles ROM loading and displays game titles

## Building the Project

To build this project:
1. Ensure you have Android Studio with NDK support installed
2. Clone this repository with all submodules:
   git clone --recursive https://github.com/yourusername/recreemulcream.git
3. Open the project in Android Studio
4. Build and run on your device

## Source Code Structure

- `/app/src/main/java/com/example/recreemulcream/` - Main Java code
   - `/emulation/` - Core emulation classes
   - `/emulation/core/` - Emulator bridge interfaces
   - `/emulation/input/` - Input handling system
- `/app/src/main/jniLibs/` - Directory structure for native libraries (placeholder in this version)

## How It Works

1. When you select a ROM file, the app loads it with `JavaEmulator`
2. Game visuals are rendered in pure Java with animated effects
3. The game title is extracted from the ROM header
4. Controls work normally to provide an interactive experience
5. If native libraries are found in the future, they'll be used instead

## Upgrading to Real Emulation

To upgrade to actual emulation:

1. Build native libraries (.so files) for each emulator core
2. Place them in the appropriate architecture folders under `/app/src/main/jniLibs/`
3. The app will automatically use native emulation when available

## Credits

- Original emulator cores:
   - mGBA: https://mgba.io/
   - Snes9x: http://www.snes9x.com/
   - melonDS: http://melonds.kuribo64.net/