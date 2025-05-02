package com.example.recreemulcream.emulation.core;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.Log;
import android.view.SurfaceHolder;

import com.example.recreemulcream.emulation.input.InputHandler;

import java.io.File;

/**
 * Bridge between Java and native melonDS emulator code
 */
public class DsEmulatorBridge implements EmulatorBridge {
    private static final String TAG = "DsEmulatorBridge";

    private InputHandler inputHandler;
    private boolean isInitialized = false;
    private Context context;
    private String romPath;
    private String saveDir;
    private String biosDir;
    private Bitmap frameBuffer;

    // Store touch coordinates
    private int touchX = -1;  // -1 means not touching
    private int touchY = -1;

    static {
        System.loadLibrary("recreemulcream");
    }

    public DsEmulatorBridge(Context context) {
        this.context = context;
        this.saveDir = new File(context.getFilesDir(), "saves/ds").getAbsolutePath();
        this.biosDir = new File(context.getFilesDir(), "bios").getAbsolutePath();

        // Create save directory
        File saveDirFile = new File(saveDir);
        if (!saveDirFile.exists()) {
            saveDirFile.mkdirs();
        }

        // Create BIOS directory
        File biosDirFile = new File(biosDir);
        if (!biosDirFile.exists()) {
            biosDirFile.mkdirs();
        }

        // Create framebuffer for DS (256x384, combining both screens)
        frameBuffer = Bitmap.createBitmap(256, 384, Bitmap.Config.ARGB_8888);
    }

    @Override
    public boolean initializeEmulator(String romPath) {
        Log.d(TAG, "Initializing DS emulator with ROM: " + romPath);
        this.romPath = romPath;

        // Look for BIOS files
        File bios7File = new File(biosDir, "bios7.bin");
        File bios9File = new File(biosDir, "bios9.bin");
        File firmwareFile = new File(biosDir, "firmware.bin");

        if (!bios7File.exists() || !bios9File.exists() || !firmwareFile.exists()) {
            Log.e(TAG, "BIOS files not found. DS emulation requires BIOS files!");
            return false;
        }

        // Initialize with ROM and BIOS paths
        isInitialized = nativeInitialize(romPath, biosDir);
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

            // Convert button array to bit mask (format expected by melonDS)
            if (buttonStates[InputHandler.BUTTON_A] == 1) buttonMask |= 0x001;      // A
            if (buttonStates[InputHandler.BUTTON_B] == 1) buttonMask |= 0x002;      // B
            if (buttonStates[InputHandler.BUTTON_SELECT] == 1) buttonMask |= 0x004;  // Select
            if (buttonStates[InputHandler.BUTTON_START] == 1) buttonMask |= 0x008;   // Start
            if (buttonStates[InputHandler.BUTTON_RIGHT] == 1) buttonMask |= 0x010;   // Right
            if (buttonStates[InputHandler.BUTTON_LEFT] == 1) buttonMask |= 0x020;    // Left
            if (buttonStates[InputHandler.BUTTON_UP] == 1) buttonMask |= 0x040;      // Up
            if (buttonStates[InputHandler.BUTTON_DOWN] == 1) buttonMask |= 0x080;    // Down
            if (buttonStates[InputHandler.BUTTON_R] == 1) buttonMask |= 0x100;      // R
            if (buttonStates[InputHandler.BUTTON_L] == 1) buttonMask |= 0x200;      // L
            if (buttonStates[InputHandler.BUTTON_X] == 1) buttonMask |= 0x400;      // X
            if (buttonStates[InputHandler.BUTTON_Y] == 1) buttonMask |= 0x800;      // Y

            // Update touch screen coordinates
            updateTouchCoordinates(buttonStates);

            nativeUpdateInput(buttonMask, touchX, touchY);
        }

        // Return approximate time (in milliseconds) until next frame should be drawn
        return 16; // ~60 FPS
    }

    /**
     * Updates touch coordinates based on input handler
     * This is a simple implementation where we map different
     * buttons to touch different parts of the screen
     */
    private void updateTouchCoordinates(int[] buttonStates) {
        // If button mapped to touch is pressed, simulate touch on bottom screen
        boolean isTouching = false;

        // Check if screen tap requested (some custom buttons or screen overlay)
        // For simplicity, we're not implementing this in detail here

        if (isTouching) {
            // Screen touch coordinates can be set here - for now using center of screen
            touchX = 128;
            touchY = 96;
        } else {
            // No touching
            touchX = -1;
            touchY = -1;
        }
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
    private native boolean nativeInitialize(String romPath, String biosPath);

    private native boolean nativeRunFrame(Bitmap bitmap);

    private native void nativeUpdateInput(int buttonsState, int touchX, int touchY);

    private native boolean nativeSaveState(String path);

    private native boolean nativeLoadState(String path);

    private native void nativeCleanup();
}