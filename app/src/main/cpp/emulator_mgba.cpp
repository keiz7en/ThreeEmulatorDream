#include <jni.h>
#include <string>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>

// mGBA headers
#ifdef __has_include
#if __has_include("mgba/core/core.h") && !defined(ENABLE_FALLBACK_MODE)
#define HAS_MGBA_HEADERS 1
#else
#define HAS_MGBA_HEADERS 0
#endif
#else
#define HAS_MGBA_HEADERS 0
#endif

#if HAS_MGBA_HEADERS
// Use the real headers
#include <mgba/core/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
#include <mgba/gba/interface.h>
#include <mgba/gba/gba.h>
#include <mgba/gba/renderers/software.h>
#include <mgba-util/vfs.h>
#include <mgba-util/memory.h>
// Uncomment for audio support
// #include <mgba/core/thread.h>
// #include <mgba/core/input.h>
#else
// Forward declarations for stub implementation
struct mCore {
    bool (*init)(struct mCore*);
    void (*deinit)(struct mCore*);
    void (*reset)(struct mCore*);
    void (*runFrame)(struct mCore*);
    bool (*loadROM)(struct mCore*, void*);
    void (*getPixels)(struct mCore*, void*, size_t);
    void (*setKeys)(struct mCore*, uint16_t);
    bool (*saveState)(struct mCore*, void*);
    bool (*loadState)(struct mCore*, void*);
    
    struct mCoreConfig {
        void* dummy;
    } config;
};

struct VFile {
    void (*close)(struct VFile*);
};

// Stub functions for when the actual library is not available
static inline struct mCore* GBACoreCreate(void) { return NULL; }
static inline bool mCoreInitConfig(struct mCore* core, void* unused) { return false; }
static inline bool mCoreConfigSetIntValue(struct mCoreConfig* config, const char* key, int value) { return false; }
static inline struct VFile* VFileOpen(const char* path, int mode) { return NULL; }
static inline struct VFile* VFileMemChunk(void* unused, size_t unused2) { return NULL; }
static inline void coreDeinit(struct mCore* core) {}
#endif

// Define button codes
#define GBA_KEY_A 0
#define GBA_KEY_B 1
#define GBA_KEY_SELECT 2
#define GBA_KEY_START 3
#define GBA_KEY_RIGHT 4
#define GBA_KEY_LEFT 5
#define GBA_KEY_UP 6
#define GBA_KEY_DOWN 7
#define GBA_KEY_R 8
#define GBA_KEY_L 9

#define TAG "EmulatorMGBA"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// Global state
static struct mCore* core = NULL;
static bool core_initialized = false;

// Input state
static uint16_t input_state = 0;

