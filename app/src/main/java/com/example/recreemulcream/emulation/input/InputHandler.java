package com.example.recreemulcream.emulation.input;

/**
 * Interface for processing input for emulators
 */
public interface InputHandler {
    // Button indices
    int BUTTON_UP = 0;
    int BUTTON_DOWN = 1;
    int BUTTON_LEFT = 2;
    int BUTTON_RIGHT = 3;
    int BUTTON_A = 4;
    int BUTTON_B = 5;
    int BUTTON_X = 6;
    int BUTTON_Y = 7;
    int BUTTON_L = 8;
    int BUTTON_R = 9;
    int BUTTON_SELECT = 10;
    int BUTTON_START = 11;

    /**
     * Get the current state of all buttons
     *
     * @return Array of button states (1 = pressed, 0 = released)
     */
    int[] getButtonStates();
}