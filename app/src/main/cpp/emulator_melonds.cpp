#include <jni.h>
#include <string>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <android/bitmap.h>
#include <math.h>

// Check for melonDS headers
#ifdef __has_include
#if __has_include("NDS.h") && !defined(ENABLE_FALLBACK_MODE)
#define HAS_MELONDS_HEADERS 1
#else
#define HAS_MELONDS_HEADERS 0
#endif
#else
#define HAS_MELONDS_HEADERS 0
#endif

#if HAS_MELONDS_HEADERS
#include "NDS.h"
#include "Config.h"
#include "Platform.h"
#include "GPU.h"
#include "SPU.h"
#include "DSi.h"
#include "DSi_NAND.h"
#include "AREngine.h"
#include "ROMManager.h"
#else
// Forward declarations for stub implementation
namespace NDS {
    inline bool Init() { return false; }
    inline void DeInit() {}
    inline void Start() {}
    inline void Stop() {}
    inline void RunFrame() {}
    inline void SetKeyMask(uint16_t) {}
    inline void TouchScreen(int, int) {}
    inline void ReleaseScreen() {}
    inline bool SaveState(const char*) { return false; }
    inline bool LoadState(const char*) { return false; }
}

namespace Config {
    static bool DSPathsConfigured = false;
    static char BIOS9Path[1024] = {0};
    static char BIOS7Path[1024] = {0}; 
    static char FirmwarePath[1024] = {0};
    
    inline void Load() {}
}

namespace GPU {
    static uint32_t* Framebuffer[2] = {nullptr, nullptr};
}

namespace ROMManager {
    inline bool LoadROM(const char*, bool, bool) { return false; }
}
#endif

#define TAG "EmulatorMelonDS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// DS screen dimensions
// Both screens are 256x192, arranged vertically in a 256x384 buffer
#define DS_SCREEN_WIDTH 256
#define DS_SCREEN_HEIGHT 192
#define DS_COMBINED_HEIGHT (DS_SCREEN_HEIGHT * 2)

