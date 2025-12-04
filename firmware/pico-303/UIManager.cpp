/**
 * @file UIManager.cpp
 * @brief UI state management and rotary encoder handling implementation
 */

#include "UIManager.h"

// Quadrature decoding lookup table
// [previous state][current state] -> delta (-1, 0, +1)
const int8_t UIManager::kQuadratureTable[4][4] = {
  { 0, +1, -1,  0},  // 00 -> 00, 01, 10, 11
  {-1,  0,  0, +1},  // 01 -> 00, 01, 10, 11
  {+1,  0,  0, -1},  // 10 -> 00, 01, 10, 11
  { 0, -1, +1,  0}   // 11 -> 00, 01, 10, 11
};

// Static member definitions
volatile uint8_t UIManager::lastEncoderState = 0;
volatile uint32_t UIManager::lastEncoderTime = 0;
volatile int16_t UIManager::encoderDelta = 0;
uint8_t UIManager::pinA = 0;
uint8_t UIManager::pinB = 0;
uint8_t UIManager::pinSW = 0;

// Parameter definitions (24 parameters from pico-303)
Parameter UIManager::parameters[] = {
  {"Volume",       7,   76,  0, 127},  // CC7 - default ~60% volume
  {"Wave ",  18,   0,   0, 127},  // CC18
  {"Pitch",   16,   64,  0, 127},  // CC16 - 64 = center (0 semitones)
  {"Cutoff",      74,   64,  0, 127},  // CC74
  {"Res",   71,   0,   0, 127},  // CC71
  {"Env",     17,   64,  0, 127},  // CC17
  {"Decay",       75,   64,  0, 127},  // CC75
  {"Accent",  15,   64,  0, 127},  // CC15
  {"SubOsc",   14,   0,   0, 127},  // CC14
  {"Dist On",     80,   0,   0, 127},  // CC80 - >63 = on
  {"Dist Mode",   77,   0,   0, 4},    // CC77 - 5 modes (0-4)
  {"Dist Amt",    78,   0,   0, 127},  // CC78
  {"Dist Mix",    79,   0,   0, 127},  // CC79
  {"Dly Time",  81,   32,  0, 127},  // CC81
  {"Dly Fdbk",    82,   64,  0, 127},  // CC82
  {"Dly Sync",    86,   32,  0, 127},  // CC86
  {"Dly L Div",   91,   32,  0, 127},  // CC91
  {"Dly R Div",   92,   32,  0, 127},  // CC92
  {"Dly L Mod",   93,   0,   0, 2},    // CC93 - 3 modes (0-2)
  {"Dly R Mod",   94,   0,   0, 2},    // CC94 - 3 modes (0-2)
  {"Dly Mix",     83,   38,  0, 127},  // CC83
  {"Glide",      100,   64,  0, 127}   // CC100
};

const uint8_t UIManager::paramCount = sizeof(UIManager::parameters) / sizeof(Parameter);

UIManager::UIManager()
  : state(UI_MENU)
  , currentParamIndex(0)
  , lastButtonState(false)
  , lastButtonTime(0)
  , parameterCallback(nullptr)
{
}

void UIManager::begin(uint8_t pinA, uint8_t pinB, uint8_t pinSW) {
  UIManager::pinA = pinA;
  UIManager::pinB = pinB;
  UIManager::pinSW = pinSW;

  // Configure encoder pins
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  pinMode(pinSW, INPUT_PULLUP);
  
  // Read initial encoder state
  uint8_t a = digitalRead(pinA);
  uint8_t b = digitalRead(pinB);
  lastEncoderState = (a << 1) | b;
  lastEncoderTime = millis();
  
  // Attach interrupts for encoder pins
  attachInterrupt(digitalPinToInterrupt(pinA), handleEncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinB), handleEncoderInterrupt, CHANGE);
  
  // Read initial button state
  lastButtonState = digitalRead(pinSW);
  lastButtonTime = millis();
}

