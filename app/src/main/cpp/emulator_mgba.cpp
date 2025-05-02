#include <jni.h>
#include <string>
#include <cstring>
#include <android/log.h>
#include <android/bitmap.h>

// Simple mgba-core includes
extern "C" {
    #include "mgba/core/core.h"
    #include "mgba/core/thread.h"
    #include "mgba/gba/core.h"
    #include "mgba/internal/gba/memory.h"
    #include "mgba/util/vfs.h"
}

#define LOG_TAG "EmulatorMgba"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global variables for mGBA emulation
static struct mCore* core = nullptr;
static uint32_t* videoBuffer = nullptr;
static size_t videoBufferSize = 0;
static int width = 240;
static int height = 160;

// GBA button mapping
static const uint16_t GBA_KEY_A      = 1 << 0;
static const uint16_t GBA_KEY_B      = 1 << 1;
static const uint16_t GBA_KEY_SELECT = 1 << 2;
static const uint16_t GBA_KEY_START  = 1 << 3;
static const uint16_t GBA_KEY_RIGHT  = 1 << 4;
static const uint16_t GBA_KEY_LEFT   = 1 << 5;
static const uint16_t GBA_KEY_UP     = 1 << 6;
static const uint16_t GBA_KEY_DOWN   = 1 << 7;
static const uint16_t GBA_KEY_R      = 1 << 8;
static const uint16_t GBA_KEY_L      = 1 << 9;

/**
 * Initialize the GBA emulator core with a ROM file
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_GbaEmulatorBridge_nativeInitialize(
        JNIEnv* env,
        jobject /* this */,
        jstring romPath) {

    // Clean up any existing core
    if (core) {
        core->deinit(core);
        free(core);
        core = nullptr;
    }

    // Free any existing video buffer
    if (videoBuffer) {
        delete[] videoBuffer;
        videoBuffer = nullptr;
    }

    // Convert jstring to C string
    const char* cRomPath = env->GetStringUTFChars(romPath, nullptr);
    LOGI("Initializing mGBA with ROM: %s", cRomPath);

    // Initialize the core
    core = GBACoreCreate();
    if (!core) {
        LOGE("Failed to create mGBA core");
        env->ReleaseStringUTFChars(romPath, cRomPath);
        return JNI_FALSE;
    }

    // Open ROM file using the VFS
    struct VFile* rom = VFileOpen(cRomPath, O_RDONLY);
    if (!rom) {
        LOGE("Failed to open ROM file: %s", cRomPath);
        env->ReleaseStringUTFChars(romPath, cRomPath);
        core->deinit(core);
        free(core);
        core = nullptr;
        return JNI_FALSE;
    }

    // Load ROM into the core
    if (!core->loadROM(core, rom)) {
        LOGE("Failed to load ROM: %s", cRomPath);
        rom->close(rom);
        core->deinit(core);
        free(core);
        core = nullptr;
        env->ReleaseStringUTFChars(romPath, cRomPath);
        return JNI_FALSE;
    }

    env->ReleaseStringUTFChars(romPath, cRomPath);

    // Set up video dimensions
    width = core->width(core);
    height = core->height(core);
    videoBufferSize = width * height * sizeof(uint32_t);
    
    // Allocate video buffer
    videoBuffer = new uint32_t[width * height];
    memset(videoBuffer, 0, videoBufferSize);
    
    // Set video buffer in the core
    core->setVideoBuffer(core, videoBuffer, width);

    // Initialize and start the core
    core->reset(core);
    
    LOGI("mGBA initialized successfully with dimensions %dx%d", width, height);
    return JNI_TRUE;
}

/**
 * Run a single frame of the emulation
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_GbaEmulatorBridge_nativeRunFrame(
        JNIEnv* env,
        jobject /* this */,
        jobject bitmap) {
            
    if (!core || !videoBuffer) {
        LOGE("Core not initialized or video buffer is null");
        return JNI_FALSE;
    }
    
    // Run a single frame
    core->runFrame(core);
    
    // Lock the Android bitmap
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGE("Failed to get bitmap info");
        return JNI_FALSE;
    }
    
    // Check bitmap format
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format not RGBA_8888");
        return JNI_FALSE;
    }
    
    // Make sure dimensions match
    if (info.width < width || info.height < height) {
        LOGE("Bitmap dimensions too small: %dx%d, need %dx%d", 
            info.width, info.height, width, height);
        return JNI_FALSE;
    }
    
    // Lock bitmap pixels
    void* pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        LOGE("Failed to lock bitmap pixels");
        return JNI_FALSE;
    }
    
    // Copy the video buffer to the bitmap
    uint32_t* src = videoBuffer;
    uint32_t* dst = (uint32_t*)pixels;
    
    for (int y = 0; y < height; y++) {
        memcpy(dst + y * info.stride / sizeof(uint32_t), 
               src + y * width, 
               width * sizeof(uint32_t));
    }
    
    AndroidBitmap_unlockPixels(env, bitmap);
    return JNI_TRUE;
}

/**
 * Process controller input
 */
extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_GbaEmulatorBridge_nativeUpdateInput(
        JNIEnv* env,
        jobject /* this */,
        jint buttonsState) {
            
    if (!core) {
        return;
    }
    
    // Apply the button mask directly to the core
    core->setKeys(core, buttonsState);
}

/**
 * Save game state
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_GbaEmulatorBridge_nativeSaveState(
        JNIEnv* env,
        jobject /* this */,
        jstring path) {
            
    if (!core) {
        LOGE("Core not initialized");
        return JNI_FALSE;
    }
    
    const char* cPath = env->GetStringUTFChars(path, nullptr);
    LOGI("Saving state to: %s", cPath);
    
    struct VFile* vf = VFileOpen(cPath, O_WRONLY | O_CREAT | O_TRUNC);
    
    bool success = false;
    if (vf) {
        success = core->saveState(core, vf);
        vf->close(vf);
    }
    
    env->ReleaseStringUTFChars(path, cPath);
    return success ? JNI_TRUE : JNI_FALSE;
}

/**
 * Load game state
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_GbaEmulatorBridge_nativeLoadState(
        JNIEnv* env,
        jobject /* this */,
        jstring path) {
            
    if (!core) {
        LOGE("Core not initialized");
        return JNI_FALSE;
    }
    
    const char* cPath = env->GetStringUTFChars(path, nullptr);
    LOGI("Loading state from: %s", cPath);
    
    struct VFile* vf = VFileOpen(cPath, O_RDONLY);
    
    bool success = false;
    if (vf) {
        success = core->loadState(core, vf);
        vf->close(vf);
    }
    
    env->ReleaseStringUTFChars(path, cPath);
    return success ? JNI_TRUE : JNI_FALSE;
}

/**
 * Clean up resources
 */
extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_GbaEmulatorBridge_nativeCleanup(
        JNIEnv* env,
        jobject /* this */) {
            
    if (core) {
        core->deinit(core);
        free(core);
        core = nullptr;
    }
    
    if (videoBuffer) {
        delete[] videoBuffer;
        videoBuffer = nullptr;
    }
    
    LOGI("mGBA emulator cleaned up");
}