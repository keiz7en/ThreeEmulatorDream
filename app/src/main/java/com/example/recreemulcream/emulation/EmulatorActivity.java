package com.example.recreemulcream.emulation;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;

import com.example.recreemulcream.R;
import com.example.recreemulcream.emulation.core.EmulatorBridge;
import com.example.recreemulcream.emulation.input.VirtualGamepad;

public class EmulatorActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    public static final String EXTRA_ROM_PATH = "rom_path";
    public static final String EXTRA_CONSOLE_TYPE = "console_type";

    private SurfaceView emulatorSurface;
    private SurfaceHolder surfaceHolder;
    private TextView fpsCounterText;
    private ConstraintLayout controlsLayout;
    private VirtualGamepad virtualGamepad;

    private EmulatorBridge emulatorBridge;
    private EmulationThread emulationThread;

    private String romPath;
    private String consoleType;
    private boolean isEmulationRunning = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_emulator);

        // Keep screen on during gameplay
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Force landscape orientation for better gaming experience
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        // Hide system UI for immersive mode
        hideSystemUI();

        // Get ROM path and console type from intent
        romPath = getIntent().getStringExtra(EXTRA_ROM_PATH);
        consoleType = getIntent().getStringExtra(EXTRA_CONSOLE_TYPE);

        if (romPath == null || consoleType == null) {
            finish(); // Exit if no ROM path or console type provided
            return;
        }

        // Set title to ROM name
        String romName = getRomName(romPath);
        setTitle(romName + " - " + getConsoleDisplayName(consoleType));

        // Initialize UI components
        setupUIComponents();

        // Initialize emulator bridge based on console type
        initializeEmulatorBridge();
    }

    private void setupUIComponents() {
        emulatorSurface = findViewById(R.id.emulatorSurface);
        surfaceHolder = emulatorSurface.getHolder();
        surfaceHolder.addCallback(this);

        fpsCounterText = findViewById(R.id.fpsCounter);

        // Show FPS counter based on preferences
        android.content.SharedPreferences prefs = getSharedPreferences("EmulatorPrefs", MODE_PRIVATE);
        boolean showFps = prefs.getBoolean("fps_counter", false);
        fpsCounterText.setVisibility(showFps ? View.VISIBLE : View.GONE);

        controlsLayout = findViewById(R.id.controlsLayout);

        // Setup virtual gamepad based on console type
        virtualGamepad = new VirtualGamepad(this, consoleType);
        virtualGamepad.setupButtons(controlsLayout);

        // Show L/R buttons for SNES and GBA
        View shoulderButtons = findViewById(R.id.shoulderButtonsContainer);
        if (consoleType.equals("gba") || consoleType.equals("snes")) {
            shoulderButtons.setVisibility(View.VISIBLE);
        }

        // Setup control buttons
        ImageButton pauseButton = findViewById(R.id.pauseButton);
        pauseButton.setOnClickListener(v -> {
            if (isEmulationRunning) {
                pauseEmulation();
                Toast.makeText(this, "Emulation paused", Toast.LENGTH_SHORT).show();
                pauseButton.setImageResource(android.R.drawable.ic_media_play);
            } else {
                resumeEmulation();
                Toast.makeText(this, "Emulation resumed", Toast.LENGTH_SHORT).show();
                pauseButton.setImageResource(android.R.drawable.ic_media_pause);
            }
        });

        ImageButton saveButton = findViewById(R.id.saveButton);
        saveButton.setOnClickListener(v -> {
            boolean success = saveState();
            if (success) {
                Toast.makeText(this, "State saved", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(this, "Failed to save state", Toast.LENGTH_SHORT).show();
            }
        });

        ImageButton loadButton = findViewById(R.id.loadButton);
        loadButton.setOnClickListener(v -> {
            boolean success = loadState();
            if (success) {
                Toast.makeText(this, "State loaded", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(this, "No saved state found", Toast.LENGTH_SHORT).show();
            }
        });

        // Hide ROM name display
        TextView romNameDisplay = findViewById(R.id.romNameDisplay);
        if (romNameDisplay != null) {
            romNameDisplay.setVisibility(View.GONE);
        }

        // Display Java-only mode message
        Toast.makeText(this, "Java-Only Mode (No Native Support)", Toast.LENGTH_LONG).show();
    }

    private void initializeEmulatorBridge() {
        switch (consoleType) {
            case "gba":
                emulatorBridge = new EmulatorBridge.GbaEmulatorBridge();
                break;
            case "snes":
                emulatorBridge = new EmulatorBridge.SnesEmulatorBridge();
                break;
            case "ds":
                emulatorBridge = new EmulatorBridge.GbaEmulatorBridge();
                break;
            case "nes":
                emulatorBridge = new EmulatorBridge.NesEmulatorBridge();
                break;
            default:
                finish(); // Unsupported console type
                return;
        }

        // Set input handler
        emulatorBridge.setInputHandler(virtualGamepad);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        surfaceHolder = holder;
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        surfaceHolder = holder;

        // Surface is ready, start emulation
        startEmulation();
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        // Stop emulation when surface is destroyed
        stopEmulation();
    }

    private void startEmulation() {
        if (emulatorBridge != null && surfaceHolder != null) {
            // Initialize the emulator with the ROM
            if (emulatorBridge.initializeEmulator(romPath)) {
                // Start emulation thread
                emulationThread = new EmulationThread(
                        emulatorBridge,
                        surfaceHolder,
                        new EmulationThread.FpsCallback() {
                            @Override
                            public void onFpsUpdate(int fps) {
                                runOnUiThread(() -> updateFpsCounter(fps));
                            }
                        }
                );
                emulationThread.start();
                isEmulationRunning = true;
            } else {
                // Show error message if initialization fails
                showError("Failed to load ROM: " + romPath);
            }
        }
    }

    private void stopEmulation() {
        if (emulationThread != null) {
            emulationThread.stopEmulation();
            emulationThread = null;
        }

        if (emulatorBridge != null) {
            emulatorBridge.shutdown();
        }

        isEmulationRunning = false;
    }

    private void pauseEmulation() {
        if (emulationThread != null) {
            emulationThread.pauseEmulation();
            isEmulationRunning = false;
        }
    }

    private void resumeEmulation() {
        if (emulationThread != null) {
            emulationThread.resumeEmulation();
            isEmulationRunning = true;
        }
    }

    private boolean saveState() {
        if (emulatorBridge != null) {
            boolean success = emulatorBridge.saveState();
            // Show a visual indicator that state was saved
            if (success) {
                showSaveStateIndicator();
            }
            return success;
        }
        return false;
    }

    private boolean loadState() {
        if (emulatorBridge != null) {
            boolean success = emulatorBridge.loadState();
            // Show a visual indicator that state was loaded
            if (success) {
                showLoadStateIndicator();
            }
            return success;
        }
        return false;
    }

    private void showSaveStateIndicator() {
        // Create and show a brief "State Saved" indicator in the UI
        View container = findViewById(android.R.id.content);
        if (container != null) {
            Toast.makeText(this, "Game state saved", Toast.LENGTH_SHORT).show();
        }
    }

    private void showLoadStateIndicator() {
        // Create and show a brief "State Loaded" indicator in the UI
        View container = findViewById(android.R.id.content);
        if (container != null) {
            Toast.makeText(this, "Game state loaded", Toast.LENGTH_SHORT).show();
        }
    }

    private void updateFpsCounter(int fps) {
        if (fpsCounterText != null) {
            fpsCounterText.setText(String.format("FPS: %d", fps));
        }
    }

    private void showError(String message) {
        // Simple error toast implementation
        android.widget.Toast.makeText(this, message, android.widget.Toast.LENGTH_LONG).show();
        finish();
    }

    private void hideSystemUI() {
        // Enables regular immersive mode
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                        View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                        View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                        View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                        View.SYSTEM_UI_FLAG_FULLSCREEN
        );
    }

    private String getRomName(String path) {
        int lastSlash = path.lastIndexOf('/');
        if (lastSlash >= 0 && lastSlash < path.length() - 1) {
            return path.substring(lastSlash + 1);
        }
        return path;
    }

    private String getConsoleDisplayName(String consoleType) {
        switch (consoleType) {
            case "gba":
                return "Game Boy Advance";
            case "snes":
                return "Super Nintendo";
            case "ds":
                return "Nintendo DS";
            case "nes":
                return "Nintendo Entertainment System";
            default:
                return consoleType.toUpperCase();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemUI();
        if (emulationThread != null && !isEmulationRunning) {
            resumeEmulation();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (isEmulationRunning) {
            pauseEmulation();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        stopEmulation();
    }
}