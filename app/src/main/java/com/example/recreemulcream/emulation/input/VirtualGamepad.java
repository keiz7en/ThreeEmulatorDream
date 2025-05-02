package com.example.recreemulcream.emulation.input;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.example.recreemulcream.R;

/**
 * Implementation of an on-screen virtual gamepad
 */
public class VirtualGamepad implements InputHandler {
    private final Context context;
    private final String consoleType;
    private final int[] buttonStates;

    private static final String PREFS_NAME = "EmulatorPrefs";
    private static final String KEY_BUTTON_MAPPING_PREFIX = "button_mapping_";

    private static final int BUTTON_UP = 0;
    private static final int BUTTON_DOWN = 1;
    private static final int BUTTON_LEFT = 2;
    private static final int BUTTON_RIGHT = 3;
    private static final int BUTTON_A = 4;
    private static final int BUTTON_B = 5;
    private static final int BUTTON_X = 6;
    private static final int BUTTON_Y = 7;
    private static final int BUTTON_L = 8;
    private static final int BUTTON_R = 9;
    private static final int BUTTON_SELECT = 10;
    private static final int BUTTON_START = 11;

    public VirtualGamepad(Context context, String consoleType) {
        this.context = context;
        this.consoleType = consoleType;
        this.buttonStates = new int[12]; // 12 buttons total
    }

    /**
     * Setup the virtual buttons and their listeners
     */
    public void setupButtons(ConstraintLayout controlsLayout) {
        setupDpad(controlsLayout);
        setupActionButtons(controlsLayout);
        setupMenuButtons(controlsLayout);

        // SNES and GBA have shoulder buttons, NES doesn't
        if (!consoleType.equals("nes")) {
            setupShoulderButtons(controlsLayout);
        }

        // SNES has 4 face buttons, NES and GBA have 2
        if (consoleType.equals("snes")) {
            showAllFaceButtons(controlsLayout);
        } else {
            hideXYButtons(controlsLayout);
        }

        // Apply button size from preferences
        applyButtonCustomization(controlsLayout);
    }

    private void setupDpad(ConstraintLayout controlsLayout) {
        ImageButton buttonUp = controlsLayout.findViewById(R.id.buttonUp);
        ImageButton buttonDown = controlsLayout.findViewById(R.id.buttonDown);
        ImageButton buttonLeft = controlsLayout.findViewById(R.id.buttonLeft);
        ImageButton buttonRight = controlsLayout.findViewById(R.id.buttonRight);

        setupButtonTouchListener(buttonUp, BUTTON_UP);
        setupButtonTouchListener(buttonDown, BUTTON_DOWN);
        setupButtonTouchListener(buttonLeft, BUTTON_LEFT);
        setupButtonTouchListener(buttonRight, BUTTON_RIGHT);
    }

    private void setupActionButtons(ConstraintLayout controlsLayout) {
        Button buttonA = controlsLayout.findViewById(R.id.buttonA);
        Button buttonB = controlsLayout.findViewById(R.id.buttonB);
        Button buttonX = controlsLayout.findViewById(R.id.buttonX);
        Button buttonY = controlsLayout.findViewById(R.id.buttonY);

        setupButtonTouchListener(buttonA, BUTTON_A);
        setupButtonTouchListener(buttonB, BUTTON_B);
        setupButtonTouchListener(buttonX, BUTTON_X);
        setupButtonTouchListener(buttonY, BUTTON_Y);
    }

    private void setupMenuButtons(ConstraintLayout controlsLayout) {
        Button buttonSelect = controlsLayout.findViewById(R.id.buttonSelect);
        Button buttonStart = controlsLayout.findViewById(R.id.buttonStart);

        setupButtonTouchListener(buttonSelect, BUTTON_SELECT);
        setupButtonTouchListener(buttonStart, BUTTON_START);
    }

    private void setupShoulderButtons(ConstraintLayout controlsLayout) {
        View container = controlsLayout.findViewById(R.id.shoulderButtonsContainer);
        container.setVisibility(View.VISIBLE);

        Button buttonL = controlsLayout.findViewById(R.id.buttonL);
        Button buttonR = controlsLayout.findViewById(R.id.buttonR);

        setupButtonTouchListener(buttonL, BUTTON_L);
        setupButtonTouchListener(buttonR, BUTTON_R);
    }

    private void showAllFaceButtons(ConstraintLayout controlsLayout) {
        Button buttonX = controlsLayout.findViewById(R.id.buttonX);
        Button buttonY = controlsLayout.findViewById(R.id.buttonY);

        buttonX.setVisibility(View.VISIBLE);
        buttonY.setVisibility(View.VISIBLE);
    }

    private void hideXYButtons(ConstraintLayout controlsLayout) {
        Button buttonX = controlsLayout.findViewById(R.id.buttonX);
        Button buttonY = controlsLayout.findViewById(R.id.buttonY);

        buttonX.setVisibility(View.GONE);
        buttonY.setVisibility(View.GONE);
    }

