#include <jni.h>
#include <string>
#include <android/log.h>

#define LOG_TAG "FileUtils"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * Simplified file utilities implementation to get the build working
 */

// Basic implementation of directory creation
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_recreemulcream_util_FileUtils_nativeMkdir(
        JNIEnv* env,
        jobject /* this */,
        jstring dirPath) {
    
    // Just return true for now - will implement correctly later
    return JNI_TRUE;
}

// Basic implementation of file listing
extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_example_recreemulcream_util_FileUtils_nativeListFiles(
        JNIEnv* env,
        jobject /* this */,
        jstring dirPath,
        jobjectArray extensions) {
    
    // Just return an empty array for now
    return env->NewObjectArray(0, env->FindClass("java/lang/String"), 
                               env->NewStringUTF(""));
}