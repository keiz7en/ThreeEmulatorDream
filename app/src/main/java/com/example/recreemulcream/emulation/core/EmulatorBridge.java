package com.example.recreemulcream.emulation.core;

import android.os.Build;
import android.view.Surface;
import android.view.SurfaceHolder;
import com.example.recreemulcream.emulation.input.InputHandler;

import java.lang.reflect.Method;

import android.util.Log;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Color;
import androidx.annotation.NonNull;

/**
 * Bridge interface for native emulator cores
 */
public interface EmulatorBridge {
    boolean initializeEmulator(String romPath);

    int runFrame(SurfaceHolder holder);

    void setInputHandler(InputHandler inputHandler);

    boolean saveState();

    boolean loadState();

    void shutdown();

    /**
     * Helper method to safely get the native surface handle
     */
    static long getSurfaceHandle(Surface surface) {
        if (surface == null) {
            Log.e("EmulatorBridge", "Surface is null");
            return 0;
        }

        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1) {
                try {
                    // Use reflection to access the native handle through SurfaceControl
                    Method method = Surface.class.getMethod("getLong", String.class);
                    return (long) method.invoke(surface, "NATIVE_WINDOW");
                } catch (Exception e) {
                    // Fallback to getNativeHandle if getLong method fails
                    Log.w("EmulatorBridge", "getLong method failed, trying getNativeHandle", e);
                    Method method = Surface.class.getMethod("getNativeHandle");
                    return (long) method.invoke(surface);
                }
            } else {
                // For older Android versions, we'll need to use reflection
                Method method = Surface.class.getMethod("getNativeHandle");
                return (long) method.invoke(surface);
            }
        } catch (Exception e) {
            Log.e("EmulatorBridge", "Failed to get surface handle", e);
            return 0;
        }
    }

    /**
     * NES Emulator Bridge implementation
     * Currently stubbed with a placeholder
     */
    class NesEmulatorBridge implements EmulatorBridge {
        static {
            try {
                System.loadLibrary("nes-lib");
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
            }
        }

        // Native method declarations
        private native boolean nativeInitialize(String romPath);

        private native int nativeRunFrame(long surfacePtr);

        private native void nativeSetInput(int[] buttons);

        private native boolean nativeSaveState();

        private native boolean nativeLoadState();

        private native void nativeShutdown();

        private InputHandler inputHandler;
        private String romPath;

        @Override
        public boolean initializeEmulator(String romPath) {
            this.romPath = romPath;
            try {
                return nativeInitialize(romPath);
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
                // Fall back to Java version
                return true;
            }
        }

        @Override
        public int runFrame(SurfaceHolder holder) {
            try {
                // Process input
                if (inputHandler != null) {
                    nativeSetInput(inputHandler.getButtonStates());
                }

                // Run frame and render if native lib is available
                long surfaceHandle = getSurfaceHandle(holder.getSurface());
                if (surfaceHandle != 0) {
                    return nativeRunFrame(surfaceHandle);
                } else {
                    // If we can't get the native handle, fallback to Java mode
                    renderBasicScreen(holder, 0xFFFF0000); // Red
                    return 16;
                }
            } catch (UnsatisfiedLinkError e) {
                // Fall back to Java version
                // Draw a simple test frame with red color
                renderBasicScreen(holder, 0xFFFF0000); // Red
                return 16; // ~60 FPS
            }
        }

        @Override
        public void setInputHandler(InputHandler inputHandler) {
            this.inputHandler = inputHandler;
        }

        @Override
        public boolean saveState() {
            try {
                return nativeSaveState();
            } catch (UnsatisfiedLinkError e) {
                // Fall back to Java version
                return true;
            }
        }

        @Override
        public boolean loadState() {
            try {
                return nativeLoadState();
            } catch (UnsatisfiedLinkError e) {
                // Fall back to Java version
                return true;
            }
        }

        @Override
        public void shutdown() {
            try {
                nativeShutdown();
            } catch (UnsatisfiedLinkError e) {
                // Ignore
            }
        }
    }

    /**
     * SNES Emulator Bridge implementation
     */
    class SnesEmulatorBridge implements EmulatorBridge {
        static {
            try {
                System.loadLibrary("snes-lib");
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
            }
        }

        // Native method declarations
        private native boolean nativeInitialize(String romPath);

        private native int nativeRunFrame(long surfacePtr);

        private native void nativeSetInput(int[] buttons);

        private native boolean nativeSaveState();

        private native boolean nativeLoadState();

        private native void nativeShutdown();

        private InputHandler inputHandler;
        private String romPath;

        @Override
        public boolean initializeEmulator(String romPath) {
            this.romPath = romPath;
            try {
                return nativeInitialize(romPath);
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
                return true;
            }
        }

        @Override
        public int runFrame(SurfaceHolder holder) {
            try {
                // Process input
                if (inputHandler != null) {
                    nativeSetInput(inputHandler.getButtonStates());
                }

                // Run frame and render
                long surfaceHandle = getSurfaceHandle(holder.getSurface());
                if (surfaceHandle != 0) {
                    return nativeRunFrame(surfaceHandle);
                } else {
                    // If we can't get the native handle, fallback to Java mode
                    renderBasicScreen(holder, 0xFF00FF00); // Green
                    return 16;
                }
            } catch (UnsatisfiedLinkError e) {
                // Fall back to Java version
                renderBasicScreen(holder, 0xFF00FF00); // Green
                return 16;
            }
        }

        @Override
        public void setInputHandler(InputHandler inputHandler) {
            this.inputHandler = inputHandler;
        }

        @Override
        public boolean saveState() {
            try {
                return nativeSaveState();
            } catch (UnsatisfiedLinkError e) {
                return true;
            }
        }

        @Override
        public boolean loadState() {
            try {
                return nativeLoadState();
            } catch (UnsatisfiedLinkError e) {
                return true;
            }
        }

        @Override
        public void shutdown() {
            try {
                nativeShutdown();
            } catch (UnsatisfiedLinkError e) {
                // Ignore
            }
        }
    }

    /**
     * GBA Emulator Bridge implementation
     */
    class GbaEmulatorBridge implements EmulatorBridge {
        static {
            try {
                System.loadLibrary("gba-lib");
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
            }
        }

        // Native method declarations
        private native boolean nativeInitialize(String romPath);

        private native int nativeRunFrame(long surfacePtr);

        private native void nativeSetInput(int[] buttons);

        private native boolean nativeSaveState();

        private native boolean nativeLoadState();

        private native void nativeShutdown();

        private InputHandler inputHandler;
        private String romPath;

        @Override
        public boolean initializeEmulator(String romPath) {
            this.romPath = romPath;
            try {
                return nativeInitialize(romPath);
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
                return true;
            }
        }

        @Override
        public int runFrame(SurfaceHolder holder) {
            try {
                // Process input
                if (inputHandler != null) {
                    nativeSetInput(inputHandler.getButtonStates());
                }

                // Run frame and render
                long surfaceHandle = getSurfaceHandle(holder.getSurface());
                if (surfaceHandle != 0) {
                    return nativeRunFrame(surfaceHandle);
                } else {
                    // If we can't get the native handle, fallback to Java mode
                    renderBasicScreen(holder, 0xFF0000FF); // Blue
                    return 16;
                }
            } catch (UnsatisfiedLinkError e) {
                // Fall back to Java version
                renderBasicScreen(holder, 0xFF0000FF); // Blue
                return 16;
            }
        }

        @Override
        public void setInputHandler(InputHandler inputHandler) {
            this.inputHandler = inputHandler;
        }

        @Override
        public boolean saveState() {
            try {
                return nativeSaveState();
            } catch (UnsatisfiedLinkError e) {
                return true;
            }
        }

        @Override
        public boolean loadState() {
            try {
                return nativeLoadState();
            } catch (UnsatisfiedLinkError e) {
                return true;
            }
        }

        @Override
        public void shutdown() {
            try {
                nativeShutdown();
            } catch (UnsatisfiedLinkError e) {
                // Ignore
            }
        }
    }

    /**
     * DS Emulator Bridge implementation
     */
    class DsEmulatorBridge implements EmulatorBridge {
        static {
            try {
                System.loadLibrary("ds-lib");
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
            }
        }

        // Native method declarations
        private native boolean nativeInitialize(String romPath);

        private native int nativeRunFrame(long surfacePtr);

        private native void nativeSetInput(int[] buttons);

        private native boolean nativeSaveState();

        private native boolean nativeLoadState();

        private native void nativeShutdown();

        private InputHandler inputHandler;
        private String romPath;

        @Override
        public boolean initializeEmulator(String romPath) {
            this.romPath = romPath;
            try {
                return nativeInitialize(romPath);
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
                return true;
            }
        }

        @Override
        public int runFrame(SurfaceHolder holder) {
            try {
                // Process input
                if (inputHandler != null) {
                    nativeSetInput(inputHandler.getButtonStates());
                }

                // Run frame and render
                long surfaceHandle = getSurfaceHandle(holder.getSurface());
                if (surfaceHandle != 0) {
                    return nativeRunFrame(surfaceHandle);
                } else {
                    // If we can't get the native handle, fallback to Java mode
                    renderBasicScreen(holder, 0xFFFF00FF); // Purple for DS
                    return 16;
                }
            } catch (UnsatisfiedLinkError e) {
                // Fall back to Java version
                renderBasicScreen(holder, 0xFFFF00FF); // Purple for DS
                return 16;
            }
        }

        @Override
        public void setInputHandler(InputHandler inputHandler) {
            this.inputHandler = inputHandler;
        }

        @Override
        public boolean saveState() {
            try {
                return nativeSaveState();
            } catch (UnsatisfiedLinkError e) {
                return true;
            }
        }

        @Override
        public boolean loadState() {
            try {
                return nativeLoadState();
            } catch (UnsatisfiedLinkError e) {
                return true;
            }
        }

        @Override
        public void shutdown() {
            try {
                nativeShutdown();
            } catch (UnsatisfiedLinkError e) {
                // Ignore
            }
        }
    }

    // Thread-safe paint holder using a holder class pattern
    static final class PaintHolder {
        static final Paint PAINT = createPaint();

        private static Paint createPaint() {
            Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setTextSize(40);
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setColor(Color.WHITE); // Set default color
            return paint;
        }
    }

    // Method to safely access the Paint instance
    static Paint getPaint() {
        return PaintHolder.PAINT;
    }

    // Helper method to create a basic emulator screen
    static void renderBasicScreen(SurfaceHolder holder, int color) {
        if (holder == null) {
            Log.e("EmulatorBridge", "SurfaceHolder is null");
            return;
        }

        Canvas canvas = null;
        try {
            canvas = holder.lockCanvas();
            if (canvas != null) {
                Paint paint = getPaint();

                // Fill background
                paint.setColor(color);
                canvas.drawRect(0, 0, canvas.getWidth(), canvas.getHeight(), paint);

                // Draw "Java-Only Mode" text
                paint.setColor(Color.WHITE);
                float centerX = canvas.getWidth() / 2f;
                float centerY = canvas.getHeight() / 2f;
                canvas.drawText("Native libraries not loaded",
                        centerX, centerY - 20, paint);
                canvas.drawText("(Falling back to Java mode)",
                        centerX, centerY + 40, paint);
            }
        } catch (Exception e) {
            Log.e("EmulatorBridge", "Error rendering fallback screen", e);
        } finally {
            if (canvas != null) {
                try {
                    holder.unlockCanvasAndPost(canvas);
                } catch (Exception e) {
                    Log.e("EmulatorBridge", "Failed to post canvas", e);
                }
            }
        }
    }
}