bool UIManager::update() {
  bool needsRedraw = false;
  
  // Handle encoder rotation (Atomic read from ISR)
  noInterrupts();
  int16_t delta = encoderDelta;
  encoderDelta = 0;
  interrupts();
  
  if (delta != 0) {
    needsRedraw = true;
    
    if (state == UI_MENU) {
      // Navigate menu
      int16_t newIndex = (int16_t)currentParamIndex + delta;
      
      // Wrap around
      if (newIndex < 0) {
        newIndex = paramCount - 1;
      } else if (newIndex >= paramCount) {
        newIndex = 0;
      }
      
      currentParamIndex = (uint8_t)newIndex;
    } 
    else if (state == UI_EDIT) {
      // Edit parameter value
      Parameter& param = parameters[currentParamIndex];
      int16_t newValue = (int16_t)param.value + delta;
      
      // Clamp to min/max
      if (newValue < param.minVal) {
        newValue = param.minVal;
      } else if (newValue > param.maxVal) {
        newValue = param.maxVal;
      }
      
      param.value = (uint8_t)newValue;
      
      // Trigger callback if parameter changed
      if (parameterCallback) {
        parameterCallback(param.cc, param.value);
      }
    }
  }
  
  // Handle button press
  if (readButton()) {
    needsRedraw = true;
    
    // Toggle between menu and edit mode
    if (state == UI_MENU) {
      state = UI_EDIT;
    } else {
      state = UI_MENU;
    }
  }
  
  return needsRedraw;
}

void UIManager::handleEncoderInterrupt() {
  // Read current encoder pins
  uint8_t a = digitalRead(pinA);
  uint8_t b = digitalRead(pinB);
  uint8_t currentState = (a << 1) | b;
  
  // Look up delta from quadrature table
  int8_t rawDelta = kQuadratureTable[lastEncoderState][currentState];
  lastEncoderState = currentState;
  
  if (rawDelta != 0) {
    // Accumulator for 2-step-per-detent encoders
    static int8_t stepAccumulator = 0;
    stepAccumulator += rawDelta;
    
    // Every 2 steps = 1 actual movement (for 2-step encoders)
    if (abs(stepAccumulator) >= 2) {
      int8_t direction = (stepAccumulator > 0) ? 1 : -1;
      stepAccumulator = 0;  // Reset accumulator
      
      uint32_t now = millis();
      uint32_t deltaTime = now - lastEncoderTime;
      lastEncoderTime = now;
      
      // Apply acceleration based on rotation speed
      uint8_t multiplier = calculateAcceleration(deltaTime);
      encoderDelta += direction * multiplier;
    }
  }
}

uint8_t UIManager::calculateAcceleration(uint32_t deltaTime) {
  // Acceleration based on rotation speed
  // More conservative thresholds to ensure we don't miss steps
  if (deltaTime < 15) {
    return 4;  // Very fast
  } else if (deltaTime < 30) {
    return 2;  // Fast
  } else {
    return 1;  // Normal/slow
  }
}

bool UIManager::readButton() {
  bool currentState = digitalRead(pinSW);
  uint32_t now = millis();
  
  // Button is active low (pressed = LOW)
  // Improved debouncing: require stable state for longer period
  if (currentState == LOW && lastButtonState == HIGH) {
    // Potential button press detected
    if (now - lastButtonTime > 100) {  // Increased to 100ms debounce
      lastButtonState = currentState;
      lastButtonTime = now;
      return true;  // Button was pressed
    }
  } else if (currentState == HIGH && lastButtonState == LOW) {
    // Button released
    if (now - lastButtonTime > 100) {  // Also debounce release
      lastButtonState = currentState;
      lastButtonTime = now;
    }
  }
  
  return false;
}

const Parameter& UIManager::getParameter(uint8_t index) const {
  if (index >= paramCount) {
    index = 0;  // Safety fallback
  }
  return parameters[index];
}

void UIManager::updateParameterValue(uint8_t cc, uint8_t value) {
  // Find parameter by CC number and update its value
  for (uint8_t i = 0; i < paramCount; i++) {
    if (parameters[i].cc == cc) {
      parameters[i].value = value;
      break;
    }
  }
}
