/**
 * @file UIManager.h
 * @brief UI state management and rotary encoder handling for pico-303
 * 
 * Manages the user interface state machine (Menu vs Edit mode), handles rotary
 * encoder input with quadrature decoding and acceleration, and maintains the
 * parameter list for the synth.
 */

#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <Arduino.h>

// Pin definitions
#define ENCODER_A_PIN   6
#define ENCODER_B_PIN   7
#define ENCODER_SW_PIN  8

// UI States
enum UIState {
  UI_MENU,    // Browsing parameters
  UI_EDIT     // Editing selected parameter
};

// Parameter structure
struct Parameter {
  const char* name;
  uint8_t cc;
  uint8_t value;
  uint8_t minVal;
  uint8_t maxVal;
};

class UIManager {
public:
  UIManager();
  
  /**
   * @brief Initialize encoder pins and UI state
   */
  void begin();
  
  /**
   * @brief Updates UI state.
   * Checks for encoder movement (from ISR) and button presses.
   * @return true if display needs redraw
   */
  bool update();

  // Static ISR for encoder interrupts
  static void handleEncoderInterrupt();
  
  /**
   * @brief Get current UI state
   */
  UIState getState() const { return state; }
  
  /**
   * @brief Get current parameter index (for menu navigation)
   */
  uint8_t getCurrentParamIndex() const { return currentParamIndex; }
  
  /**
   * @brief Get parameter at given index
   */
  const Parameter& getParameter(uint8_t index) const;
  
  /**
   * @brief Get total number of parameters
   */
  uint8_t getParameterCount() const { return paramCount; }
  
  /**
   * @brief Update parameter value from external source (e.g., MIDI CC)
   * @param cc Control change number
   * @param value New value (0-127)
   */
  void updateParameterValue(uint8_t cc, uint8_t value);
  
  /**
   * @brief Set callback for parameter changes
   * @param callback Function to call when parameter changes (cc, value)
   */
  void setParameterCallback(void (*callback)(uint8_t cc, uint8_t value)) {
    parameterCallback = callback;
  }

private:
  // Quadrature decoding table
  static const int8_t kQuadratureTable[4][4];
  
  // UI state
  UIState state;
  uint8_t currentParamIndex;
  
  // Encoder state (volatile for ISR)
  static volatile uint8_t lastEncoderState;
  static volatile uint32_t lastEncoderTime;
  static volatile int16_t encoderDelta; // Accumulated delta from ISR
  
  // Internal helpers
  static uint8_t calculateAcceleration(uint32_t deltaTime);
  bool readButton();
  
  // Button state
  bool lastButtonState;
  uint32_t lastButtonTime;
  
  // Parameters array
  static Parameter parameters[];
  static const uint8_t paramCount;
  
  // Callback for parameter changes
  void (*parameterCallback)(uint8_t cc, uint8_t value);
};

#endif // UIMANAGER_H
