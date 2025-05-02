package com.example.recreemulcream.util;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import java.io.File;

/**
 * Utility class to check for emulator core availability and system requirements
 */
public class EmulatorStatusChecker {

    // Native methods to check emulator core status
    public boolean checkEmulatorAvailability(String emulatorType) {
        try {
            return nativeCheckEmulatorAvailability(emulatorType);
        } catch (UnsatisfiedLinkError e) {
            Log.e("EmulatorStatusChecker", "Failed to check emulator availability: " + e.getMessage());
            // For now, all emulator types are available in Java mode
            return "gba".equals(emulatorType) || "snes".equals(emulatorType) || "ds".equals(emulatorType);
        }
    }

    public String getEmulatorVersionInfo(String emulatorType) {
        try {
            return nativeGetEmulatorVersionInfo(emulatorType);
        } catch (UnsatisfiedLinkError e) {
            Log.e("EmulatorStatusChecker", "Failed to get emulator version: " + e.getMessage());
            if ("gba".equals(emulatorType)) {
                return "mGBA Core (Java fallback)";
            } else if ("snes".equals(emulatorType)) {
                return "SNES9x Core (Java fallback)";
            } else if ("ds".equals(emulatorType)) {
                return "melonDS Core (Java fallback)";
            }
            return "Emulator not available (Java fallback)";
        }
    }

    // Native method declarations
    private native boolean nativeCheckEmulatorAvailability(String emulatorType);

    private native String nativeGetEmulatorVersionInfo(String emulatorType);

    // Load native library
    static {
        try {
            System.loadLibrary("recreemulcream");
        } catch (UnsatisfiedLinkError e) {
            Log.e("EmulatorStatusChecker", "Failed to load native library: " + e.getMessage());
        }
    }

    /**
     * Check if the device meets the minimum requirements for emulation
     */
    public static boolean deviceMeetsRequirements(Context context) {
        // Check Android version (Android 6.0+)
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return false;
        }

        // Check available memory
        ActivityManager activityManager =
                (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
        activityManager.getMemoryInfo(memInfo);

        // Minimum 2GB RAM requirement
        long totalMem = memInfo.totalMem;
        long minRequiredMem = 2L * 1024 * 1024 * 1024; // 2GB in bytes

        return totalMem >= minRequiredMem;
    }

    /**
     * Check if specific emulator system meets requirements
     */
    public static boolean emulatorMeetsRequirements(Context context, String emulatorType) {
        // General device check
        if (!deviceMeetsRequirements(context)) {
            return false;
        }

        // Specific checks for emulator types
        switch (emulatorType) {
            case "gba":
                // GBA emulation is usually lightweight
                return true;

            case "snes":
                // SNES emulation is also fairly lightweight
                return true;

            case "ds":
                // Check for additional requirements for DS emulation
                ActivityManager activityManager =
                        (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
                ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
                activityManager.getMemoryInfo(memInfo);

                // DS emulation needs at least 3GB RAM
                long dsMinRequiredMem = 3L * 1024 * 1024 * 1024; // 3GB
                return memInfo.totalMem >= dsMinRequiredMem;

            default:
                return false;
        }
    }

    /**
     * Check if the required BIOS files exist for the given emulator
     */
    public static boolean checkRequiredBiosFiles(Context context, String emulatorType) {
        // Only DS emulation requires BIOS files
        if ("ds".equals(emulatorType)) {
            File biosDir = new File(context.getFilesDir(), "bios");
            File bios7File = new File(biosDir, "bios7.bin");
            File bios9File = new File(biosDir, "bios9.bin");
            File firmwareFile = new File(biosDir, "firmware.bin");

            return bios7File.exists() && bios9File.exists() && firmwareFile.exists();
        }

        // GBA and SNES don't require BIOS files
        return true;
    }
}