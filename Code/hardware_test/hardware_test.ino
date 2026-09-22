/*
 * hardware_test.ino
 * Bring-up test for the Arduino Security/Access circuit (see MDs/aufbau.md).
 *
 * Purpose: check that every attached component responds. This is NOT the
 * final firmware - it only exercises the hardware and prints results to the
 * Serial Monitor (9600 baud) and to the OLED.
 *
 * Wiring used by this test (from aufbau.md):
 *   4x4 Keypad rows R1..R4 -> D9, D8, D7, D6
 *   4x4 Keypad cols C1..C4 -> D5, D4, D3, D2
 *   Passive buzzer signal  -> D10
 *   Motion sensor OUT      -> A2
 *   OLED I2C               -> SDA / SCL  (see note below)
 *   All VCC -> 5V, all GND -> GND
 *
 * ---------------------------------------------------------------------------
 * ASSUMPTIONS (not specified in aufbau.md - change if your parts differ):
 *   1. OLED is an SH1106 128x64, I2C address 0x3C.
 *   2. Keypad key layout is the common 4x4 matrix:
 *        1 2 3 A / 4 5 6 B / 7 8 9 C / * 0 # D
 *   3. Motion sensor OUT is active HIGH when motion is detected.
 *
 * NOTE ON I2C PINS:
 *   aufbau.md records OLED SDA -> A5 and SCL -> A4. On the Arduino Uno the
 *   hardware I2C bus is fixed: SDA = A4, SCL = A5. The Wire library (used by
 *   the OLED library) can only use those fixed pins. If the OLED test fails,
 *   the SDA/SCL wires are almost certainly swapped - move SDA to A4 and
 *   SCL to A5.
 *
 * Required libraries (Library Manager):
 *   - Keypad            by Mark Stanley / Alexander Brevig
 *   - Adafruit SH110X   (SH1106 / SH1107 controllers)
 *   - Adafruit GFX
 * ---------------------------------------------------------------------------
 */

#include <Wire.h>
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ----- Pin configuration --------------------------------------------------
const byte ROW_PINS[4] = {9, 8, 7, 6};   // R1, R2, R3, R4
const byte COL_PINS[4] = {5, 4, 3, 2};   // C1, C2, C3, C4

const byte BUZZER_PIN = 10;
const byte MOTION_PIN = A2;

// ----- OLED -------------------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ----- Keypad ---------------------------------------------------------
const char KEYS[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
Keypad keypad = Keypad(makeKeymap(KEYS),
                       (byte *)ROW_PINS, (byte *)COL_PINS, 4, 4);

// ----- Test state -------------------------------------------------------
bool oledOk = false;
int  keysSeen = 0;
char lastKey = ' ';

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }  // wait for Serial Monitor on boards that need it

  Serial.println();
  Serial.println(F("=== Hardware bring-up test ==="));

  pinMode(MOTION_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  testOled();
  testBuzzer();

  Serial.println(F("Setup done. Now testing keypad + motion sensor."));
  Serial.println(F("Press every key once. Wave a hand in front of the sensor."));
  showStatus("Press keys /", "wave at sensor");
}

void loop() {
  checkKeypad();
  checkMotion();
}

// ---------------------------------------------------------------------------
// OLED: initialise and draw a test pattern
// ---------------------------------------------------------------------------
void testOled() {
  Serial.print(F("[OLED] init at 0x"));
  Serial.print(OLED_ADDRESS, HEX);
  Serial.print(F(" ... "));

  if (display.begin(OLED_ADDRESS, true)) {
    oledOk = true;
    Serial.println(F("OK"));

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(F("OLED OK"));
    display.println(F("Hardware test"));
    display.drawRect(0, 20, SCREEN_WIDTH, 20, SH110X_WHITE);
    display.fillRect(2, 22, SCREEN_WIDTH - 4, 16, SH110X_WHITE);
    display.display();
    delay(1500);
  } else {
    oledOk = false;
    Serial.println(F("FAILED"));
    Serial.println(F("       -> check power and SDA=A4 / SCL=A5 (Uno fixed pins),"));
    Serial.println(F("          try address 0x3D, check the OLED controller type."));
  }
}

// ---------------------------------------------------------------------------
// Buzzer: short beep sequence, must be audible
// ---------------------------------------------------------------------------
void testBuzzer() {
  Serial.print(F("[BUZZER] beep test on D"));
  Serial.print(BUZZER_PIN);
  Serial.println(F(" ... listen for 3 tones"));

  const int freqs[3] = {1000, 1500, 2000};
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, freqs[i], 150);
    delay(250);
  }
  noTone(BUZZER_PIN);
  Serial.println(F("[BUZZER] done (confirm you heard it)"));
}

// ---------------------------------------------------------------------------
// Keypad: report each key press
// ---------------------------------------------------------------------------
void checkKeypad() {
  char key = keypad.getKey();
  if (key == NO_KEY) {
    return;
  }

  keysSeen++;
  lastKey = key;

  Serial.print(F("[KEYPAD] key = '"));
  Serial.print(key);
  Serial.print(F("'  (total presses: "));
  Serial.print(keysSeen);
  Serial.println(F(")"));

  // short feedback beep so keypad + buzzer are tested together
  tone(BUZZER_PIN, 1800, 40);

  char line1[17];
  char line2[17];
  snprintf(line1, sizeof(line1), "Key: %c", key);
  snprintf(line2, sizeof(line2), "Count: %d", keysSeen);
  showStatus(line1, line2);
}

// ---------------------------------------------------------------------------
// Motion sensor: report state changes only
// ---------------------------------------------------------------------------
void checkMotion() {
  static int lastState = -1;
  int state = digitalRead(MOTION_PIN);

  if (state != lastState) {
    lastState = state;
    Serial.print(F("[MOTION] "));
    Serial.println(state == HIGH ? F("motion DETECTED") : F("clear"));

    if (oledOk) {
      display.fillRect(0, 52, SCREEN_WIDTH, 12, SH110X_BLACK);
      display.setCursor(0, 54);
      display.print(state == HIGH ? F("MOTION!") : F("no motion"));
      display.display();
    }

    if (state == HIGH) {
      tone(BUZZER_PIN, 2500, 120);
    }
  }
}

// ---------------------------------------------------------------------------
// Helper: two short lines on the OLED (ignored if OLED failed)
// ---------------------------------------------------------------------------
void showStatus(const char *line1, const char *line2) {
  if (!oledOk) {
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  display.println(line2);
  display.display();
}
