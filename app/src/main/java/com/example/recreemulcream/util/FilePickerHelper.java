package com.example.recreemulcream.util;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.webkit.MimeTypeMap;
import android.widget.Toast;

import androidx.documentfile.provider.DocumentFile;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

public class FilePickerHelper {
    private static final int FILE_PICKER_REQUEST_CODE = 1010;
    private static String selectedFilePath = null;
    private static final ExecutorService executor = Executors.newSingleThreadExecutor();
    private static Future<?> romScanTask = null;

    /**
     * Open system file picker to select ROM files
     *
     * @param activity       The activity context
     * @param fileExtensions Array of file extensions to filter (e.g. ".nes", ".gba")
     */
    public static void pickRomFile(Activity activity, String[] fileExtensions) {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");

        if (fileExtensions != null && fileExtensions.length > 0) {
            // Create a MIME type filter for the specific file extensions
            String[] mimeTypes = new String[fileExtensions.length];
            for (int i = 0; i < fileExtensions.length; i++) {
                String extension = fileExtensions[i].replace(".", "");
                mimeTypes[i] = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
            }

            // Filter out any null MIME types
            List<String> validMimeTypes = new ArrayList<>();
            for (String mimeType : mimeTypes) {
                if (mimeType != null) {
                    validMimeTypes.add(mimeType);
                }
            }

            // If we couldn't find proper MIME types, use octet-stream
            if (validMimeTypes.isEmpty()) {
                validMimeTypes.add("application/octet-stream");
            }

            intent.putExtra(Intent.EXTRA_MIME_TYPES, validMimeTypes.toArray(new String[0]));
        }

        activity.startActivityForResult(intent, FILE_PICKER_REQUEST_CODE);
    }

    /**
     * Handle the result from file picker
     *
     * @return true if this was a file picker result and it was handled
     */
    public static boolean handleFilePickerResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == FILE_PICKER_REQUEST_CODE && resultCode == Activity.RESULT_OK) {
            if (data != null && data.getData() != null) {
                Uri uri = data.getData();
                // Store the URI string for now, it will be converted to a file path when needed
                selectedFilePath = uri.toString();
                return true;
            }
        }
        return false;
    }

    /**
     * Get the selected file path. This converts the URI to a file path if needed.
     */
    public static String getSelectedFilePath() {
        return selectedFilePath;
    }

    /**
     * Get a usable file path from the selected URI
     */
    public static String getUsableFilePath(Context context) {
        if (selectedFilePath == null) return null;

        try {
            Uri uri = Uri.parse(selectedFilePath);
            return FileUtils.getPathFromUri(context, uri);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Scan for ROM files in common locations
     *
     * @param context    Context
     * @param extensions ROM file extensions to look for
     * @param callback   Callback to be called when a ROM is found
     */
    public static void scanForRoms(Context context, String[] extensions, RomScanCallback callback) {
        // Cancel any running scan task
        if (romScanTask != null && !romScanTask.isDone()) {
            romScanTask.cancel(true);
        }

        romScanTask = executor.submit(() -> {
            try {
                // Get common ROM directories
                List<File> romDirs = FileUtils.getCommonRomDirs(context);

                // Scan each directory for matching ROM files
                for (File dir : romDirs) {
                    if (!dir.exists() || !dir.isDirectory() || Thread.currentThread().isInterrupted()) {
                        continue;
                    }

                    List<File> romFiles = FileUtils.findRomFiles(dir.getAbsolutePath(), extensions);
                    for (File rom : romFiles) {
                        if (Thread.currentThread().isInterrupted()) {
                            break;
                        }

                        callback.onRomFound(rom);
                    }
                }

                callback.onScanComplete();
            } catch (Exception e) {
                e.printStackTrace();
                callback.onScanError(e.getMessage());
            }
        });
    }

    /**
     * Cancel any ongoing ROM scan
     */
    public static void cancelRomScan() {
        if (romScanTask != null && !romScanTask.isDone()) {
            romScanTask.cancel(true);
        }
    }

    /**
     * Interface for ROM scan callbacks
     */
    public interface RomScanCallback {
        void onRomFound(File romFile);

        void onScanComplete();

        void onScanError(String error);
    }
}