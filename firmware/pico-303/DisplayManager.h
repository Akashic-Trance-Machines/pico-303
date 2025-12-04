/**
 * @file DisplayManager.h
 * @brief OLED display management for pico-303 UI
 * 
 * Handles SSD1306 OLED display initialization and rendering of menu and edit screens.
 */

#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "UIManager.h"

// Display configuration moved to main sketch
// #define DISPLAY_I2C_BUS  1
// #define DISPLAY_I2C_SDA  2
// #define DISPLAY_I2C_SCL  3
#define DISPLAY_I2C_ADDR 0x3C
#define DISPLAY_W 128
#define DISPLAY_H 32

class DisplayManager {
public:
  DisplayManager();
  
  /**
   * @brief Initialize I2C and OLED display
   * @param sda I2C SDA pin
   * @param scl I2C SCL pin
   * @return true if initialization successful
   */
  bool begin(uint8_t sda, uint8_t scl);
  
  /**
   * @brief Render menu state (parameter name with arrows)
   * @param param Parameter to display
   */
  void renderMenu(const Parameter& param);
  
  /**
   * @brief Render edit state (parameter name and value)
   * @param param Parameter being edited
   */
  void renderEdit(const Parameter& param);
  
  /**
   * @brief Clear the display
   */
  void clear();

private:
  Adafruit_SSD1306 display;
  
  // Arrow bitmaps (5x7 pixels)
  static const unsigned char image_ButtonLeft_bits[];
  static const unsigned char image_ButtonUp_bits[];
  static const unsigned char image_SmallArrowDown_bits[];
};

#endif // DISPLAYMANAGER_H
