package com.example.recreemulcream;

import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;

import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.example.recreemulcream.databinding.ActivityMainBinding;
import com.example.recreemulcream.emulation.EmulatorActivity;
import com.example.recreemulcream.util.EmulatorStatusChecker;
import com.example.recreemulcream.util.FilePickerHelper;
import com.example.recreemulcream.util.FileUtils;
import com.example.recreemulcream.util.PermissionsHelper;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    // Used to load the native library on application startup
    static {
        try {
            System.loadLibrary("recreemulcream");
        } catch (UnsatisfiedLinkError e) {
            // Handle native library load failure
        }
    }

    private ActivityMainBinding binding;
    private PermissionsHelper permissionsHelper;
    private String selectedConsole = "";
    private List<File> foundRoms = new ArrayList<>();
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    private EmulatorStatusChecker emulatorStatusChecker = new EmulatorStatusChecker();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        permissionsHelper = new PermissionsHelper(this);
        permissionsHelper.requestStoragePermissions();

        setupUI();

        // Check emulator availability
        checkEmulatorsAvailable();
    }

    private void setupUI() {
        // Set up console selection
        binding.nesCard.setOnClickListener(v -> selectConsole("nes"));
        binding.gbaCard.setOnClickListener(v -> selectConsole("gba"));
        binding.snesCard.setOnClickListener(v -> selectConsole("snes"));
        binding.dsCard.setOnClickListener(v -> selectConsole("ds"));

        // Set up action buttons
        binding.loadRomButton.setOnClickListener(v -> loadRom());
        binding.scanRomButton.setOnClickListener(v -> scanForRoms());
        binding.settingsButton.setOnClickListener(v -> openSettings());

        // Show emulator version info
        binding.versionInfo.setText(getVersionInfo());
    }

    private String getVersionInfo() {
        return "RecreEmulCream v1.0 (Java-only mode)";
    }

    private void selectConsole(String console) {
        // Reset selection highlight
        binding.nesCard.setCardBackgroundColor(getResources().getColor(R.color.console_card_bg, null));
        binding.gbaCard.setCardBackgroundColor(getResources().getColor(R.color.console_card_bg, null));
        binding.snesCard.setCardBackgroundColor(getResources().getColor(R.color.console_card_bg, null));
        binding.dsCard.setCardBackgroundColor(getResources().getColor(R.color.console_card_bg, null));

        // Highlight selected console
        selectedConsole = console;
        CardView selectedCard = null;

        switch (console) {
            case "nes":
                selectedCard = binding.nesCard;
                break;
            case "gba":
                selectedCard = binding.gbaCard;
                break;
            case "snes":
                selectedCard = binding.snesCard;
                break;
            case "ds":
                selectedCard = binding.dsCard;
                break;
        }

        if (selectedCard != null) {
            selectedCard.setCardBackgroundColor(getResources().getColor(R.color.console_selected_bg, null));
        }

        // Show toast with selection
        String consoleName = "";
        switch (console) {
            case "nes":
                consoleName = "Nintendo Entertainment System";
                break;
            case "gba":
                consoleName = "Game Boy Advance";
                break;
            case "snes":
                consoleName = "Super Nintendo";
                break;
            case "ds":
                consoleName = "Nintendo DS";
                break;
        }
        Toast.makeText(this, consoleName + " selected", Toast.LENGTH_SHORT).show();

        // Check if console has special requirements (like DS with BIOS files)
        if ("ds".equals(console) && !EmulatorStatusChecker.checkRequiredBiosFiles(this, console)) {
            Toast.makeText(this,
                    "Warning: Nintendo DS emulation requires BIOS files that aren't present",
                    Toast.LENGTH_LONG).show();
        }
    }

    private void loadRom() {
        if (selectedConsole.isEmpty()) {
            Toast.makeText(this, "Please select a console first", Toast.LENGTH_SHORT).show();
            return;
        }

        // Start file picker with appropriate file extension filter
        String[] extensions;
        switch (selectedConsole) {
            case "gba":
                extensions = new String[]{".gba"};
                break;
            case "snes":
                extensions = new String[]{".sfc", ".smc"};
                break;
            case "ds":
                extensions = new String[]{".nds"};
                break;
            default:
                extensions = new String[]{};
                break;
        }

        FilePickerHelper.pickRomFile(this, extensions);
    }

    private void scanForRoms() {
        if (selectedConsole.isEmpty()) {
            Toast.makeText(this, "Please select a console first", Toast.LENGTH_SHORT).show();
            return;
        }

        // Start ROM scanning process
        binding.romScanProgress.setVisibility(View.VISIBLE);
        binding.scanRomButton.setEnabled(false);
        binding.loadRomButton.setEnabled(false);

        foundRoms.clear();

        // Get file extensions for selected console
        String[] extensions;
        switch (selectedConsole) {
            case "gba":
                extensions = new String[]{".gba"};
                break;
            case "snes":
                extensions = new String[]{".sfc", ".smc"};
                break;
            case "ds":
                extensions = new String[]{".nds"};
                break;
            default:
                extensions = new String[]{};
                break;
        }

        // Start scanning
        FilePickerHelper.scanForRoms(this, extensions, new FilePickerHelper.RomScanCallback() {
            @Override
            public void onRomFound(File romFile) {
                foundRoms.add(romFile);
                mainHandler.post(() -> {
                    binding.scanStatusText.setText("Found: " + romFile.getName());
                });
            }

            @Override
            public void onScanComplete() {
                mainHandler.post(() -> {
                    binding.romScanProgress.setVisibility(View.GONE);
                    binding.scanRomButton.setEnabled(true);
                    binding.loadRomButton.setEnabled(true);

                    if (foundRoms.size() > 0) {
                        binding.scanStatusText.setText("Found " + foundRoms.size() + " ROMs");

                        // Show ROM selection dialog
                        showRomSelectionDialog(foundRoms);
                    } else {
                        binding.scanStatusText.setText("No ROMs found");
                    }
                });
            }

            @Override
            public void onScanError(String error) {
                mainHandler.post(() -> {
                    binding.romScanProgress.setVisibility(View.GONE);
                    binding.scanRomButton.setEnabled(true);
                    binding.loadRomButton.setEnabled(true);
                    binding.scanStatusText.setText("Error: " + error);
                });
            }
        });
    }

    private void showRomSelectionDialog(List<File> roms) {
        if (roms.isEmpty()) return;

        // Create a dialog to let the user choose a ROM
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Select a ROM");

        String[] romNames = new String[roms.size()];
        for (int i = 0; i < roms.size(); i++) {
            romNames[i] = roms.get(i).getName();
        }

        builder.setItems(romNames, (dialog, which) -> {
            File selectedRom = roms.get(which);
            startEmulation(selectedRom.getAbsolutePath());
        });

        builder.show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (FilePickerHelper.handleFilePickerResult(requestCode, resultCode, data)) {
            String romPath = FilePickerHelper.getUsableFilePath(this);
            if (romPath != null && !romPath.isEmpty()) {
                startEmulation(romPath);
            }
        }
    }

    private void startEmulation(String romPath) {
        Intent intent = new Intent(this, EmulatorActivity.class);
        intent.putExtra(EmulatorActivity.EXTRA_ROM_PATH, romPath);
        intent.putExtra(EmulatorActivity.EXTRA_CONSOLE_TYPE, selectedConsole);
        startActivity(intent);
    }

    private void openSettings() {
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivity(intent);
    }

    private void checkEmulatorsAvailable() {
        boolean gbaAvailable = true; // Temporarily hardcode to true
        boolean snesAvailable = true; // Temporarily hardcode to true
        boolean dsAvailable = true; // Temporarily hardcode to true

        // Commented out actual checks until the native library is ready
        // boolean gbaAvailable = emulatorStatusChecker.checkEmulatorAvailability("gba");
        // boolean snesAvailable = emulatorStatusChecker.checkEmulatorAvailability("snes");
        // boolean dsAvailable = emulatorStatusChecker.checkEmulatorAvailability("ds");

        // Enable/disable cards based on availability
        binding.gbaCard.setEnabled(gbaAvailable);
        binding.snesCard.setEnabled(snesAvailable);
        binding.dsCard.setEnabled(dsAvailable);

        // Apply visual indication if not available
        if (!gbaAvailable) {
            binding.gbaCard.setAlpha(0.5f);
        }
        if (!snesAvailable) {
            binding.snesCard.setAlpha(0.5f);
        }
        if (!dsAvailable) {
            binding.dsCard.setAlpha(0.5f);
        }
    }
}