// Screen buffer
static uint32_t* video_buffer = NULL;
static size_t video_buffer_size = 0;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeInitialize(
        JNIEnv* env, jobject thiz, jstring rom_path) {
    
    // Cleanup previous core if any
    if (core) {
        #if HAS_MGBA_HEADERS
        core->deinit(core);
        coreDeinit(core);
        #endif
        core = NULL;
        core_initialized = false;
    }
    
    const char* path = env->GetStringUTFChars(rom_path, NULL);
    LOGI("Initializing GBA emulator with ROM: %s (fallback mode)", path);
    
    #if HAS_MGBA_HEADERS
    // Create mCore instance
    core = GBACoreCreate();
    if (!core) {
        LOGE("Failed to create mGBA core");
        env->ReleaseStringUTFChars(rom_path, path);
        return JNI_FALSE;
    }
    
    // Initialize core
    core->init(core);
    mCoreInitConfig(core, NULL);
    
    // Set video dimensions
    mCoreConfigSetIntValue(&core->config, "width", 240);
    mCoreConfigSetIntValue(&core->config, "height", 160);
    
    // Open ROM
    struct VFile* rom = VFileOpen(path, O_RDONLY);
    env->ReleaseStringUTFChars(rom_path, path);
    
    if (!rom) {
        LOGE("Failed to open ROM file");
        core->deinit(core);
        coreDeinit(core);
        core = NULL;
        return JNI_FALSE;
    }
    
    if (!core->loadROM(core, rom)) {
        LOGE("Failed to load ROM");
        rom->close(rom);
        core->deinit(core);
        coreDeinit(core);
        core = NULL;
        return JNI_FALSE;
    }
    
    // Create video buffer
    video_buffer_size = 240 * 160 * sizeof(uint32_t);
    video_buffer = (uint32_t*)malloc(video_buffer_size);
    if (!video_buffer) {
        LOGE("Failed to allocate video buffer");
        rom->close(rom);
        core->deinit(core);
        coreDeinit(core);
        core = NULL;
        return JNI_FALSE;
    }
    
    // Reset core
    core->reset(core);
    core_initialized = true;
    
    LOGI("mGBA initialized successfully");
    return JNI_TRUE;
    #else
    env->ReleaseStringUTFChars(rom_path, path);
    
    // Extract ROM filename for display
    std::string romName = path;
    size_t lastSlash = romName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        romName = romName.substr(lastSlash + 1);
    }
    
    // Create a dummy video buffer for fallback rendering
    video_buffer_size = 240 * 160 * sizeof(uint32_t);
    video_buffer = (uint32_t*)malloc(video_buffer_size);
    if (!video_buffer) {
        LOGE("Failed to allocate video buffer for fallback mode");
        return JNI_FALSE;
    }
    
    // Create an attractive GBA-like display with ROM name
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 240; x++) {
            // Create a gradient background
            uint8_t r = 32 + (x * 192 / 240);
            uint8_t g = 96 + (y * 64 / 160);
            uint8_t b = 164 - (y * 32 / 160);
            
            video_buffer[y * 240 + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            
            // Add a border
            if (x < 4 || x >= 236 || y < 4 || y >= 156) {
                video_buffer[y * 240 + x] = 0xFF111111; // Dark border
            }
        }
    }
    
    // Draw "GAME BOY ADVANCE" text
    const char* gbaText = "GAME BOY ADVANCE";
    int textX = 50;
    int textY = 30;
    
    for (int i = 0; i < strlen(gbaText); i++) {
        for (int py = 0; py < 10; py++) {
            for (int px = 0; px < 8; px++) {
                int screenX = textX + (i * 9) + px;
                int screenY = textY + py;
                
                if (screenX >= 0 && screenX < 240 && screenY >= 0 && screenY < 160) {
                    video_buffer[screenY * 240 + screenX] = 0xFFFFFFFF; // White text
                }
            }
        }
    }
    
    // Draw ROM name
    textX = 30;
    textY = 80;
    
    const char* romText = romName.c_str();
    int maxLen = std::min((int)romName.length(), 20); // Limit to 20 chars
    
    for (int i = 0; i < maxLen; i++) {
        for (int py = 0; py < 8; py++) {
            for (int px = 0; px < 6; px++) {
                int screenX = textX + (i * 7) + px;
                int screenY = textY + py;
                
                if (screenX >= 0 && screenX < 240 && screenY >= 0 && screenY < 160) {
                    video_buffer[screenY * 240 + screenX] = 0xFFFFFF00; // Yellow text
                }
            }
        }
    }
    
    // Draw "FALLBACK MODE" text
    const char* fbText = "FALLBACK MODE";
    textX = 65;
    textY = 120;
    
    for (int i = 0; i < strlen(fbText); i++) {
        for (int py = 0; py < 8; py++) {
            for (int px = 0; px < 6; px++) {
                int screenX = textX + (i * 7) + px;
                int screenY = textY + py;
                
                if (screenX >= 0 && screenX < 240 && screenY >= 0 && screenY < 160) {
                    video_buffer[screenY * 240 + screenX] = 0xFFFF5555; // Light red text
                }
            }
        }
    }
    
    core_initialized = true;
    LOGI("GBA emulator initialized in fallback mode");
    return JNI_TRUE;
    #endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeRunFrame(
        JNIEnv* env, jobject thiz, jobject bitmap) {
    
    if (!core_initialized || !video_buffer) {
        LOGE("Core not initialized");
        return JNI_FALSE;
    }
    
    #if HAS_MGBA_HEADERS
    // Run one frame
    core->setKeys(core, input_state);
    core->runFrame(core);
    
    // Get video buffer from core
    core->getPixels(core, video_buffer, video_buffer_size);
    #else
    // Static counter for animation
    static int frame_counter = 0;
    frame_counter++;
    
    // In fallback mode, create an animated pattern
    if (frame_counter % 120 == 0) { // Every 2 seconds at 60fps
        // Update background with a subtle animation
        for (int y = 0; y < 160; y++) {
            for (int x = 0; x < 240; x++) {
                if (x >= 4 && x < 236 && y >= 4 && y < 156) { // Skip the border
                    // Create an animated gradient background
                    uint8_t phase = (frame_counter / 120) % 6;
                    uint8_t r = 32 + (x * 192 / 240);
                    uint8_t g = 96 + (y * 64 / 160);
                    uint8_t b = 164 - (y * 32 / 160);
                    
                    // Shift the colors based on animation phase
                    switch (phase) {
                        case 0: break; // Original colors
                        case 1: std::swap(r, g); break;
                        case 2: std::swap(g, b); break;
                        case 3: std::swap(r, b); break;
                        case 4: r = g; g = b; b = r; break;
                        case 5: r = b; b = g; g = r; break;
                    }
                    
                    video_buffer[y * 240 + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
        }
    }
    
    // Draw a bouncing dot to show it's not frozen
    int dotX = 120 + (int)(60 * sin(frame_counter * 0.05));
    int dotY = 140 + (int)(10 * sin(frame_counter * 0.1));
    
    // Draw a 4x4 white dot
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            int px = dotX + x;
            int py = dotY + y;
            if (px >= 4 && px < 236 && py >= 4 && py < 156) {
                video_buffer[py * 240 + px] = 0xFFFFFFFF;
            }
        }
    }
    #endif
    
    // Lock bitmap pixels for direct writing
    AndroidBitmapInfo info;
    void* pixels;
    
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGE("Failed to get bitmap info");
        return JNI_FALSE;
    }
    
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format not RGBA_8888");
        return JNI_FALSE;
    }
    
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        LOGE("Failed to lock bitmap pixels");
        return JNI_FALSE;
    }
    
    // Copy GBA buffer directly to the bitmap
    uint32_t* dst = (uint32_t*)pixels;
    int dst_width = info.width;
    int dst_height = info.height;
    
    if (dst_width == 240 && dst_height == 160) {
        // Perfect match - direct copy
        memcpy(dst, video_buffer, video_buffer_size);
    } else {
        // Need to scale - simple nearest-neighbor scaling
        for (int y = 0; y < dst_height; y++) {
            int src_y = y * 160 / dst_height;
            for (int x = 0; x < dst_width; x++) {
                int src_x = x * 240 / dst_width;
                dst[y * dst_width + x] = video_buffer[src_y * 240 + src_x];
            }
        }
    }
    
    AndroidBitmap_unlockPixels(env, bitmap);
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeUpdateInput(
        JNIEnv* env, jobject thiz, jint buttonMask) {
    
    if (!core_initialized) return;
    
    // Map button mask directly to mGBA keys
    input_state = 0;
    
    if (buttonMask & (1 << 0)) input_state |= 1 << GBA_KEY_A;     // A
    if (buttonMask & (1 << 1)) input_state |= 1 << GBA_KEY_B;     // B
    if (buttonMask & (1 << 2)) input_state |= 1 << GBA_KEY_SELECT; // Select
    if (buttonMask & (1 << 3)) input_state |= 1 << GBA_KEY_START;  // Start
    if (buttonMask & (1 << 4)) input_state |= 1 << GBA_KEY_RIGHT; // Right
    if (buttonMask & (1 << 5)) input_state |= 1 << GBA_KEY_LEFT;  // Left
    if (buttonMask & (1 << 6)) input_state |= 1 << GBA_KEY_UP;    // Up
    if (buttonMask & (1 << 7)) input_state |= 1 << GBA_KEY_DOWN;  // Down
    if (buttonMask & (1 << 8)) input_state |= 1 << GBA_KEY_R;     // R
    if (buttonMask & (1 << 9)) input_state |= 1 << GBA_KEY_L;     // L
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeSaveState(
        JNIEnv* env, jobject thiz, jstring path) {
    
    if (!core_initialized) return JNI_FALSE;
    
    #if HAS_MGBA_HEADERS
    const char* save_path = env->GetStringUTFChars(path, NULL);
    struct VFile* state = VFileOpen(save_path, O_WRONLY | O_CREAT | O_TRUNC);
    env->ReleaseStringUTFChars(path, save_path);
    
    if (!state) {
        LOGE("Failed to create file for save state");
        return JNI_FALSE;
    }
    
    if (!core->saveState(core, state)) {
        LOGE("Failed to save state");
        state->close(state);
        return JNI_FALSE;
    }
    
    state->close(state);
    return JNI_TRUE;
    #else
    LOGE("mGBA headers not found");
    return JNI_FALSE;
    #endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeLoadState(
        JNIEnv* env, jobject thiz) {
    
    if (!core_initialized) return JNI_FALSE;
    
    #if HAS_MGBA_HEADERS
    struct VFile* state = VFileMemChunk(NULL, 0);
    if (!state) {
        LOGE("Failed to create memory file for load state");
        return JNI_FALSE;
    }
    
    if (!core->loadState(core, state)) {
        LOGE("Failed to load state");
        state->close(state);
        return JNI_FALSE;
    }
    
    state->close(state);
    return JNI_TRUE;
    #else
    LOGE("mGBA headers not found");
    return JNI_FALSE;
    #endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeShutdown(
        JNIEnv* env, jobject thiz) {
    
    if (core_initialized && core) {
        #if HAS_MGBA_HEADERS
        core->deinit(core);
        coreDeinit(core);
        #endif
        core = NULL;
        core_initialized = false;
    }
    
    if (video_buffer) {
        free(video_buffer);
        video_buffer = NULL;
    }
    
    LOGI("mGBA emulator shut down");
}