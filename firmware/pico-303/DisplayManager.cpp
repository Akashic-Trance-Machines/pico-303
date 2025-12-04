/**
 * @file DisplayManager.cpp
 * @brief OLED display management implementation
 */

#include "DisplayManager.h"

const unsigned char DisplayManager::image_ButtonLeft_bits[] = {0x10,0x30,0x70,0xf0,0x70,0x30,0x10};

const unsigned char DisplayManager::image_ButtonUp_bits[] = {0x10,0x38,0x7c,0xfe};

const unsigned char DisplayManager::image_SmallArrowDown_bits[] = {0xfe,0x7c,0x38,0x10};

DisplayManager::DisplayManager()
  : display(DISPLAY_W, DISPLAY_H, &Wire1, -1)
{
}

bool DisplayManager::begin() {
  // Initialize I2C on bus 1
  Wire1.setSDA(DISPLAY_I2C_SDA);
  Wire1.setSCL(DISPLAY_I2C_SCL);
  Wire1.begin();
  
  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDR)) {
    return false;
  }
  
  // Configure display
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.display();
  
  return true;
}

void DisplayManager::clear() {
  display.clearDisplay();
  display.display();
}

void DisplayManager::renderMenu(const Parameter& param) {
  display.clearDisplay();
  
  // Draw parameter name
  display.setTextSize(1);
  display.setCursor(6, 12);
  display.print(param.name);
  
  // Draw progress bar outline
  display.drawRoundRect(65, 12, 62, 7, 2, 1);
  
  // Scale bar width based on parameter value
  // Map param.value from [minVal, maxVal] to [2, 61] pixels
  int barWidth = map(param.value, param.minVal, param.maxVal, 2, 61);
  display.fillRect(66, 13, barWidth, 5, 1);

  display.drawBitmap(3, 0, image_ButtonUp_bits, 7, 4, 1);

  display.drawBitmap(3, 28, image_SmallArrowDown_bits, 7, 4, 1);


  
  display.display();
}

void DisplayManager::renderEdit(const Parameter& param) {
  display.clearDisplay();

  // Draw parameter name
  display.setTextSize(1);
  display.setCursor(6, 12);
  display.print(param.name);

  display.drawBitmap(0, 12, image_ButtonLeft_bits, 4, 7, 1);
  
  // Draw progress bar outline
  display.drawRoundRect(65, 12, 62, 7, 2, 1);
  
  // Scale bar width based on parameter value
  // Map param.value from [minVal, maxVal] to [2, 61] pixels
  int barWidth = map(param.value, param.minVal, param.maxVal, 2, 61);
  display.fillRect(66, 13, barWidth, 5, 1);
  
  display.setCursor(65, 3);
  display.print(param.value);
  
  display.display();
}