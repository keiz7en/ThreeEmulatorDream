package com.example.recreemulcream.emulation;

import android.view.SurfaceHolder;

import com.example.recreemulcream.emulation.core.EmulatorBridge;

/**
 * Thread responsible for running the emulation loop
 */
public class EmulationThread extends Thread {
    private static final int TARGET_FPS = 60;
    private static final long FRAME_TIME_NS = 1_000_000_000 / TARGET_FPS; // 16.6ms in nanoseconds

    private final EmulatorBridge emulatorBridge;
    private final SurfaceHolder surfaceHolder;
    private final FpsCallback fpsCallback;

    private volatile boolean isRunning;
    private volatile boolean isPaused;

    /**
     * Callback interface for FPS updates
     */
    public interface FpsCallback {
        void onFpsUpdate(int fps);
    }

    /**
     * Constructor
     * @param emulatorBridge The emulator bridge
     * @param surfaceHolder The surface holder to render to
     * @param fpsCallback Callback for FPS updates
     */
    public EmulationThread(
            EmulatorBridge emulatorBridge,
            SurfaceHolder surfaceHolder,
            FpsCallback fpsCallback) {
        this.emulatorBridge = emulatorBridge;
        this.surfaceHolder = surfaceHolder;
        this.fpsCallback = fpsCallback;
        this.isRunning = true;
        this.isPaused = false;
    }

    @Override
    public void run() {
        long lastTime = System.nanoTime();
        int frames = 0;
        long fpsTimer = System.currentTimeMillis();

        while (isRunning) {
            if (isPaused) {
                try {
                    Thread.sleep(100); // Sleep while paused
                    continue;
                } catch (InterruptedException e) {
                    // Ignore
                }
            }

            // Run a single frame
            if (surfaceHolder.getSurface().isValid()) {
                emulatorBridge.runFrame(surfaceHolder);
                frames++;
            }

            // Throttle to target FPS
            long currentTime = System.nanoTime();
            long elapsedTime = currentTime - lastTime;
            long sleepTime = (FRAME_TIME_NS - elapsedTime) / 1_000_000; // Convert to milliseconds

            if (sleepTime > 0) {
                try {
                    Thread.sleep(sleepTime);
                } catch (InterruptedException e) {
                    // Ignore
                }
            }

            lastTime = System.nanoTime();

            // Update FPS counter once per second
            if (System.currentTimeMillis() - fpsTimer >= 1000) {
                if (fpsCallback != null) {
                    fpsCallback.onFpsUpdate(frames);
                }
                frames = 0;
                fpsTimer = System.currentTimeMillis();
            }
        }
    }

    /**
     * Stop the emulation thread
     */
    public void stopEmulation() {
        isRunning = false;
        interrupt();
    }

    /**
     * Pause emulation
     */
    public void pauseEmulation() {
        isPaused = true;
    }

    /**
     * Resume emulation
     */
    public void resumeEmulation() {
        isPaused = false;
    }
}