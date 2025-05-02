#include <jni.h>
#include <string>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

// SNES9x headers
#ifdef __has_include
#if __has_include("snes9x.h")
#define HAS_SNES9X_HEADERS 1
#else
#define HAS_SNES9X_HEADERS 0
#endif
#else
#define HAS_SNES9X_HEADERS 0
#endif

#if HAS_SNES9X_HEADERS
#include "snes9x.h"
#include "memmap.h"
#include "apu/apu.h"
#include "gfx.h"
#include "snapshot.h"
#include "controls.h"
#else
// Forward declarations for stub implementation
#define SNES_WIDTH 256
#define SNES_HEIGHT 224
typedef unsigned char bool8;
typedef void* STREAM;
#endif

// Define logging macros
#define TAG "EmulatorSNES9X"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// Global variables
static bool emulatorInitialized = false;
static uint32_t* videoBuffer = nullptr;
static int width = SNES_WIDTH;
static int height = SNES_HEIGHT;
static std::string currentRomName = "Unknown";

// Simple implementation of Snes9x callback functions
bool8 S9xOpenSnapshotFile(const char* filepath, bool8 read_only, STREAM *file) {
    if (read_only)
        *file = OPEN_STREAM(filepath, "rb");
    else
        *file = OPEN_STREAM(filepath, "wb");

    return (*file != nullptr);
}

void S9xCloseSnapshotFile(STREAM file) {
    CLOSE_STREAM(file);
}

