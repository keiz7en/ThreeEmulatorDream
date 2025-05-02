#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/bitmap.h>

// Snes9x includes
#include "snes9x.h"
#include "memmap.h"
#include "apu/apu.h"
#include "gfx.h"
#include "snapshot.h"
#include "controls.h"

// Define logging macros
#define LOG_TAG "EmulatorSnes9x"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global variables
static bool emulatorInitialized = false;
static uint16_t* videoBuffer = nullptr;
static int width = SNES_WIDTH;
static int height = SNES_HEIGHT;

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
Java_com_example_recreemulcream_emulation_core_SnesEmulatorBridge_nativeInitialize(
        JNIEnv* env,
        jobject /* this */,
        jstring romPath) {

    if (emulatorInitialized) {
        // Clean up previous instance
        Memory.Deinit();
        S9xDeinitAPU();
        S9xGraphicsDeinit();
        emulatorInitialized = false;
    }

    // Convert Java string to C string
    const char* cRomPath = env->GetStringUTFChars(romPath, nullptr);
    LOGI("Initializing SNES9x with ROM: %s", cRomPath);

    // Initialize Snes9x components
    if (!Memory.Init() || !S9xInitAPU() || !S9xGraphicsInit()) {
        LOGE("Failed to initialize SNES9x subsystems");
        env->ReleaseStringUTFChars(romPath, cRomPath);
        return JNI_FALSE;
    }

    // Set up memory and port handlers
    Memory.MapRAM();
    Memory.ClearSRAM();

    // Load ROM
    bool loaded = Memory.LoadROM(cRomPath);
    env->ReleaseStringUTFChars(romPath, cRomPath);
    
    if (!loaded) {
        LOGE("Failed to load SNES ROM");
        return JNI_FALSE;
    }

    // Setup graphics
    S9xInitDisplay(nullptr, nullptr);
    GFX.Pitch = SNES_WIDTH * 2; // 16-bit
    videoBuffer = new uint16_t[SNES_WIDTH * SNES_HEIGHT];
    GFX.Screen = (uint16_t*) videoBuffer;

    // Reset and start emulation
    S9xReset();
    emulatorInitialized = true;

    LOGI("SNES9x initialized successfully");
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_SnesEmulatorBridge_nativeRunFrame(
        JNIEnv* env,
        jobject /* this */,
        jobject bitmap) {

    if (!emulatorInitialized) {
        LOGE("SNES9x not initialized");
        return JNI_FALSE;
    }

    // Run a frame
    S9xMainLoop();

    // Copy frame to bitmap
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGE("Failed to get bitmap info");
        return JNI_FALSE;
    }

    void* pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        LOGE("Failed to lock bitmap pixels");
        return JNI_FALSE;
    }

    // Convert from 16-bit to 32-bit ARGB
    uint32_t* dest = static_cast<uint32_t*>(pixels);
    for (int i = 0; i < SNES_WIDTH * SNES_HEIGHT; i++) {
        uint16_t srcPixel = videoBuffer[i];
        // Convert RGB565 to ARGB8888
        uint8_t r = (srcPixel >> 11) & 0x1F;
        uint8_t g = (srcPixel >> 5) & 0x3F;
        uint8_t b = srcPixel & 0x1F;
        
        r = (r << 3) | (r >> 2);  // 5-bit to 8-bit
        g = (g << 2) | (g >> 4);  // 6-bit to 8-bit
        b = (b << 3) | (b >> 2);  // 5-bit to 8-bit
        
        dest[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    AndroidBitmap_unlockPixels(env, bitmap);
    
    return JNI_TRUE;
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

    bool result = S9xFreezeGame(cPath);
    env->ReleaseStringUTFChars(path, cPath);

    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_SnesEmulatorBridge_nativeLoadState(
        JNIEnv* env,
        jobject /* this */,
        jstring path) {

    if (!emulatorInitialized) {
        LOGE("SNES9x not initialized");
        return JNI_FALSE;
    }

    const char* cPath = env->GetStringUTFChars(path, nullptr);
    LOGI("Loading state from: %s", cPath);

    bool result = S9xUnfreezeGame(cPath);
    env->ReleaseStringUTFChars(path, cPath);

    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_SnesEmulatorBridge_nativeCleanup(
        JNIEnv* env,
        jobject /* this */) {

    if (emulatorInitialized) {
        Memory.Deinit();
        S9xDeinitAPU();
        S9xGraphicsDeinit();
        emulatorInitialized = false;
    }

    if (videoBuffer) {
        delete[] videoBuffer;
        videoBuffer = nullptr;
    }

    LOGI("SNES9x emulator cleaned up");
}