package com.example.recreemulcream.emulation.core;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.Log;
import android.view.SurfaceHolder;

import com.example.recreemulcream.emulation.input.InputHandler;

import java.io.File;

/**
 * Bridge between Java and native mGBA emulator code
 */
public class GbaEmulatorBridge implements EmulatorBridge {
    private static final String TAG = "GbaEmulatorBridge";

    private final Context context;
    private InputHandler inputHandler;
    private boolean isInitialized = false;
    private String romPath;
    private String saveDir;
    private Bitmap frameBuffer;
    private int frameWidth = 240;
    private int frameHeight = 160;

    // Load native library
    static {
        try {
            System.loadLibrary("recreemulcream");
            Log.d(TAG, "Native GBA library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library: " + e.getMessage());
        }
    }

    public GbaEmulatorBridge(Context context) {
        this.context = context;
        this.saveDir = new File(context.getFilesDir(), "saves/gba").getAbsolutePath();

        // Create save directory
        File saveDirFile = new File(saveDir);
        if (!saveDirFile.exists()) {
            saveDirFile.mkdirs();
        }

        // Create framebuffer for GBA (240x160)
        frameBuffer = Bitmap.createBitmap(frameWidth, frameHeight, Bitmap.Config.ARGB_8888);
    }

    @Override
    public boolean initializeEmulator(String romPath) {
        Log.d(TAG, "Initializing GBA emulator with ROM: " + romPath);
        this.romPath = romPath;

        try {
            isInitialized = nativeInitialize(romPath);
            return isInitialized;
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to initialize GBA emulator: " + e.getMessage());
            return false;
        }
    }

    @Override
    public int runFrame(SurfaceHolder holder) {
        if (!isInitialized || holder == null) {
            return 0;
        }

        try {
            // Have native code render to our bitmap
            boolean frameRendered = nativeRunFrame(frameBuffer);
            if (!frameRendered) {
                return 0;
            }

            // Draw the framebuffer to the surface
            Canvas canvas = holder.lockCanvas();
            if (canvas != null) {
                // Scale to fit the surface while preserving aspect ratio
                float surfaceWidth = canvas.getWidth();
                float surfaceHeight = canvas.getHeight();

                float scaleX = surfaceWidth / frameWidth;
                float scaleY = surfaceHeight / frameHeight;
                float scale = Math.min(scaleX, scaleY);

                float scaledWidth = frameWidth * scale;
                float scaledHeight = frameHeight * scale;

                float left = (surfaceWidth - scaledWidth) / 2;
                float top = (surfaceHeight - scaledHeight) / 2;

                canvas.drawRGB(0, 0, 0); // Clear to black
                canvas.drawBitmap(frameBuffer, null,
                        new android.graphics.RectF(left, top, left + scaledWidth, top + scaledHeight),
                        null);

                holder.unlockCanvasAndPost(canvas);
            }

            // Update input state
            processInput();

            // Return time until next frame (60 fps = ~16.7ms)
            return 16;
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Native method error: " + e.getMessage());
            return 0;
        }
    }

    private void processInput() {
        if (inputHandler == null) return;

        // Get button states from the input handler
        int[] buttonStates = inputHandler.getButtonStates();

        // Convert button array to GBA button mask
        int buttonMask = 0;
        if (buttonStates[InputHandler.BUTTON_A] == 1) buttonMask |= (1 << 0); // A
        if (buttonStates[InputHandler.BUTTON_B] == 1) buttonMask |= (1 << 1); // B
        if (buttonStates[InputHandler.BUTTON_SELECT] == 1) buttonMask |= (1 << 2); // Select
        if (buttonStates[InputHandler.BUTTON_START] == 1) buttonMask |= (1 << 3); // Start
        if (buttonStates[InputHandler.BUTTON_RIGHT] == 1) buttonMask |= (1 << 4); // Right
        if (buttonStates[InputHandler.BUTTON_LEFT] == 1) buttonMask |= (1 << 5); // Left
        if (buttonStates[InputHandler.BUTTON_UP] == 1) buttonMask |= (1 << 6); // Up
        if (buttonStates[InputHandler.BUTTON_DOWN] == 1) buttonMask |= (1 << 7); // Down
        if (buttonStates[InputHandler.BUTTON_R] == 1) buttonMask |= (1 << 8); // R
        if (buttonStates[InputHandler.BUTTON_L] == 1) buttonMask |= (1 << 9); // L

        // Send button mask to native code
        nativeUpdateInput(buttonMask);
    }

    @Override
    public void setInputHandler(InputHandler inputHandler) {
        this.inputHandler = inputHandler;
    }

    @Override
    public boolean saveState() {
        if (!isInitialized) {
            return false;
        }

        String statePath = new File(saveDir, getSaveFileName()).getAbsolutePath();
        try {
            return nativeSaveState(statePath);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to save state: " + e.getMessage());
            return false;
        }
    }

    @Override
    public boolean loadState() {
        if (!isInitialized) {
            return false;
        }

        String statePath = new File(saveDir, getSaveFileName()).getAbsolutePath();
        File stateFile = new File(statePath);

        if (!stateFile.exists()) {
            Log.d(TAG, "No save state found: " + statePath);
            return false;
        }

        try {
            return nativeLoadState(statePath);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load state: " + e.getMessage());
            return false;
        }
    }

    @Override
    public void shutdown() {
        if (isInitialized) {
            try {
                nativeCleanup();
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "Error during cleanup: " + e.getMessage());
            }
            isInitialized = false;
        }

        if (frameBuffer != null && !frameBuffer.isRecycled()) {
            frameBuffer.recycle();
            frameBuffer = null;
        }
    }

    private String getSaveFileName() {
        // Get ROM file name without extension
        String fileName = new File(romPath).getName();
        int lastDot = fileName.lastIndexOf('.');
        if (lastDot > 0) {
            fileName = fileName.substring(0, lastDot);
        }
        return fileName + ".sav";
    }

    // Native methods
    private native boolean nativeInitialize(String romPath);
    private native boolean nativeRunFrame(Bitmap bitmap);

    private native void nativeUpdateInput(int buttonMask);
    private native boolean nativeSaveState(String path);
    private native boolean nativeLoadState(String path);
    private native void nativeCleanup();
}