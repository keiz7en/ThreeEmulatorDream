package com.example.recreemulcream.emulation.core;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.Log;
import android.view.SurfaceHolder;

import com.example.recreemulcream.emulation.input.InputHandler;

import java.io.File;

/**
 * Bridge between Java and native Snes9x emulator code
 */
public class SnesEmulatorBridge implements EmulatorBridge {
    private static final String TAG = "SnesEmulatorBridge";

    private InputHandler inputHandler;
    private boolean isInitialized = false;
    private Context context;
    private String romPath;
    private String saveDir;
    private Bitmap frameBuffer;

    static {
        try {
            System.loadLibrary("recreemulcream");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Error loading native library: " + e.getMessage());
        }
    }

    public SnesEmulatorBridge(Context context) {
        this.context = context;
        this.saveDir = new File(context.getFilesDir(), "saves/snes").getAbsolutePath();

        // Create save directory
        File saveDirFile = new File(saveDir);
        if (!saveDirFile.exists()) {
            saveDirFile.mkdirs();
        }

        // Create framebuffer for SNES (256x224)
        frameBuffer = Bitmap.createBitmap(256, 224, Bitmap.Config.ARGB_8888);
    }

    @Override
    public boolean initializeEmulator(String romPath) {
        Log.d(TAG, "Initializing SNES emulator with ROM: " + romPath);
        this.romPath = romPath;
        isInitialized = nativeInitialize(romPath);
        return isInitialized;
    }

    @Override
    public int runFrame(SurfaceHolder holder) {
        if (!isInitialized || holder == null) {
            return 0;
        }

        // Run a frame and draw to the bitmap
        boolean frameRendered = nativeRunFrame(frameBuffer);
        if (!frameRendered) {
            return 0;
        }

        // Draw the framebuffer to the surface
        try {
            Canvas canvas = holder.lockCanvas();
            if (canvas != null) {
                // Scale to fit the surface while maintaining aspect ratio
                canvas.drawBitmap(frameBuffer, null,
                        new android.graphics.Rect(0, 0, canvas.getWidth(), canvas.getHeight()), null);
                holder.unlockCanvasAndPost(canvas);
            }
        } catch (Exception e) {
            Log.e(TAG, "Error drawing to surface", e);
            return 0;
        }

        // Update input state if handler is available
        if (inputHandler != null) {
            int[] buttonStates = inputHandler.getButtonStates();
            int buttonMask = 0;

            // Convert button array to bit mask (format expected by Snes9x)
            if (buttonStates[InputHandler.BUTTON_A] == 1) buttonMask |= 0x80;       // A
            if (buttonStates[InputHandler.BUTTON_B] == 1) buttonMask |= 0x40;       // B
            if (buttonStates[InputHandler.BUTTON_X] == 1) buttonMask |= 0x20;       // X
            if (buttonStates[InputHandler.BUTTON_Y] == 1) buttonMask |= 0x10;       // Y
            if (buttonStates[InputHandler.BUTTON_L] == 1) buttonMask |= 0x08;       // L
            if (buttonStates[InputHandler.BUTTON_R] == 1) buttonMask |= 0x04;       // R
            if (buttonStates[InputHandler.BUTTON_SELECT] == 1) buttonMask |= 0x02;  // Select
            if (buttonStates[InputHandler.BUTTON_START] == 1) buttonMask |= 0x01;   // Start

            // D-pad uses the high byte
            if (buttonStates[InputHandler.BUTTON_UP] == 1) buttonMask |= 0x800;     // Up
            if (buttonStates[InputHandler.BUTTON_DOWN] == 1) buttonMask |= 0x400;   // Down
            if (buttonStates[InputHandler.BUTTON_LEFT] == 1) buttonMask |= 0x200;   // Left
            if (buttonStates[InputHandler.BUTTON_RIGHT] == 1) buttonMask |= 0x100;  // Right

            nativeUpdateInput(buttonMask);
        }

        // Return approximate time (in milliseconds) until next frame should be drawn
        return 16; // ~60 FPS
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

        File stateFile = new File(saveDir, getStateFileName());
        return nativeSaveState(stateFile.getAbsolutePath());
    }

    @Override
    public boolean loadState() {
        if (!isInitialized) {
            return false;
        }

        File stateFile = new File(saveDir, getStateFileName());
        if (!stateFile.exists()) {
            Log.d(TAG, "Save state doesn't exist: " + stateFile.getAbsolutePath());
            return false;
        }

        return nativeLoadState(stateFile.getAbsolutePath());
    }

    @Override
    public void shutdown() {
        if (isInitialized) {
            nativeCleanup();
            isInitialized = false;
        }

        if (frameBuffer != null && !frameBuffer.isRecycled()) {
            frameBuffer.recycle();
            frameBuffer = null;
        }
    }

    private String getStateFileName() {
        // Generate save state filename based on the ROM name
        String romName = new File(romPath).getName();
        return romName + ".sav";
    }

    // Native methods
    private native boolean nativeInitialize(String romPath);

    private native boolean nativeRunFrame(Bitmap bitmap);

    private native void nativeUpdateInput(int buttonsState);

    private native boolean nativeSaveState(String path);

    private native boolean nativeLoadState(String path);

    private native void nativeCleanup();
}