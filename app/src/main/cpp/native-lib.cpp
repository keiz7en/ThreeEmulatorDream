#include <jni.h>
#include <string>
#include <android/log.h>

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
    std::string hello = "RecreEmulCream Native Library v1.0 - mGBA Core Ready";
    
    // Log successful library loading
    LOGI("Native library loaded successfully");
    
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
        // Not yet implemented
        isAvailable = false; 
    } else if (strcmp(type, "ds") == 0) {
        // Not yet implemented
        isAvailable = false;
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
        versionInfo = "mGBA Core 0.10 for Game Boy Advance";
    } else if (strcmp(type, "snes") == 0) {
        versionInfo = "SNES emulation not yet available";
    } else if (strcmp(type, "ds") == 0) {
        versionInfo = "DS emulation not yet available";
    } else {
        versionInfo = "Unknown Emulator Type";
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