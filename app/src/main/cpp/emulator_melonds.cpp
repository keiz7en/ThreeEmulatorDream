#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/bitmap.h>

#include "NDS.h"
#include "Config.h"
#include "Platform.h"
#include "GPU.h"
#include "SPU.h"
#include "DSi.h"
#include "DSi_NAND.h"
#include "AREngine.h"
#include "ROMManager.h"

#define LOG_TAG "EmulatorMelonDS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global variables
static bool emulatorInitialized = false;
static uint32_t* videoBuffer = nullptr;
static int width = 256;  // DS screen width
static int height = 384; // Combined DS screen height (192 * 2)
static const int screenSize = width * height;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_DsEmulatorBridge_nativeInitialize(
        JNIEnv* env,
        jobject /* this */,
        jstring romPath,
        jstring biosPath) {

    if (emulatorInitialized) {
        // Clean up previous instance
        NDS::DeInit();
        emulatorInitialized = false;
    }

    // Convert Java strings to C strings
    const char* cRomPath = env->GetStringUTFChars(romPath, nullptr);
    const char* cBiosPath = env->GetStringUTFChars(biosPath, nullptr);
    
    LOGI("Initializing melonDS with ROM: %s, BIOS: %s", cRomPath, cBiosPath);

    // Set up config
    Config::Load();
    Config::DSPathsConfigured = true;
    
    // Set BIOS paths
    strncpy(Config::BIOS9Path, cBiosPath, 1023);
    strncpy(Config::BIOS7Path, cBiosPath, 1023);
    strncpy(Config::FirmwarePath, cBiosPath, 1023);
    
    Config::BIOS9Path[1023] = '\0';
    Config::BIOS7Path[1023] = '\0';
    Config::FirmwarePath[1023] = '\0';
    
    // Initialize emulator
    if (!NDS::Init()) {
        LOGE("Failed to initialize melonDS");
        env->ReleaseStringUTFChars(romPath, cRomPath);
        env->ReleaseStringUTFChars(biosPath, cBiosPath);
        return JNI_FALSE;
    }
    
    // Load ROM
    if (!ROMManager::LoadROM(cRomPath, false, false)) {
        LOGE("Failed to load DS ROM");
        NDS::DeInit();
        env->ReleaseStringUTFChars(romPath, cRomPath);
        env->ReleaseStringUTFChars(biosPath, cBiosPath);
        return JNI_FALSE;
    }

    env->ReleaseStringUTFChars(romPath, cRomPath);
    env->ReleaseStringUTFChars(biosPath, cBiosPath);

    // Initialize video buffer to store rendered frames
    videoBuffer = new uint32_t[width * height];
    
    // Start emulation
    NDS::Start();
    emulatorInitialized = true;
    
    LOGI("melonDS initialized successfully");
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_DsEmulatorBridge_nativeRunFrame(
        JNIEnv* env,
        jobject /* this */,
        jobject bitmap) {

    if (!emulatorInitialized) {
        LOGE("melonDS not initialized");
        return JNI_FALSE;
    }

    // Run a frame
    NDS::RunFrame();
    
    // Get current frame from both screens
    uint32_t* topScreen = GPU::Framebuffer[0];
    uint32_t* bottomScreen = GPU::Framebuffer[1];

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

    // Copy both DS screens to one combined bitmap
    uint32_t* dest = static_cast<uint32_t*>(pixels);
    
    // Copy top screen (256x192)
    for (int y = 0; y < 192; y++) {
        memcpy(dest + y * width, 
               topScreen + y * 256, 
               256 * sizeof(uint32_t));
    }
    
    // Copy bottom screen (256x192)
    for (int y = 0; y < 192; y++) {
        memcpy(dest + (y + 192) * width, 
               bottomScreen + y * 256, 
               256 * sizeof(uint32_t));
    }

    AndroidBitmap_unlockPixels(env, bitmap);
    
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_DsEmulatorBridge_nativeUpdateInput(
        JNIEnv* env,
        jobject /* this */,
        jint buttons,
        jint touchX,
        jint touchY) {

    if (!emulatorInitialized) {
        return;
    }

    // Map input bits to NDS buttons
    uint16_t keyInput = 0xFFFF;
    
    if (buttons & 0x001) keyInput &= ~(1 << 0);  // A
    if (buttons & 0x002) keyInput &= ~(1 << 1);  // B
    if (buttons & 0x004) keyInput &= ~(1 << 2);  // Select
    if (buttons & 0x008) keyInput &= ~(1 << 3);  // Start
    if (buttons & 0x010) keyInput &= ~(1 << 4);  // Right
    if (buttons & 0x020) keyInput &= ~(1 << 5);  // Left
    if (buttons & 0x040) keyInput &= ~(1 << 6);  // Up
    if (buttons & 0x080) keyInput &= ~(1 << 7);  // Down
    if (buttons & 0x100) keyInput &= ~(1 << 8);  // R
    if (buttons & 0x200) keyInput &= ~(1 << 9);  // L
    if (buttons & 0x400) keyInput &= ~(1 << 10); // X
    if (buttons & 0x800) keyInput &= ~(1 << 11); // Y
    
    NDS::SetKeyMask(keyInput);
    
    // Handle touch input
    bool touching = (touchX >= 0 && touchY >= 0);
    if (touching) {
        // Make sure coordinates are within the bottom screen
        touchX = std::min(255, std::max(0, touchX));
        touchY = std::min(191, std::max(0, touchY));
        
        NDS::TouchScreen(touchX, touchY);
    } else {
        NDS::ReleaseScreen();
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_DsEmulatorBridge_nativeSaveState(
        JNIEnv* env,
        jobject /* this */,
        jstring path) {

    if (!emulatorInitialized) {
        LOGE("melonDS not initialized");
        return JNI_FALSE;
    }

    const char* cPath = env->GetStringUTFChars(path, nullptr);
    LOGI("Saving state to: %s", cPath);
    
    bool success = NDS::SaveState(cPath);
    env->ReleaseStringUTFChars(path, cPath);
    
    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_DsEmulatorBridge_nativeLoadState(
        JNIEnv* env,
        jobject /* this */,
        jstring path) {

    if (!emulatorInitialized) {
        LOGE("melonDS not initialized");
        return JNI_FALSE;
    }

    const char* cPath = env->GetStringUTFChars(path, nullptr);
    LOGI("Loading state from: %s", cPath);
    
    bool success = NDS::LoadState(cPath);
    env->ReleaseStringUTFChars(path, cPath);
    
    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_DsEmulatorBridge_nativeCleanup(
        JNIEnv* env,
        jobject /* this */) {

    if (emulatorInitialized) {
        NDS::Stop();
        NDS::DeInit();
        emulatorInitialized = false;
    }

    if (videoBuffer) {
        delete[] videoBuffer;
        videoBuffer = nullptr;
    }

    LOGI("melonDS emulator cleaned up");
}