/*
 * display_test.ino
 * Isolated OLED test - no keypad, no buzzer, no motion sensor.
 *
 * Purpose: find out whether the OLED itself can draw the icons and text used
 * by alarm_system.ino. If everything looks correct here but is garbled in the
 * main sketch, the problem is free SRAM, not the display.
 *
 * The sketch cycles through 4 screens (2 s each) and prints the free SRAM to
 * the Serial Monitor (9600 baud) before every screen.
 *
 *   Screen 1: frame + text size 1 and 2
 *   Screen 2: heart icon
 *   Screen 3: skull icon
 *   Screen 4: masked PIN "****" + status line
 *
 * OLED: SH1106 128x64, I2C address 0x3C, SDA = A4, SCL = A5.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

byte step = 0;

// free SRAM in bytes - anything below ~250 is dangerous on the Uno
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("=== OLED display test ==="));

  if (!display.begin(OLED_ADDRESS, true)) {
    Serial.println(F("[OLED] begin() FAILED"));
    Serial.println(F("  -> check 5V/GND, SDA=A4, SCL=A5, address 0x3C or 0x3D"));
    while (true) {
      ;  // nothing else to do
    }
  }
  Serial.println(F("[OLED] begin() ok"));
  display.clearDisplay();
  display.display();
}

void loop() {
  Serial.print(F("screen "));
  Serial.print(step + 1);
  Serial.print(F("  free SRAM = "));
  Serial.println(freeRam());

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  switch (step) {
    case 0:  // frame and text - checks the basic pixel mapping
      display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
      display.setTextSize(1);
      display.setCursor(4, 6);
      display.print(F("Text size 1 - ok?"));
      display.setTextSize(2);
      display.setCursor(4, 26);
      display.print(F("SCHARF"));
      display.setTextSize(1);
      display.setCursor(4, 50);
      display.print(F("frame must be full"));
      break;

    case 1:  // heart = disarmed icon
      drawHeart(64, 26);
      display.setTextSize(2);
      display.setCursor(4, 49);
      display.print(F("UNSCHARF"));
      break;

    case 2:  // skull = armed icon
      drawSkull(64, 24);
      display.setTextSize(2);
      display.setCursor(28, 49);
      display.print(F("SCHARF"));
      break;

    case 3:  // masked PIN entry
      drawSkull(64, 24);
      display.setTextSize(2);
      display.setCursor(40, 49);
      display.print(F("****"));
      break;
  }

  display.display();
  delay(2000);

  step++;
  if (step > 3) {
    step = 0;
  }
}

// ---------------------------------------------------------------------------
// Icons - same code as in alarm_system.ino
// ---------------------------------------------------------------------------
void drawSkull(int cx, int cy) {
  display.fillRoundRect(cx - 19, cy - 22, 38, 32, 12, SH110X_WHITE);  // cranium
  display.fillRoundRect(cx - 11, cy + 4, 22, 13, 4, SH110X_WHITE);    // jaw

  display.fillCircle(cx - 9, cy - 7, 6, SH110X_BLACK);                // eyes
  display.fillCircle(cx + 9, cy - 7, 6, SH110X_BLACK);
  display.fillTriangle(cx, cy - 1, cx - 3, cy + 4, cx + 3, cy + 4, SH110X_BLACK);

  display.drawFastHLine(cx - 11, cy + 5, 22, SH110X_BLACK);           // teeth
  for (int i = 0; i < 4; i++) {
    display.drawFastVLine(cx - 7 + i * 5, cy + 6, 11, SH110X_BLACK);
  }
}

void drawHeart(int cx, int cy) {
  display.fillCircle(cx - 8, cy - 6, 9, SH110X_WHITE);
  display.fillCircle(cx + 8, cy - 6, 9, SH110X_WHITE);
  display.fillTriangle(cx - 16, cy - 1, cx + 16, cy - 1, cx, cy + 18, SH110X_WHITE);
}