    private void setupButtonTouchListener(View button, final int buttonIndex) {
        button.setOnTouchListener((v, event) -> {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                buttonStates[buttonIndex] = 1;

                // If this button is remapped, also set the remapped button state
                int remappedButton = getRemappedButton(buttonIndex);
                if (remappedButton != -1) {
                    buttonStates[remappedButton] = 1;
                }
            } else if (event.getAction() == MotionEvent.ACTION_UP) {
                buttonStates[buttonIndex] = 0;

                // If this button is remapped, also clear the remapped button state
                int remappedButton = getRemappedButton(buttonIndex);
                if (remappedButton != -1) {
                    buttonStates[remappedButton] = 0;
                }
            }
            return false;
        });
    }

    /**
     * Apply button size and opacity settings from preferences
     */
    private void applyButtonCustomization(ConstraintLayout controlsLayout) {
        SharedPreferences prefs = context.getSharedPreferences("EmulatorPrefs", Context.MODE_PRIVATE);
        int buttonSize = prefs.getInt("button_size", 50); // Default 50%
        int buttonOpacity = prefs.getInt("button_opacity", 70); // Default 70%

        float scaleFactor = 0.5f + (buttonSize / 100f); // Scale from 0.5 to 1.5
        float alpha = buttonOpacity / 100f;

        // Apply to all control buttons
        applyScaleToButtonGroup(controlsLayout.findViewById(R.id.dpadContainer), scaleFactor, alpha);
        applyScaleToButtonGroup(controlsLayout.findViewById(R.id.actionButtonsContainer), scaleFactor, alpha);
        applyScaleToButtonGroup(controlsLayout.findViewById(R.id.menuButtonsContainer), scaleFactor, alpha);
        applyScaleToButtonGroup(controlsLayout.findViewById(R.id.shoulderButtonsContainer), scaleFactor, alpha);
    }

    /**
     * Apply scale to a button container and its children
     */
    private void applyScaleToButtonGroup(View container, float scale, float alpha) {
        if (container == null) return;

        container.setAlpha(alpha);

        if (container instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) container;
            for (int i = 0; i < group.getChildCount(); i++) {
                View child = group.getChildAt(i);
                ViewGroup.LayoutParams params = child.getLayoutParams();

                if (params.width > 0) {
                    params.width = (int) (params.width * scale);
                }
                if (params.height > 0) {
                    params.height = (int) (params.height * scale);
                }

                child.setLayoutParams(params);

                // Handle nested ViewGroups
                if (child instanceof ViewGroup) {
                    applyScaleToButtonGroup(child, scale, alpha);
                }
            }
        }
    }

    /**
     * Gets remapped button index based on preferences
     *
     * @param originalButton The original button index
     * @return The remapped button index, or -1 if not remapped
     */
    private int getRemappedButton(int originalButton) {
        String buttonKey = getButtonKey(originalButton);

        if (buttonKey == null) {
            return -1;
        }

        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        String mapping = prefs.getString(KEY_BUTTON_MAPPING_PREFIX + buttonKey, "Default");

        if ("Default".equals(mapping) || mapping == null || mapping.startsWith("Default (")) {
            return -1;
        }

        return getButtonIndex(mapping);
    }

    /**
     * Get button key name from index
     */
    private String getButtonKey(int buttonIndex) {
        switch (buttonIndex) {
            case BUTTON_UP:
                return "up";
            case BUTTON_DOWN:
                return "down";
            case BUTTON_LEFT:
                return "left";
            case BUTTON_RIGHT:
                return "right";
            case BUTTON_A:
                return "a";
            case BUTTON_B:
                return "b";
            case BUTTON_X:
                return "x";
            case BUTTON_Y:
                return "y";
            case BUTTON_L:
                return "l";
            case BUTTON_R:
                return "r";
            case BUTTON_SELECT:
                return "select";
            case BUTTON_START:
                return "start";
            default:
                return null;
        }
    }

    /**
     * Get button index from key name
     */
    private int getButtonIndex(String buttonName) {
        switch (buttonName.toLowerCase()) {
            case "up":
                return BUTTON_UP;
            case "down":
                return BUTTON_DOWN;
            case "left":
                return BUTTON_LEFT;
            case "right":
                return BUTTON_RIGHT;
            case "a":
                return BUTTON_A;
            case "b":
                return BUTTON_B;
            case "x":
                return BUTTON_X;
            case "y":
                return BUTTON_Y;
            case "l":
                return BUTTON_L;
            case "r":
                return BUTTON_R;
            case "select":
                return BUTTON_SELECT;
            case "start":
                return BUTTON_START;
            default:
                return -1;
        }
    }

    @Override
    public int[] getButtonStates() {
        return buttonStates;
    }
}