// Global variables
static bool melonds_initialized = false;
static uint32_t* video_buffer = NULL;
static int input_state[12] = {0}; // Array for button states
static std::string current_rom_name = "Unknown";

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeInitialize(
        JNIEnv* env, jobject thiz, jstring rom_path) {
    
    const char* path = env->GetStringUTFChars(rom_path, nullptr);
    LOGI("Initializing NDS emulator with ROM: %s (fallback mode)", path);
    
    // Extract ROM filename for display
    current_rom_name = path;
    size_t lastSlash = current_rom_name.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        current_rom_name = current_rom_name.substr(lastSlash + 1);
    }
    
    // Cleanup previous resources if any
    if (video_buffer) {
        free(video_buffer);
        video_buffer = NULL;
    }
    
    // Create video buffer for both screens (top and bottom)
    video_buffer = (uint32_t*)malloc(DS_SCREEN_WIDTH * DS_COMBINED_HEIGHT * sizeof(uint32_t));
    if (!video_buffer) {
        LOGE("Failed to allocate video buffer");
        env->ReleaseStringUTFChars(rom_path, path);
        return JNI_FALSE;
    }
    
    #if HAS_MELONDS_HEADERS
    // Initialize emulator core
    // NDS::Init();
    // NDS::LoadROM(path);
    
    // For now, we're just using placeholder graphics since the actual integration isn't complete
    melonds_initialized = true;
    #else
    LOGI("MelonDS headers not available, using fallback mode");
    
    // Create a test pattern for the top and bottom screens
    for (int screen = 0; screen < 2; screen++) {
        int offsetY = screen * DS_SCREEN_HEIGHT;
        
        for (int y = 0; y < DS_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < DS_SCREEN_WIDTH; x++) {
                uint32_t color;
                
                // Create a distinct pattern for each screen
                if (screen == 0) { // Top screen
                    // Create a gradient pattern
                    uint8_t r = 40 + (x * 80 / DS_SCREEN_WIDTH);
                    uint8_t g = 40 + (y * 80 / DS_SCREEN_HEIGHT);
                    uint8_t b = 120;
                    color = 0xFF000000 | (r << 16) | (g << 8) | b;
                    
                    // Add a border
                    if (x < 4 || x > 252 || y < 4 || y > 188) {
                        color = 0xFFFFFFFF; // White border on top screen
                    }
                } else { // Bottom screen
                    // Create a different gradient for touch screen
                    uint8_t r = 120;
                    uint8_t g = 40 + (x * 80 / DS_SCREEN_WIDTH);
                    uint8_t b = 40 + (y * 80 / DS_SCREEN_HEIGHT);
                    color = 0xFF000000 | (r << 16) | (g << 8) | b;
                    
                    // Add a border
                    if (x < 4 || x > 252 || y < 4 || y > 188) {
                        color = 0xFF000000; // Black border on bottom screen
                    }
                }
                
                video_buffer[(offsetY + y) * DS_SCREEN_WIDTH + x] = color;
            }
        }
        
        // Add "NINTENDO DS" text on top screen
        if (screen == 0) {
            const char* dsText = "NINTENDO DS";
            int textLen = 11;
            int textX = (DS_SCREEN_WIDTH - textLen * 10) / 2;
            int textY = DS_SCREEN_HEIGHT / 2 - 30;
            
            for (int i = 0; i < textLen; i++) {
                for (int y = 0; y < 12; y++) {
                    for (int x = 0; x < 8; x++) {
                        int px = textX + i * 10 + x;
                        int py = textY + y;
                        if (px >= 4 && px < DS_SCREEN_WIDTH - 4 && py >= 4 && py < DS_SCREEN_HEIGHT - 4) {
                            video_buffer[py * DS_SCREEN_WIDTH + px] = 0xFFFFFFFF;
                        }
                    }
                }
            }
            
            // Add ROM name
            int nameY = DS_SCREEN_HEIGHT / 2;
            int maxLen = std::min((int)current_rom_name.length(), 20); // Limit length
            textX = (DS_SCREEN_WIDTH - maxLen * 7) / 2;
            
            for (int i = 0; i < maxLen; i++) {
                for (int y = 0; y < 10; y++) {
                    for (int x = 0; x < 6; x++) {
                        int px = textX + i * 7 + x;
                        int py = nameY + y;
                        if (px >= 4 && px < DS_SCREEN_WIDTH - 4 && py >= 4 && py < DS_SCREEN_HEIGHT - 4) {
                            video_buffer[py * DS_SCREEN_WIDTH + px] = 0xFFFFFF00; // Yellow
                        }
                    }
                }
            }
            
            // Add "FALLBACK MODE" text
            const char* fbText = "FALLBACK MODE";
            textLen = 13;
            textX = (DS_SCREEN_WIDTH - textLen * 7) / 2;
            textY = DS_SCREEN_HEIGHT / 2 + 30;
            
            for (int i = 0; i < textLen; i++) {
                for (int y = 0; y < 10; y++) {
                    for (int x = 0; x < 6; x++) {
                        int px = textX + i * 7 + x;
                        int py = textY + y;
                        if (px >= 4 && px < DS_SCREEN_WIDTH - 4 && py >= 4 && py < DS_SCREEN_HEIGHT - 4) {
                            video_buffer[py * DS_SCREEN_WIDTH + px] = 0xFFFF5555; // Light red
                        }
                    }
                }
            }
        }
        
        // Add "TOUCH SCREEN" text on bottom screen
        if (screen == 1) {
            const char* tsText = "TOUCH SCREEN";
            int textLen = 12;
            int textX = (DS_SCREEN_WIDTH - textLen * 10) / 2;
            int textY = 30;
            
            for (int i = 0; i < textLen; i++) {
                for (int y = 0; y < 12; y++) {
                    for (int x = 0; x < 8; x++) {
                        int px = textX + i * 10 + x;
                        int py = textY + y;
                        int bufferPos = ((DS_SCREEN_HEIGHT + py) * DS_SCREEN_WIDTH) + px;
                        if (px >= 4 && px < DS_SCREEN_WIDTH - 4 && py >= 4 && py < DS_SCREEN_HEIGHT - 4) {
                            video_buffer[bufferPos] = 0xFFFFFFFF;
                        }
                    }
                }
            }
        }
    }
    
    melonds_initialized = true;
    #endif
    
    env->ReleaseStringUTFChars(rom_path, path);
    return melonds_initialized ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeRunFrame(
        JNIEnv* env, jobject thiz, jobject bitmap) {
    
    if (!melonds_initialized || !video_buffer) {
        return JNI_FALSE;
    }
    
    #if HAS_MELONDS_HEADERS
    // Run a single frame
    // NDS::RunFrame();
    
    // Get frame data from both screens
    // memcpy(video_buffer, GPU::Framebuffer[0], DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * sizeof(uint32_t));
    // memcpy(video_buffer + DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT, 
    //        GPU::Framebuffer[1], 
    //        DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * sizeof(uint32_t));
    #else
    // Animate the fallback mode
    static int frame_counter = 0;
    frame_counter++;
    
    // Animate top screen
    if (frame_counter % 60 == 0) {
        for (int y = 4; y < DS_SCREEN_HEIGHT - 4; y++) {
            for (int x = 4; x < DS_SCREEN_WIDTH - 4; x++) {
                // Only modify certain areas (create a wave pattern)
                if ((y + frame_counter / 20) % 30 > 15) {
                    uint8_t phase = (frame_counter / 60) % 6;
                    uint8_t r = 40 + (x * 80 / DS_SCREEN_WIDTH);
                    uint8_t g = 40 + (y * 80 / DS_SCREEN_HEIGHT);
                    uint8_t b = 120;
                    
                    // Shift colors based on phase
                    switch (phase) {
                        case 0: break; // Default
                        case 1: std::swap(r, g); break;
                        case 2: std::swap(g, b); break;
                        case 3: std::swap(r, b); break;
                        case 4: r = 120; g = 80; b = 40; break;
                        case 5: r = 40; g = 120; b = 80; break;
                    }
                    
                    video_buffer[y * DS_SCREEN_WIDTH + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
        }
    }
    
    // Animate touch screen - simulate touches
    int touchX = 128 + (int)(60 * sin(frame_counter * 0.03));
    int touchY = 96 + (int)(40 * cos(frame_counter * 0.05));
    
    // Draw a touch point on the bottom screen
    for (int y = -8; y <= 8; y++) {
        for (int x = -8; x <= 8; x++) {
            int distance = (int)sqrt(x*x + y*y);
            if (distance <= 8) {
                int px = touchX + x;
                int py = touchY + y;
                // Add to bottom screen
                int bufferPos = ((DS_SCREEN_HEIGHT + py) * DS_SCREEN_WIDTH) + px;
                
                if (px >= 4 && px < DS_SCREEN_WIDTH - 4 && py >= 4 && py < DS_SCREEN_HEIGHT - 4) {
                    // Fade based on distance
                    uint32_t alpha = 255 - distance * 25;
                    if (alpha > 255) alpha = 255;
                    video_buffer[bufferPos] = 0xFF000000 | (alpha << 16) | (alpha << 8) | 255;
                }
            }
        }
    }
    
    // Draw animated DS buttons on bottom screen
    int buttonY = DS_SCREEN_HEIGHT + 150;
    int startX = 40;
    int buttonSpacing = 35;
    
    // Draw 4 direction buttons
    for (int dir = 0; dir < 4; dir++) {
        int buttonX;
        int buttonCenterY;
        
        switch (dir) {
            case 0: // Up
                buttonX = startX + buttonSpacing;
                buttonCenterY = buttonY - buttonSpacing;
                break;
            case 1: // Right
                buttonX = startX + 2 * buttonSpacing;
                buttonCenterY = buttonY;
                break;
            case 2: // Down
                buttonX = startX + buttonSpacing;
                buttonCenterY = buttonY + buttonSpacing;
                break;
            case 3: // Left
                buttonX = startX;
                buttonCenterY = buttonY;
                break;
        }
        
        // Determine if button is highlighted in animation
        bool isHighlighted = false;
        switch (dir) {
            case 0: isHighlighted = (frame_counter / 30) % 4 == 0; break;
            case 1: isHighlighted = (frame_counter / 30) % 4 == 1; break;
            case 2: isHighlighted = (frame_counter / 30) % 4 == 2; break;
            case 3: isHighlighted = (frame_counter / 30) % 4 == 3; break;
        }
        
        // Draw triangle button
        for (int y = -10; y <= 10; y++) {
            for (int x = -10; x <= 10; x++) {
                int px = buttonX + x;
                int py = buttonCenterY + y;
                
                // Different shape for each direction
                bool inShape = false;
                switch (dir) {
                    case 0: // Up - triangle pointing up
                        inShape = (x >= -8 && x <= 8) && (y >= -2 && y <= 8) && (abs(x) <= 8 - y);
                        break;
                    case 1: // Right - triangle pointing right
                        inShape = (x >= -8 && x <= 2) && (y >= -8 && y <= 8) && (abs(y) <= 8 + x);
                        break;
                    case 2: // Down - triangle pointing down
                        inShape = (x >= -8 && x <= 8) && (y >= -8 && y <= 2) && (abs(x) <= 8 + y);
                        break;
                    case 3: // Left - triangle pointing left
                        inShape = (x >= -2 && x <= 8) && (y >= -8 && y <= 8) && (abs(y) <= 8 - x);
                        break;
                }
                
                if (inShape && px >= 4 && px < DS_SCREEN_WIDTH - 4 && py >= DS_SCREEN_HEIGHT + 4 && py < DS_COMBINED_HEIGHT - 4) {
                    video_buffer[py * DS_SCREEN_WIDTH + px] = isHighlighted ? 0xFFFFFFFF : 0xFFAAAAAA;
                }
            }
        }
    }
    
    // Draw A and B buttons
    for (int button = 0; button < 2; button++) {
        int buttonX = 180 + button * 40;
        int buttonY = DS_SCREEN_HEIGHT + 160;
        
        // Check animation status
        bool isPressed = (button == 0 && (frame_counter / 20) % 6 == 0) || 
                         (button == 1 && (frame_counter / 20) % 6 == 3);
        
        // Draw circular button
        for (int y = -12; y <= 12; y++) {
            for (int x = -12; x <= 12; x++) {
                int distanceSquared = x*x + y*y;
                if (distanceSquared <= 144) { // Circle with radius 12
                    int px = buttonX + x;
                    int py = buttonY + y;
                    
                    if (px >= 4 && px < DS_SCREEN_WIDTH - 4 && py >= DS_SCREEN_HEIGHT + 4 && py < DS_COMBINED_HEIGHT - 4) {
                        video_buffer[py * DS_SCREEN_WIDTH + px] = isPressed ? 0xFFFFFFFF : 0xFFAA8888;
                    }
                }
            }
        }
        
        // Draw A or B letter
        const char letter = (button == 0) ? 'A' : 'B';
        for (int y = -5; y <= 5; y++) {
            for (int x = -3; x <= 3; x++) {
                int px = buttonX + x;
                int py = buttonY + y;
                
                bool drawPixel = false;
                if (letter == 'A') {
                    drawPixel = (x == -3 && y >= -3) || (x == 3 && y >= -3) || (y == -3) ||
                               (y == 0 && x >= -2 && x <= 2);
                } else { // B
                    drawPixel = (x == -3) || (y == -5 && x >= -2 && x <= 2) || 
                               (y == 0 && x >= -2 && x <= 2) || (y == 5 && x >= -2 && x <= 2) ||
                               (x == 3 && y != -5 && y != 0 && y != 5 && y > -5 && y < 5);
                }
                
                if (drawPixel && px >= 4 && px < DS_SCREEN_WIDTH - 4 && py >= DS_SCREEN_HEIGHT + 4 && py < DS_COMBINED_HEIGHT - 4) {
                    video_buffer[py * DS_SCREEN_WIDTH + px] = isPressed ? 0xFF000000 : 0xFF000000;
                }
            }
        }
    }
    #endif
    
    // Draw to the provided bitmap
    AndroidBitmapInfo info;
    void* pixels;
    
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGE("Failed to get bitmap info");
        return 0;
    }
    
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        LOGE("Failed to lock bitmap pixels");
        return 0;
    }
    
    // Copy buffer to bitmap
    uint32_t* dst = (uint32_t*)pixels;
    
    // Check if bitmap dimensions match our buffer
    if (info.width == DS_SCREEN_WIDTH && info.height == DS_COMBINED_HEIGHT) {
        // Direct copy
        memcpy(dst, video_buffer, DS_SCREEN_WIDTH * DS_COMBINED_HEIGHT * sizeof(uint32_t));
    } else {
        // Need to scale
        float scaleX = (float)DS_SCREEN_WIDTH / info.width;
        float scaleY = (float)DS_COMBINED_HEIGHT / info.height;
        
        for (int y = 0; y < info.height; y++) {
            int src_y = (int)(y * scaleY);
            if (src_y >= DS_COMBINED_HEIGHT) src_y = DS_COMBINED_HEIGHT - 1;
            
            for (int x = 0; x < info.width; x++) {
                int src_x = (int)(x * scaleX);
                if (src_x >= DS_SCREEN_WIDTH) src_x = DS_SCREEN_WIDTH - 1;
                
                dst[y * info.stride / 4 + x] = video_buffer[src_y * DS_SCREEN_WIDTH + src_x];
            }
        }
    }
    
    AndroidBitmap_unlockPixels(env, bitmap);

    return 16; // ~60 FPS
    #else
    return 0;
    #endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeSetInput(
        JNIEnv* env,
        jobject thiz,
        jintArray buttons) {

    if (!emulatorInitialized) {
        return;
    }

    jint* buttonStates = env->GetIntArrayElements(buttons, NULL);
    jsize len = env->GetArrayLength(buttons);

    // Map buttons to NDS buttons
    uint16_t keyInput = 0xFFFF;
    
    if (len >= 12) { // Make sure we have enough elements
        if (buttonStates[0]) keyInput &= ~(1 << 6);  // Up
        if (buttonStates[1]) keyInput &= ~(1 << 7);  // Down
        if (buttonStates[2]) keyInput &= ~(1 << 5);  // Left
        if (buttonStates[3]) keyInput &= ~(1 << 4);  // Right
        if (buttonStates[4]) keyInput &= ~(1 << 0);  // A
        if (buttonStates[5]) keyInput &= ~(1 << 1);  // B
        if (buttonStates[6]) keyInput &= ~(1 << 10); // X
        if (buttonStates[7]) keyInput &= ~(1 << 11); // Y
        if (buttonStates[8]) keyInput &= ~(1 << 9);  // L
        if (buttonStates[9]) keyInput &= ~(1 << 8);  // R
        if (buttonStates[10]) keyInput &= ~(1 << 2); // Select
        if (buttonStates[11]) keyInput &= ~(1 << 3); // Start
    }
    
    #if HAS_MELONDS_HEADERS
    NDS::SetKeyMask(keyInput);
    #endif
    
    env->ReleaseIntArrayElements(buttons, buttonStates, 0);

    // For touch, we'd need additional parameters or a separate method
    // For now, just release the touch screen
    #if HAS_MELONDS_HEADERS
    NDS::ReleaseScreen();
    #endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeSaveState(
        JNIEnv* env,
        jobject thiz) {

    if (!emulatorInitialized) {
        LOGE("melonDS not initialized");
        return JNI_FALSE;
    }

    // Use a default path in the app's data directory
    std::string savePath = "/data/data/com.example.recreemulcream/files/saves/ds/savestate.mln";
    LOGI("Saving state to: %s", savePath.c_str());
    
    #if HAS_MELONDS_HEADERS
    bool success = NDS::SaveState(savePath.c_str());
    return success ? JNI_TRUE : JNI_FALSE;
    #else
    return JNI_FALSE;
    #endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeLoadState(
        JNIEnv* env,
        jobject thiz) {

    if (!emulatorInitialized) {
        LOGE("melonDS not initialized");
        return JNI_FALSE;
    }

    // Use a default path in the app's data directory
    std::string savePath = "/data/data/com.example.recreemulcream/files/saves/ds/savestate.mln";
    LOGI("Loading state from: %s", savePath.c_str());
    
    #if HAS_MELONDS_HEADERS
    bool success = NDS::LoadState(savePath.c_str());
    return success ? JNI_TRUE : JNI_FALSE;
    #else
    return JNI_FALSE;
    #endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeShutdown(
        JNIEnv* env,
        jobject thiz) {
    if (emulatorInitialized) {
        #if HAS_MELONDS_HEADERS
        NDS::Stop();
        NDS::DeInit();
        #endif
        emulatorInitialized = false;
    }

    if (videoBuffer) {
        delete[] videoBuffer;
        videoBuffer = nullptr;
    }

    LOGI("melonDS emulator cleaned up");
}