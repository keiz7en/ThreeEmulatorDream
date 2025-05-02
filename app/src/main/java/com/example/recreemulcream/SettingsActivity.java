package com.example.recreemulcream;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import com.example.recreemulcream.databinding.ActivitySettingsBinding;

public class SettingsActivity extends AppCompatActivity {

    private static final String PREFS_NAME = "EmulatorPrefs";
    private static final String KEY_FPS_COUNTER = "fps_counter";
    private static final String KEY_FRAME_SKIP = "frame_skip";
    private static final String KEY_AUDIO_LATENCY = "audio_latency";
    private static final String KEY_SCALING_MODE = "scaling_mode";
    private static final String KEY_BUTTON_MAPPING_PREFIX = "button_mapping_";
    private static final String KEY_BUTTON_SIZE = "button_size";
    private static final String KEY_BUTTON_OPACITY = "button_opacity";

    private ActivitySettingsBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivitySettingsBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Setup action bar with back button
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setTitle(R.string.settings);
        }

        setupSettings();
    }

    private void setupSettings() {
        // FPS counter toggle
        binding.fpsCounterSwitch.setChecked(getBooleanPreference(KEY_FPS_COUNTER, false));
        binding.fpsCounterSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            savePreference(KEY_FPS_COUNTER, isChecked);
        });

        // Frame skip toggle
        binding.frameSkipSwitch.setChecked(getBooleanPreference(KEY_FRAME_SKIP, false));
        binding.frameSkipSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            savePreference(KEY_FRAME_SKIP, isChecked);
        });

        // Audio latency slider
        int latency = getIntPreference(KEY_AUDIO_LATENCY, 3);
        binding.audioLatencySeekBar.setProgress(latency);
        binding.audioLatencyValue.setText(String.format("%d ms", latency * 10));

        binding.audioLatencySeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                binding.audioLatencyValue.setText(String.format("%d ms", progress * 10));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                savePreference(KEY_AUDIO_LATENCY, seekBar.getProgress());
            }
        });

        // Scaling mode radio buttons
        int scalingMode = getIntPreference(KEY_SCALING_MODE, 0);
        switch (scalingMode) {
            case 0:
                binding.scalingOriginalRadio.setChecked(true);
                break;
            case 1:
                binding.scalingStretchRadio.setChecked(true);
                break;
            case 2:
                binding.scalingPixelPerfectRadio.setChecked(true);
                break;
        }

        binding.scalingRadioGroup.setOnCheckedChangeListener((group, checkedId) -> {
            int mode = 0;
            if (checkedId == R.id.scalingStretchRadio) {
                mode = 1;
            } else if (checkedId == R.id.scalingPixelPerfectRadio) {
                mode = 2;
            }
            savePreference(KEY_SCALING_MODE, mode);
        });

        // Setup button remapping
        binding.buttonRemappingButton.setOnClickListener(v -> {
            showButtonRemappingDialog();
        });

        // Setup button customization
        binding.buttonCustomizationButton.setOnClickListener(v -> {
            showButtonCustomizationDialog();
        });
    }

    /**
     * Shows a dialog for remapping controller buttons
     */
    private void showButtonRemappingDialog() {
        String[] buttonLabels = {
                "A Button", "B Button", "X Button", "Y Button",
                "L Trigger", "R Trigger", "Select", "Start",
                "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right"
        };

        String[] buttonKeys = {
                "a", "b", "x", "y", "l", "r", "select", "start",
                "up", "down", "left", "right"
        };

        // Current mappings
        String[] currentMappings = new String[buttonLabels.length];
        for (int i = 0; i < buttonKeys.length; i++) {
            currentMappings[i] = getStringPreference(KEY_BUTTON_MAPPING_PREFIX + buttonKeys[i],
                    "Default (" + buttonLabels[i] + ")");
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Button Remapping");

        // Create a ListView with current mappings
        ListView listView = new ListView(this);
        ButtonRemappingAdapter adapter = new ButtonRemappingAdapter(this,
                buttonLabels, currentMappings);
        listView.setAdapter(adapter);

        builder.setView(listView);
        builder.setPositiveButton("Close", null);

        // Handle list item clicks to remap buttons
        listView.setOnItemClickListener((parent, view, position, id) -> {
            showRemapOptionsDialog(buttonLabels[position], buttonKeys[position]);
        });

        builder.show();
    }

    /**
     * Shows dialog for remapping a specific button
     */
    private void showRemapOptionsDialog(String buttonLabel, String buttonKey) {
        String[] remapOptions = {
                "Default",
                "A", "B", "X", "Y",
                "L", "R",
                "Start", "Select",
                "Up", "Down", "Left", "Right"
        };

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Remap " + buttonLabel);

        builder.setItems(remapOptions, (dialog, which) -> {
            String mapping = which == 0 ? "Default" : remapOptions[which];
            savePreference(KEY_BUTTON_MAPPING_PREFIX + buttonKey, mapping);
            Toast.makeText(this,
                    buttonLabel + " remapped to " + mapping,
                    Toast.LENGTH_SHORT).show();

            // Refresh screen to show new mapping
            recreate();
        });

        builder.setNegativeButton("Cancel", null);
        builder.show();
    }

    /**
     * Shows a dialog for customizing buttons
     */
    private void showButtonCustomizationDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Button Customization");

        View view = getLayoutInflater().inflate(R.layout.dialog_button_customization, null);

        // Button size slider
        SeekBar buttonSizeSlider = view.findViewById(R.id.buttonSizeSlider);
        int currentSize = getIntPreference(KEY_BUTTON_SIZE, 50); // Default 50%
        buttonSizeSlider.setProgress(currentSize);

        // Button opacity slider
        SeekBar buttonOpacitySlider = view.findViewById(R.id.buttonOpacitySlider);
        int currentOpacity = getIntPreference(KEY_BUTTON_OPACITY, 70); // Default 70%
        buttonOpacitySlider.setProgress(currentOpacity);

        builder.setView(view);

        builder.setPositiveButton("Apply", (dialog, which) -> {
            // Save button size
            savePreference(KEY_BUTTON_SIZE, buttonSizeSlider.getProgress());
            savePreference(KEY_BUTTON_OPACITY, buttonOpacitySlider.getProgress());
            Toast.makeText(this, "Button settings saved. Restart game for changes to take effect.", Toast.LENGTH_SHORT).show();
        });

        builder.setNegativeButton("Cancel", null);

        AlertDialog dialog = builder.create();
        dialog.show();
    }

    /**
     * Adapter for button remapping list
     */
    private class ButtonRemappingAdapter extends ArrayAdapter<String> {
        private final String[] buttonLabels;
        private final String[] currentMappings;

        public ButtonRemappingAdapter(Context context, String[] buttonLabels, String[] currentMappings) {
            super(context, android.R.layout.simple_list_item_2, android.R.id.text1, buttonLabels);
            this.buttonLabels = buttonLabels;
            this.currentMappings = currentMappings;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View view = super.getView(position, convertView, parent);

            TextView text1 = view.findViewById(android.R.id.text1);
            TextView text2 = view.findViewById(android.R.id.text2);

            text1.setText(buttonLabels[position]);
            text2.setText(currentMappings[position]);

            return view;
        }
    }

    private boolean getBooleanPreference(String key, boolean defaultValue) {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        return prefs.getBoolean(key, defaultValue);
    }

    private int getIntPreference(String key, int defaultValue) {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        return prefs.getInt(key, defaultValue);
    }

    private String getStringPreference(String key, String defaultValue) {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        return prefs.getString(key, defaultValue);
    }

    private void savePreference(String key, boolean value) {
        SharedPreferences.Editor editor = getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit();
        editor.putBoolean(key, value);
        editor.apply();
    }

    private void savePreference(String key, int value) {
        SharedPreferences.Editor editor = getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit();
        editor.putInt(key, value);
        editor.apply();
    }

    private void savePreference(String key, String value) {
        SharedPreferences.Editor editor = getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit();
        editor.putString(key, value);
        editor.apply();
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}