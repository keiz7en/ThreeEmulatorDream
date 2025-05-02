package com.example.recreemulcream.util;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.util.Log;

import androidx.documentfile.provider.DocumentFile;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

public class FileUtils {
    private static final String TAG = "FileUtils";

    // Temporary Java implementations
    public String nativeGetRealPath(String contentUriPath) {
        return contentUriPath; // Simple pass-through for now
    }

    public String[] nativeListFiles(String dirPath, String[] extensions) {
        // Temporary Java implementation
        File dir = new File(dirPath);
        List<String> results = new ArrayList<>();

        if (dir.exists() && dir.isDirectory()) {
            File[] files = dir.listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.isFile()) {
                        String name = file.getName().toLowerCase();
                        for (String ext : extensions) {
                            if (name.endsWith(ext.toLowerCase())) {
                                results.add(file.getAbsolutePath());
                                break;
                            }
                        }
                    }
                }
            }
        }

        return results.toArray(new String[0]);
    }

    public boolean nativeFileExists(String filePath) {
        return new File(filePath).exists();
    }

    public boolean nativeMkdir(String dirPath) {
        return new File(dirPath).mkdirs();
    }

    /**
     * Get a usable file path from a content URI
     * This is necessary because emulator cores need direct file access
     */
    public static String getPathFromUri(Context context, Uri uri) {
        if (uri == null) return null;

        // If already a file URI, just return the path
        if ("file".equalsIgnoreCase(uri.getScheme())) {
            return uri.getPath();
        }

        // Handle content URIs
        try {
            String filename = getFileNameFromUri(context, uri);
            File file = new File(context.getCacheDir(), "roms/" + filename);

            // Make sure the directory exists
            File dir = file.getParentFile();
            if (!dir.exists()) {
                dir.mkdirs();
            }

            // Copy content to a file we can access
            copyUriToFile(context, uri, file);
            return file.getAbsolutePath();
        } catch (Exception e) {
            Log.e(TAG, "Error getting file path from URI", e);
            return null;
        }
    }

    /**
     * Get file name from URI
     */
    public static String getFileNameFromUri(Context context, Uri uri) {
        String result = null;
        if ("content".equals(uri.getScheme())) {
            try (Cursor cursor = context.getContentResolver().query(uri, null, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                    if (nameIndex >= 0) {
                        result = cursor.getString(nameIndex);
                    }
                }
            }
        }
        if (result == null) {
            result = uri.getLastPathSegment();
        }
        return result;
    }

    /**
     * Copy content URI to a file
     */
    public static void copyUriToFile(Context context, Uri uri, File destFile) throws IOException {
        try (InputStream is = context.getContentResolver().openInputStream(uri);
             OutputStream os = new FileOutputStream(destFile)) {
            if (is == null) {
                throw new IOException("Failed to open input stream");
            }
            byte[] buffer = new byte[8192];
            int read;
            while ((read = is.read(buffer)) != -1) {
                os.write(buffer, 0, read);
            }
            os.flush();
        }
    }

    /**
     * Find ROM files in a directory and subdirectories
     */
    public static List<File> findRomFiles(String baseDir, String[] extensions) {
        List<File> results = new ArrayList<>();
        File dir = new File(baseDir);
        findRomFilesRecursive(dir, extensions, results, 0);
        return results;
    }

    private static void findRomFilesRecursive(File dir, String[] extensions, List<File> results, int depth) {
        // Limit recursion depth
        if (depth > 3) return;

        if (!dir.exists() || !dir.isDirectory()) return;

        // List files in directory
        File[] files = dir.listFiles();
        if (files == null) return;

        for (File file : files) {
            if (file.isDirectory()) {
                findRomFilesRecursive(file, extensions, results, depth + 1);
            } else {
                String name = file.getName().toLowerCase();
                for (String ext : extensions) {
                    if (name.endsWith(ext.toLowerCase())) {
                        results.add(file);
                        break;
                    }
                }
            }
        }
    }

    /**
     * Get common ROM storage directories
     */
    public static List<File> getCommonRomDirs(Context context) {
        List<File> dirs = new ArrayList<>();

        // Add external storage directories
        File externalDir = Environment.getExternalStorageDirectory();
        dirs.add(externalDir);

        // Add common ROM directories
        dirs.add(new File(externalDir, "ROMs"));
        dirs.add(new File(externalDir, "roms"));
        dirs.add(new File(externalDir, "Emulation"));
        dirs.add(new File(externalDir, "emulation"));
        dirs.add(new File(externalDir, "Download"));
        dirs.add(new File(context.getExternalFilesDir(null), "roms"));

        return dirs;
    }

    /**
     * Get file extension
     */
    public static String getFileExtension(String filePath) {
        int lastDot = filePath.lastIndexOf('.');
        if (lastDot >= 0) {
            return filePath.substring(lastDot);
        }
        return "";
    }
}