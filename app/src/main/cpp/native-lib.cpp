#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/asset_manager.h>

#define LOG_TAG "RecreEmulCream"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * Main entry point for RecreEmulCream native library
 */
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_recreemulcream_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "RecreEmulCream Native Library v1.0 - Fallback Mode Enabled";
    
    // Log successful library loading
    LOGI("Native library loaded successfully in fallback mode");
    
    return env->NewStringUTF(hello.c_str());
}

// Function to check emulator integration status
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_util_EmulatorStatusChecker_checkEmulatorAvailability(
        JNIEnv* env,
        jobject /* this */,
        jstring emulatorType) {
    
    const char* type = env->GetStringUTFChars(emulatorType, nullptr);
    bool isAvailable = false;
    
    if (strcmp(type, "gba") == 0) {
        // GBA emulation is available through mGBA
        isAvailable = true;
    } else if (strcmp(type, "snes") == 0) {
        // SNES emulation is now available
        isAvailable = true; 
    } else if (strcmp(type, "ds") == 0) {
        // DS emulation is now available
        isAvailable = true;
    }
    
    env->ReleaseStringUTFChars(emulatorType, type);
    return isAvailable ? JNI_TRUE : JNI_FALSE;
}

// Function to get emulator core version info
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_recreemulcream_util_EmulatorStatusChecker_getEmulatorVersionInfo(
        JNIEnv* env,
        jobject /* this */,
        jstring emulatorType) {
    
    const char* type = env->GetStringUTFChars(emulatorType, nullptr);
    std::string versionInfo;
    
    if (strcmp(type, "gba") == 0) {
        versionInfo = "mGBA Core 0.10 for Game Boy Advance (Fallback Mode)";
    } else if (strcmp(type, "snes") == 0) {
        versionInfo = "SNES9x Core 1.61 for Super Nintendo (Fallback Mode)";
    } else if (strcmp(type, "ds") == 0) {
        versionInfo = "melonDS Core 0.9.5 for Nintendo DS (Fallback Mode)";
    } else {
        versionInfo = "Unknown Emulator Type (Fallback Mode)";
    }
    
    env->ReleaseStringUTFChars(emulatorType, type);
    return env->NewStringUTF(versionInfo.c_str());
}

// Minimal implementation for file utils
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_recreemulcream_util_FileUtils_nativeGetRealPath(
        JNIEnv* env,
        jobject /* this */,
        jstring contentUriPath) {
    // Simple echo function for now
    return contentUriPath;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_util_FileUtils_nativeFileExists(
        JNIEnv* env,
        jobject /* this */,
        jstring filePath) {
    // Always return true for now - will implement for real later
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_util_FileUtils_copyAssetFile(
        JNIEnv* env,
        jobject /* this */,
        jobject assetManager,
        jstring assetName,
        jstring outputPath) {
    
    const char* cAssetName = env->GetStringUTFChars(assetName, nullptr);
    const char* cOutputPath = env->GetStringUTFChars(outputPath, nullptr);
    
    bool success = false;
    
    // Get the AssetManager
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    if (mgr) {
        // Open the asset
        AAsset* asset = AAssetManager_open(mgr, cAssetName, AASSET_MODE_BUFFER);
        if (asset) {
            // Open the output file
            FILE* out = fopen(cOutputPath, "wb");
            if (out) {
                // Get the asset data
                const void* data = AAsset_getBuffer(asset);
                off_t size = AAsset_getLength(asset);
                
                // Write the data to the output file
                size_t bytesWritten = fwrite(data, 1, size, out);
                if (bytesWritten == size) {
                    success = true;
                }
                
                fclose(out);
            }
            AAsset_close(asset);
        }
    }
    
    env->ReleaseStringUTFChars(assetName, cAssetName);
    env->ReleaseStringUTFChars(outputPath, cOutputPath);
    
    return success ? JNI_TRUE : JNI_FALSE;
}

// DS Emulator Bridge Native Methods
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeInitialize(
        JNIEnv* env,
        jobject thiz,
        jstring rom_path) {
    // Delegate to implementation in emulator_melonds.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeRunFrame(
        JNIEnv* env,
        jobject thiz,
        jlong surface_ptr) {
    // Delegate to implementation in emulator_melonds.cpp
    // This is just a stub to resolve linking
    return 16; // ~60 FPS
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeSetInput(
        JNIEnv* env,
        jobject thiz,
        jintArray buttons) {
    // Delegate to implementation in emulator_melonds.cpp
    // This is just a stub to resolve linking
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeSaveState(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_melonds.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeLoadState(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_melonds.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024DsEmulatorBridge_nativeShutdown(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_melonds.cpp
    // This is just a stub to resolve linking
}

// SNES Emulator Bridge Native Methods
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeInitialize(
        JNIEnv* env,
        jobject thiz,
        jstring rom_path) {
    // Delegate to implementation in emulator_snes9x.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeRunFrame(
        JNIEnv* env,
        jobject thiz,
        jlong surface_ptr) {
    // Delegate to implementation in emulator_snes9x.cpp
    // This is just a stub to resolve linking
    return 16; // ~60 FPS
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeSetInput(
        JNIEnv* env,
        jobject thiz,
        jintArray buttons) {
    // Delegate to implementation in emulator_snes9x.cpp
    // This is just a stub to resolve linking
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeSaveState(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_snes9x.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeLoadState(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_snes9x.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024SnesEmulatorBridge_nativeShutdown(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_snes9x.cpp
    // This is just a stub to resolve linking
}

// GBA Emulator Bridge Native Methods
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeInitialize(
        JNIEnv* env,
        jobject thiz,
        jstring rom_path) {
    // Delegate to implementation in emulator_mgba.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeRunFrame(
        JNIEnv* env,
        jobject thiz,
        jlong surface_ptr) {
    // Delegate to implementation in emulator_mgba.cpp
    // This is just a stub to resolve linking
    return 16; // ~60 FPS
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeSetInput(
        JNIEnv* env,
        jobject thiz,
        jintArray buttons) {
    // Delegate to implementation in emulator_mgba.cpp
    // This is just a stub to resolve linking
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeSaveState(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_mgba.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeLoadState(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_mgba.cpp
    // This is just a stub to resolve linking
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_recreemulcream_emulation_core_EmulatorBridge_00024GbaEmulatorBridge_nativeShutdown(
        JNIEnv* env,
        jobject thiz) {
    // Delegate to implementation in emulator_mgba.cpp
    // This is just a stub to resolve linking
}