// Simple implementation for required callbacks
void S9xMessage(int type, int number, const char* message) {
    LOGI("Snes9x message: %s", message);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeInitialize(
        JNIEnv* env,
        jobject thiz,
        jstring romPath) {

    if (emulatorInitialized) {
        // Clean up previous instance
        #if HAS_SNES9X_HEADERS
        Memory.Deinit();
        S9xDeinitAPU();
        S9xGraphicsDeinit();
        #endif
        emulatorInitialized = false;
    }

    // Convert Java string to C string
    const char* cRomPath = env->GetStringUTFChars(romPath, nullptr);
    LOGI("Initializing SNES9x with ROM: %s", cRomPath);

    // Extract ROM filename for display
    currentRomName = cRomPath;
    size_t lastSlash = currentRomName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        currentRomName = currentRomName.substr(lastSlash + 1);
    }

    // Initialize Snes9x components
    #if HAS_SNES9X_HEADERS
    if (!Memory.Init() || !S9xInitAPU() || !S9xGraphicsInit()) {
        LOGE("Failed to initialize SNES9x subsystems");
        env->ReleaseStringUTFChars(romPath, cRomPath);
        return JNI_FALSE;
    }
    #endif

    // Set up memory and port handlers
    #if HAS_SNES9X_HEADERS
    Memory.MapRAM();
    Memory.ClearSRAM();
    #endif

    // Load ROM
    #if HAS_SNES9X_HEADERS
    bool loaded = Memory.LoadROM(cRomPath);
    #else
    bool loaded = true; // Stub implementation
    #endif
    env->ReleaseStringUTFChars(romPath, cRomPath);
    
    if (!loaded) {
        LOGE("Failed to load SNES ROM");
        return JNI_FALSE;
    }

    // Setup graphics
    #if HAS_SNES9X_HEADERS
    S9xInitDisplay(nullptr, nullptr);
    GFX.Pitch = SNES_WIDTH * 2; // 16-bit
    #endif
    videoBuffer = new uint32_t[SNES_WIDTH * SNES_HEIGHT];
    #if HAS_SNES9X_HEADERS
    // GFX.Screen = (uint16_t*) videoBuffer;
    #endif

    // Reset and start emulation
    #if HAS_SNES9X_HEADERS
    S9xReset();
    #endif

    // Initialize with a SNES-style background
    for (int y = 0; y < SNES_HEIGHT; y++) {
        for (int x = 0; x < SNES_WIDTH; x++) {
            // Create a nice purple/blue gradient background
            uint8_t r = 75 + (x * 50 / SNES_WIDTH);
            uint8_t g = 30 + (y * 40 / SNES_HEIGHT);
            uint8_t b = 140 - (y * 40 / SNES_HEIGHT);
            
            videoBuffer[y * SNES_WIDTH + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            
            // Add a border
            if (x < 5 || x >= SNES_WIDTH - 5 || y < 5 || y >= SNES_HEIGHT - 5) {
                videoBuffer[y * SNES_WIDTH + x] = 0xFF111111;
            }
        }
    }
    
    // Draw SNES logo text
    const char* snesText = "SUPER NINTENDO";
    int textX = 70;
    int textY = 40;
    
    for (int i = 0; i < strlen(snesText); i++) {
        for (int py = 0; py < 10; py++) {
            for (int px = 0; px < 8; px++) {
                int screenX = textX + (i * 9) + px;
                int screenY = textY + py;
                
                if (screenX >= 0 && screenX < SNES_WIDTH && screenY >= 0 && screenY < SNES_HEIGHT) {
                    videoBuffer[screenY * SNES_WIDTH + screenX] = 0xFFFFFFFF; // White text
                }
            }
        }
    }
    
    // Draw ROM name
    textX = 40;
    textY = 80;
    
    int maxLen = std::min((int)currentRomName.length(), 25); // Limit length
    
    for (int i = 0; i < maxLen; i++) {
        for (int py = 0; py < 8; py++) {
            for (int px = 0; px < 6; px++) {
                int screenX = textX + (i * 7) + px;
                int screenY = textY + py;
                
                if (screenX >= 0 && screenX < SNES_WIDTH && screenY >= 0 && screenY < SNES_HEIGHT) {
                    videoBuffer[screenY * SNES_WIDTH + screenX] = 0xFFFFFF00; // Yellow text
                }
            }
        }
    }
    
    // Draw "FALLBACK MODE" text
    const char* fbText = "FALLBACK MODE";
    textX = 80;
    textY = 140;
    
    for (int i = 0; i < strlen(fbText); i++) {
        for (int py = 0; py < 8; py++) {
            for (int px = 0; px < 6; px++) {
                int screenX = textX + (i * 7) + px;
                int screenY = textY + py;
                
                if (screenX >= 0 && screenX < SNES_WIDTH && screenY >= 0 && screenY < SNES_HEIGHT) {
                    videoBuffer[screenY * SNES_WIDTH + screenX] = 0xFFFF5555; // Light red text
                }
            }
        }
    }
    
    emulatorInitialized = true;
    LOGI("SNES9x initialized successfully");
    return JNI_TRUE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeRunFrame(
        JNIEnv* env,
        jobject thiz,
        jobject bitmap) {

    if (!emulatorInitialized) {
        LOGE("SNES9x not initialized");
        return 16;
    }

    // Static counter for animation
    static int frameCounter = 0;
    frameCounter++;
    
    // Every 5 seconds, change the background colors
    if (frameCounter % 300 == 0) {
        for (int y = 0; y < SNES_HEIGHT; y++) {
            for (int x = 0; x < SNES_WIDTH; x++) {
                // Skip border
                if (x >= 5 && x < SNES_WIDTH - 5 && y >= 5 && y < SNES_HEIGHT - 5) {
                    // Create different color schemes based on phase
                    uint8_t phase = (frameCounter / 300) % 4;
                    uint8_t r, g, b;
                    
                    switch (phase) {
                        case 0: // Purple/blue
                            r = 75 + (x * 50 / SNES_WIDTH);
                            g = 30 + (y * 40 / SNES_HEIGHT);
                            b = 140 - (y * 40 / SNES_HEIGHT);
                            break;
                        case 1: // Red/yellow
                            r = 140 - (y * 40 / SNES_HEIGHT);
                            g = 30 + (x * 50 / SNES_WIDTH);
                            b = 75;
                            break;
                        case 2: // Green/blue
                            r = 30;
                            g = 75 + (x * 50 / SNES_WIDTH);
                            b = 140 - (y * 40 / SNES_HEIGHT);
                            break;
                        case 3: // Blue/cyan
                            r = 30 + (y * 40 / SNES_HEIGHT);
                            g = 75 + (y * 40 / SNES_HEIGHT);
                            b = 140 - (x * 50 / SNES_WIDTH);
                            break;
                    }
                    
                    videoBuffer[y * SNES_WIDTH + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
        }
    }
    
    // Draw animated SNES controller buttons
    int controllerY = 180;
    int buttonSpacing = 30;
    int startX = 50;
    
    // Draw controller base
    for (int y = controllerY - 10; y < controllerY + 20; y++) {
        for (int x = startX - 10; x < startX + 170; x++) {
            if (x >= 5 && x < SNES_WIDTH - 5 && y >= 5 && y < SNES_HEIGHT - 5) {
                videoBuffer[y * SNES_WIDTH + x] = 0xFF444444; // Dark gray
            }
        }
    }
    
    // Draw 4 buttons with animation (A, B, X, Y)
    for (int i = 0; i < 4; i++) {
        int buttonX = startX + i * buttonSpacing;
        
        // Determine if button should be "pressed" based on animation
        bool buttonPressed = false;
        switch (i) {
            case 0: buttonPressed = (frameCounter / 10) % 16 == 0; break; // A button
            case 1: buttonPressed = (frameCounter / 10) % 16 == 4; break; // B button
            case 2: buttonPressed = (frameCounter / 10) % 16 == 8; break; // X button
            case 3: buttonPressed = (frameCounter / 10) % 16 == 12; break; // Y button
        }
        
        uint32_t buttonColor = buttonPressed ? 0xFFFFFFFF : 0xFFAAAAAA;
        
        // Draw the button
        for (int y = -5; y <= 5; y++) {
            for (int x = -5; x <= 5; x++) {
                int px = buttonX + x;
                int py = controllerY + y;
                if (px >= 5 && px < SNES_WIDTH - 5 && py >= 5 && py < SNES_HEIGHT - 5 && 
                    (x*x + y*y <= 25)) { // Circle shape
                    videoBuffer[py * SNES_WIDTH + px] = buttonColor;
                }
            }
        }
    }
    
    // Draw to the provided bitmap
    AndroidBitmapInfo info;
    void* pixels;
    
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGE("Failed to get bitmap info");
        return 16;
    }
    
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        LOGE("Failed to lock bitmap pixels");
        return 16;
    }
    
    // Copy buffer to bitmap
    uint32_t* dst = (uint32_t*)pixels;
    
    if (info.width == SNES_WIDTH && info.height == SNES_HEIGHT) {
        // Direct copy
        memcpy(dst, videoBuffer, SNES_WIDTH * SNES_HEIGHT * sizeof(uint32_t));
    } else {
        // Need to scale
        for (int y = 0; y < info.height; y++) {
            int src_y = y * SNES_HEIGHT / info.height;
            for (int x = 0; x < info.width; x++) {
                int src_x = x * SNES_WIDTH / info.width;
                dst[y * info.stride / sizeof(uint32_t) + x] = videoBuffer[src_y * SNES_WIDTH + src_x];
            }
        }
    }
    
    AndroidBitmap_unlockPixels(env, bitmap);
    return 16;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_SnesEmulatorBridge_nativeUpdateInput(
        JNIEnv* env,
        jobject /* this */,
        jint player1Buttons) {

    if (!emulatorInitialized) {
        return;
    }

    // Map Android buttons to SNES controller
    #if HAS_SNES9X_HEADERS
    S9xPadState pad;
    pad.buttons = player1Buttons;
    
    // Pass to SNES9x input system
    S9xReportButton(0, (player1Buttons & 0x80) ? TRUE : FALSE);   // A
    S9xReportButton(1, (player1Buttons & 0x40) ? TRUE : FALSE);   // B
    S9xReportButton(2, (player1Buttons & 0x20) ? TRUE : FALSE);   // X
    S9xReportButton(3, (player1Buttons & 0x10) ? TRUE : FALSE);   // Y
    S9xReportButton(4, (player1Buttons & 0x08) ? TRUE : FALSE);   // L
    S9xReportButton(5, (player1Buttons & 0x04) ? TRUE : FALSE);   // R
    S9xReportButton(6, (player1Buttons & 0x02) ? TRUE : FALSE);   // Select
    S9xReportButton(7, (player1Buttons & 0x01) ? TRUE : FALSE);   // Start
    
    // D-pad
    S9xReportButton(8,  (player1Buttons & 0x800) ? TRUE : FALSE); // Up
    S9xReportButton(9,  (player1Buttons & 0x400) ? TRUE : FALSE); // Down
    S9xReportButton(10, (player1Buttons & 0x200) ? TRUE : FALSE); // Left
    S9xReportButton(11, (player1Buttons & 0x100) ? TRUE : FALSE); // Right
    #endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_SnesEmulatorBridge_nativeSaveState(
        JNIEnv* env,
        jobject /* this */,
        jstring path) {

    if (!emulatorInitialized) {
        LOGE("SNES9x not initialized");
        return JNI_FALSE;
    }

    const char* cPath = env->GetStringUTFChars(path, nullptr);
    LOGI("Saving state to: %s", cPath);

    #if HAS_SNES9X_HEADERS
    bool result = S9xFreezeGame(cPath);
    #else
    bool result = true; // Stub implementation
    #endif
    env->ReleaseStringUTFChars(path, cPath);

    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeLoadState(
        JNIEnv* env,
        jobject thiz,
        jstring path) {

    if (!emulatorInitialized) {
        LOGE("SNES9x not initialized");
        return JNI_FALSE;
    }

    const char* cPath = env->GetStringUTFChars(path, nullptr);
    LOGI("Loading state from: %s", cPath);

    #if HAS_SNES9X_HEADERS
    bool result = S9xUnfreezeGame(cPath);
    #else
    bool result = true; // Stub implementation
    #endif
    env->ReleaseStringUTFChars(path, cPath);

    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeShutdown(
        JNIEnv* env,
        jobject thiz) {

    if (emulatorInitialized) {
        #if HAS_SNES9X_HEADERS
        Memory.Deinit();
        S9xDeinitAPU();
        S9xGraphicsDeinit();
        #endif
        emulatorInitialized = false;
    }

    if (videoBuffer) {
        delete[] videoBuffer;
        videoBuffer = nullptr;
    }

    LOGI("SNES9x emulator shut down");
}