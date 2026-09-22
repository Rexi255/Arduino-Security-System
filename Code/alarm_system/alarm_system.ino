/*
 * alarm_system.ino
 * Mini access / alarm system for the LF07 project (see MDs/aufbau.md).
 *
 * Behaviour
 *   - DISARMED  : heart icon on the OLED, motion sensor is ignored.
 *   - EXIT DELAY: skull + countdown, time to leave the room after arming.
 *   - ARMED     : skull icon, motion on the sensor triggers the alarm.
 *   - ALARM     : flashing skull + countdown, loud siren for 30 s
 *                 (or until the PIN is entered / a known card is presented).
 *
 * Two ways to arm and disarm
 *   Keypad : PIN + '#'
 *   RFID   : hold a known card (see "RFID admin menu" below) to the reader
 *   Both toggle the same way: disarmed -> exit delay -> armed, and armed or
 *   alarm -> disarmed. Three wrong PINs or unknown cards raise the alarm.
 *
 * Keypad
 *   0-9  enter PIN digits (shown as '*' only)
 *   #    confirm
 *   *    clear the current entry
 *   A    while disarmed and idle: enter the admin PIN to open the admin menu
 *   B    while disarmed: soft-reset the RFID reader (see resetRfid() below -
 *        the RC522 clones can occasionally stop answering over SPI)
 *   C-D  unused
 *
 * RFID admin menu (Keypad 'A' + ADMIN_PIN + '#', only while disarmed)
 *   Cards are no longer hard-coded: they live in the Uno's internal EEPROM,
 *   so they survive re-uploading the sketch and can be managed without a PC.
 *     1  add a card   - hold the new card to the reader
 *     2  list / delete cards - '#' next, 'D' delete shown card, '*' back
 *     *  leave the admin menu
 *   A wrong admin PIN counts as a wrong try, same as a wrong PIN_CODE.
 *   The menu times out back to normal operation after ADMIN_IDLE_TIMEOUT_MS.
 *
 * Feedback tones (passive buzzer on D10)
 *   accepted    : two short bright notes
 *   rejected    : rough low buzz, 3 s
 *   3x rejected : full alarm
 *
 * A wrong PIN or unknown card also shows a brief X icon on the OLED for the
 * length of the rejected-buzz, on top of whatever else is on screen.
 *
 * Wiring (see CLAUDE.md - changed when the RFID reader was added)
 *   Keypad rows R1..R4 -> D9, D8, D7, D6
 *   Keypad cols C1..C4 -> D5, D4, D3, D2
 *   Passive buzzer     -> D10
 *   Motion sensor OUT  -> A2  (active HIGH; moved off D11, that is SPI MOSI)
 *   OLED SH1106 128x64 -> SDA = A4, SCL = A5, I2C address 0x3C
 *   RC522 SS -> A0, RST -> A1, MOSI -> D11, MISO -> D12, SCK -> D13
 *   RC522 VCC -> 3.3V, NOT 5V
 *
 * MEMORY NOTE
 *   The Uno has 2048 bytes of SRAM and the OLED frame buffer alone takes 1024.
 *   With the Keypad library on top only ~205 bytes were left, which is not
 *   enough once tone() interrupts hit while the display code is deep in the
 *   stack - the result were random resets and corrupted key input. This sketch
 *   therefore scans the keypad itself (see scanKeypad()) and keeps every string
 *   in flash. The MFRC522 library costs about 15 bytes of static SRAM (mostly
 *   the Uid struct), so there is still room; setup() prints the free SRAM and
 *   it should stay well above 250.
 *
 * Libraries: Adafruit SH110X, Adafruit GFX, MFRC522, SPI, EEPROM
 *            (Keypad library NOT needed)
 * Serial Monitor: 9600 baud
 */

#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <MFRC522.h>

// ---------------------------------------------------------------------------
// Settings - change these if needed
// ---------------------------------------------------------------------------
const char PIN_CODE[]  = "55559";                 // access code
const char ADMIN_PIN[] = "19283";                 // opens the RFID admin menu

const unsigned long ALARM_MS         = 30000UL;  // siren length
const unsigned long ERROR_TONE_MS    = 2000UL;   // wrong-PIN buzz length
const unsigned long EXIT_DELAY_MS    = 10000UL;  // time to leave after arming
const unsigned long ENTRY_TIMEOUT_MS = 10000UL;  // clear a half-typed PIN
const unsigned long ADMIN_IDLE_TIMEOUT_MS = 20000UL; // leave admin menu if idle
const unsigned long ADMIN_MSG_MS     = 1500UL;   // "Saved" / "Full" ... display time
const byte MAX_WRONG_TRIES = 3;                  // wrong tries before alarm
const byte MAX_ENTRY_LEN   = 8;                  // keypad input buffer
const byte DEBOUNCE_MS     = 20;                 // keypad debounce

// RFID cards that may arm / disarm the system are no longer hard-coded here:
// they are stored in the Uno's internal EEPROM and managed from the admin
// menu (keypad 'A' + ADMIN_PIN + '#'), see the UID storage section further
// down. DEFAULT_UIDS is only the factory list written to EEPROM the very
// first time the sketch runs on a blank chip - after that, edit cards from
// the menu, not here.
// These are 4-byte UIDs (MIFARE Classic cards and key fobs). If rfid_test
// reports 7 bytes instead, set UID_LEN to 7 and use 7 bytes per row.
const byte UID_LEN = 4;
const byte DEFAULT_UIDS[][UID_LEN] PROGMEM = {
  {0x9C, 0xEB, 0x1F, 0x49},   // key fob <-- REPLACE with your own UID
};
const byte DEFAULT_UID_COUNT = sizeof(DEFAULT_UIDS) / UID_LEN;

// EEPROM-backed UID storage. Deliberately small: the OLED frame buffer
// already eats most of the Uno's 2048 byte SRAM (see MEMORY NOTE above), and
// the school setup never needs more than a handful of cards.
const byte MAX_STORED_UIDS = 8;
const int  EEPROM_MAGIC_ADDR = 0;
const int  EEPROM_COUNT_ADDR = 1;
const int  EEPROM_DATA_ADDR  = 2;
const byte EEPROM_MAGIC = 0xA5;

byte storedUids[MAX_STORED_UIDS][UID_LEN];
byte storedUidCount = 0;

// How often the reader is polled. A poll with no card in front of it blocks
// for ~25 ms (the RC522 answer timeout), so polling on every loop pass would
// slow down the keypad and make the siren sweep choppy.
const unsigned long RFID_POLL_MS = 200UL;

// ---------------------------------------------------------------------------
// Pins
// ---------------------------------------------------------------------------
const byte ROW_PINS[4] = {9, 8, 7, 6};   // R1, R2, R3, R4
const byte COL_PINS[4] = {5, 4, 3, 2};   // C1, C2, C3, C4
const byte BUZZER_PIN = 10;
const byte MOTION_PIN = A2;              // moved: D11 is now SPI MOSI
const byte RFID_SS_PIN  = A0;            // RC522 SDA / SS
const byte RFID_RST_PIN = A1;            // RC522 RST
// SPI itself is fixed on the Uno: MOSI = D11, MISO = D12, SCK = D13

// keymap lives in flash - the Keypad library needed it in RAM
const char KEYS[4][4] PROGMEM = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ---------------------------------------------------------------------------
// OLED
// ---------------------------------------------------------------------------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOk = false;

// ---------------------------------------------------------------------------
// RFID reader
// ---------------------------------------------------------------------------
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);
bool rfidOk = false;
unsigned long lastRfidPoll = 0;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
enum SystemState { STATE_DISARMED, STATE_EXIT_DELAY, STATE_ARMED, STATE_ALARM, STATE_ADMIN };
SystemState state = STATE_DISARMED;

char entry[MAX_ENTRY_LEN + 1];
byte entryLen = 0;
unsigned long lastKeyMs = 0;
bool enteringAdmin = false;     // current entry[] is an admin PIN, not PIN_CODE

byte wrongTries = 0;
unsigned long stateStart = 0;   // start of exit delay / alarm
bool lastMotion = false;        // for rising-edge detection
bool displayDirty = true;

// ---------------------------------------------------------------------------
// Admin menu state (RFID cards, see uidAllowed()/addUid()/deleteUidAt())
// ---------------------------------------------------------------------------
enum AdminScreen { ADMIN_MENU, ADMIN_ADD_WAIT, ADMIN_LIST };
AdminScreen adminScreen = ADMIN_MENU;
byte adminListIndex = 0;
unsigned long lastAdminKeyMs = 0;
const __FlashStringHelper *adminMsg = nullptr;
unsigned long adminMsgUntil = 0;
unsigned long lastDraw = 0;

// Sound engine (non-blocking, so the keypad keeps working during a tone)
enum SoundId { SOUND_NONE, SOUND_OK, SOUND_ERROR, SOUND_ALARM };
SoundId sound = SOUND_NONE;
unsigned long soundStart = 0;
int currentFreq = -1;

void setup() {
  Serial.begin(9600);

  // SPI has to come up before the buzzer pin is set: SPI.begin() forces D10
  // (the AVR hardware SS pin) to OUTPUT HIGH, and D10 is our buzzer. Setting
  // the buzzer low afterwards leaves it in a defined, silent state.
  SPI.begin();
  mfrc522.PCD_Init();

  setupKeypad();
  pinMode(MOTION_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  entry[0] = '\0';

  oledOk = display.begin(OLED_ADDRESS, true);
  if (oledOk) {
    display.clearDisplay();
    display.display();
  } else {
    Serial.println(F("[OLED] not found - check SDA=A4 / SCL=A5 and address 0x3C"));
  }

  // VersionReg answers 0x91 / 0x92 on a genuine RC522. 0x00 or 0xFF means the
  // reader is not answering - wiring, or 5 V instead of 3.3 V on VCC.
  byte ver = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  rfidOk = (ver != 0x00 && ver != 0xFF);
  Serial.print(F("[RFID] version 0x"));
  Serial.println(ver, HEX);
  if (!rfidOk) {
    Serial.println(F("[RFID] reader not found - check SS=A0, RST=A1 and 3.3V"));
    Serial.println(F("[RFID] keypad still works"));
  }

  loadUidsFromEeprom();

  Serial.println(F("=== Alarm system ready (disarmed) ==="));
  Serial.println(F("PIN + '#' or a known RFID card arms / disarms."));
  Serial.print(F("stored RFID cards: "));
  Serial.println(storedUidCount);
  Serial.print(F("free SRAM: "));
  Serial.println(freeRam());

  lastMotion = (digitalRead(MOTION_PIN) == HIGH);
  displayDirty = true;
}

void loop() {
  if (state == STATE_ADMIN) {
    handleAdminKeypad();
    handleAdminRfid();
  } else {
    handleKeypad();
    handleRfid();
  }
  handleMotion();
  updateState();
  updateSound();
  updateDisplay();
}

// free SRAM in bytes - if this gets close to 0 the frame buffer gets corrupted
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// ---------------------------------------------------------------------------
// Keypad - own matrix scan instead of the Keypad library (saves ~120 byte RAM)
// ---------------------------------------------------------------------------
void setupKeypad() {
  for (byte i = 0; i < 4; i++) {
    pinMode(ROW_PINS[i], INPUT_PULLUP);
    digitalWrite(COL_PINS[i], LOW);   // set the level first, then release the
    pinMode(COL_PINS[i], INPUT);      // pin - avoids switching a pull-up on
  }
}

// key that is held down right now, 0 if none
char scanKeypad() {
  char found = 0;

  for (byte c = 0; c < 4 && found == 0; c++) {
    pinMode(COL_PINS[c], OUTPUT);     // pull this column low
    delayMicroseconds(5);             // let the line settle
    for (byte r = 0; r < 4; r++) {
      if (digitalRead(ROW_PINS[r]) == LOW) {
        found = pgm_read_byte(&KEYS[r][c]);
        break;
      }
    }
    pinMode(COL_PINS[c], INPUT);      // back to high-Z
  }
  return found;
}

// debounced, reports a key once when it is pressed down
char getKeyPress() {
  static char stableKey = 0;
  static char lastRead  = 0;
  static unsigned long lastChange = 0;

  char k = scanKeypad();

  if (k != lastRead) {                // still bouncing
    lastRead = k;
    lastChange = millis();
    return 0;
  }
  if (millis() - lastChange < DEBOUNCE_MS) {
    return 0;
  }
  if (k == stableKey) {               // nothing new
    return 0;
  }
  stableKey = k;
  return k;                           // 0 here means "key released"
}

// ---------------------------------------------------------------------------
// Keypad input
// ---------------------------------------------------------------------------
void handleKeypad() {
  // drop a half-typed PIN (or a lone 'A' admin request) after a while so
  // nothing stays on the screen and a later PIN entry is not misread as an
  // admin PIN
  if ((entryLen > 0 || enteringAdmin) && millis() - lastKeyMs > ENTRY_TIMEOUT_MS) {
    clearEntry();
    enteringAdmin = false;
    displayDirty = true;
  }

  char key = getKeyPress();
  if (key == 0) {
    return;
  }
  lastKeyMs = millis();

  Serial.print(F("[KEY] "));
  Serial.println(key);

  if (key >= '0' && key <= '9') {
    if (entryLen < MAX_ENTRY_LEN) {
      entry[entryLen++] = key;
      entry[entryLen] = '\0';
      clickBeep();
    }
    displayDirty = true;
  } else if (key == '*') {
    clearEntry();
    enteringAdmin = false;
    clickBeep();
    displayDirty = true;
  } else if (key == '#') {
    submitEntry();
  } else if (key == 'A' && state == STATE_DISARMED && entryLen == 0) {
    enteringAdmin = true;
    clickBeep();
    displayDirty = true;
  } else if (key == 'B' && state == STATE_DISARMED && entryLen == 0) {
    resetRfid();
  }
  // C-D are not used
}

void clearEntry() {
  entryLen = 0;
  entry[0] = '\0';
}

// Check the typed PIN (or, while enteringAdmin, the admin PIN) and act on it
void submitEntry() {
  if (enteringAdmin) {
    bool adminOk = (entryLen > 0 && strcmp(entry, ADMIN_PIN) == 0);
    enteringAdmin = false;
    clearEntry();
    displayDirty = true;

    if (adminOk) {
      Serial.println(F("[ADMIN] PIN correct, opening menu"));
      openAdminMenu();
    } else {
      Serial.println(F("[ADMIN] wrong PIN"));
      accessDenied(F("too many wrong admin PIN entries"));
    }
    return;
  }

  bool ok = (entryLen > 0 && strcmp(entry, PIN_CODE) == 0);

  Serial.print(F("[PIN] entered: "));
  Serial.println(entry);
  clearEntry();
  displayDirty = true;

  if (ok) {
    Serial.println(F("[PIN] correct"));
    accessGranted();
  } else {
    Serial.print(F("[PIN] wrong, try "));
    Serial.println(wrongTries + 1);
    accessDenied(F("too many wrong PIN entries"));
  }
}

// ---------------------------------------------------------------------------
// RFID - a known card does exactly what the correct PIN does
// ---------------------------------------------------------------------------

// Polls the reader; on a new card, leaves its UID in mfrc522.uid and returns
// true. Shared by normal operation and the admin "add card" screen.
bool readNewCardUid() {
  if (!rfidOk || millis() - lastRfidPoll < RFID_POLL_MS) {
    return false;
  }
  lastRfidPoll = millis();

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  Serial.print(F("[RFID] uid:"));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(' ');
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // HaltA sends the card to sleep, so leaving it on the reader does not
  // retrigger - it answers again only after being taken away and back.
  mfrc522.PICC_HaltA();
  return true;
}

// RC522 clones occasionally stop answering over SPI (VersionReg then reads
// 0x00/0xFF again). Re-running the reset/init sequence brings them back
// without a full board reset. Triggered manually via keypad 'B'.
void resetRfid() {
  Serial.println(F("[RFID] manual reset"));
  mfrc522.PCD_Reset();
  mfrc522.PCD_Init();

  byte ver = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  rfidOk = (ver != 0x00 && ver != 0xFF);
  Serial.print(F("[RFID] version 0x"));
  Serial.println(ver, HEX);

  if (rfidOk) {
    startSound(SOUND_OK);
  } else {
    Serial.println(F("[RFID] still not answering"));
    startSound(SOUND_ERROR);
  }
}

void handleRfid() {
  if (!readNewCardUid()) {
    return;
  }

  if (uidAllowed()) {
    Serial.println(F("[RFID] known card"));
    accessGranted();
  } else {
    Serial.println(F("[RFID] unknown card"));
    accessDenied(F("too many unknown cards"));
  }
}

// is the UID just read one of the cards stored in EEPROM?
bool uidAllowed() {
  if (mfrc522.uid.size != UID_LEN) {
    return false;                        // wrong card family, cannot match
  }
  return findStoredUid(mfrc522.uid.uidByte) != -1;
}

// index of the given UID in storedUids, or -1 if it is not stored
int findStoredUid(const byte *uid) {
  for (byte c = 0; c < storedUidCount; c++) {
    bool match = true;
    for (byte i = 0; i < UID_LEN; i++) {
      if (uid[i] != storedUids[c][i]) {
        match = false;
        break;
      }
    }
    if (match) {
      return c;
    }
  }
  return -1;
}

// ---------------------------------------------------------------------------
// UID storage (EEPROM) - see "Settings" section above for the layout
// ---------------------------------------------------------------------------
void loadUidsFromEeprom() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
    // blank / fresh chip - seed EEPROM with the factory default list
    storedUidCount = min(DEFAULT_UID_COUNT, MAX_STORED_UIDS);
    for (byte c = 0; c < storedUidCount; c++) {
      for (byte i = 0; i < UID_LEN; i++) {
        storedUids[c][i] = pgm_read_byte(&DEFAULT_UIDS[c][i]);
      }
    }
    saveUidsToEeprom();
    Serial.println(F("[EEPROM] blank chip, wrote factory default UIDs"));
    return;
  }

  storedUidCount = EEPROM.read(EEPROM_COUNT_ADDR);
  if (storedUidCount > MAX_STORED_UIDS) {
    Serial.println(F("[EEPROM] bad stored count, resetting to 0"));
    storedUidCount = 0;
    saveUidsToEeprom();
    return;
  }

  for (byte c = 0; c < storedUidCount; c++) {
    for (byte i = 0; i < UID_LEN; i++) {
      storedUids[c][i] = EEPROM.read(EEPROM_DATA_ADDR + c * UID_LEN + i);
    }
  }
}

// writes the magic byte, the count and every stored UID back to EEPROM.
// EEPROM.update() only actually writes a byte when its value changed, so
// repeated calls do not wear the EEPROM out.
void saveUidsToEeprom() {
  EEPROM.update(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  EEPROM.update(EEPROM_COUNT_ADDR, storedUidCount);
  for (byte c = 0; c < storedUidCount; c++) {
    for (byte i = 0; i < UID_LEN; i++) {
      EEPROM.update(EEPROM_DATA_ADDR + c * UID_LEN + i, storedUids[c][i]);
    }
  }
}

// adds a UID if there is room and it is not already stored
bool addUid(const byte *uid) {
  if (storedUidCount >= MAX_STORED_UIDS) {
    return false;
  }
  for (byte i = 0; i < UID_LEN; i++) {
    storedUids[storedUidCount][i] = uid[i];
  }
  storedUidCount++;
  saveUidsToEeprom();
  return true;
}

// removes the UID at the given index, shifting the rest down
void deleteUidAt(byte index) {
  if (index >= storedUidCount) {
    return;
  }
  for (byte c = index; c < storedUidCount - 1; c++) {
    for (byte i = 0; i < UID_LEN; i++) {
      storedUids[c][i] = storedUids[c + 1][i];
    }
  }
  storedUidCount--;
  saveUidsToEeprom();
}

// ---------------------------------------------------------------------------
// Access decision - shared by keypad and RFID
// ---------------------------------------------------------------------------
void accessGranted() {
  wrongTries = 0;
  displayDirty = true;

  switch (state) {
    case STATE_ALARM:
    case STATE_ARMED:
    case STATE_EXIT_DELAY:
      setState(STATE_DISARMED);
      Serial.println(F("[STATE] disarmed"));
      break;
    case STATE_DISARMED:
      setState(STATE_EXIT_DELAY);
      Serial.println(F("[STATE] arming - exit delay running"));
      break;
  }
  startSound(SOUND_OK);
}

void accessDenied(const __FlashStringHelper *reason) {
  wrongTries++;
  displayDirty = true;

  if (wrongTries >= MAX_WRONG_TRIES) {
    wrongTries = 0;
    triggerAlarm(reason);
  } else {
    startSound(SOUND_ERROR);
  }
}

// ---------------------------------------------------------------------------
// Admin menu - manage RFID cards from the keypad/OLED, see CLAUDE.md
// ---------------------------------------------------------------------------
void openAdminMenu() {
  adminScreen = ADMIN_MENU;
  adminListIndex = 0;
  adminMsg = nullptr;
  lastAdminKeyMs = millis();
  setState(STATE_ADMIN);
  startSound(SOUND_OK);
}

void showAdminMsg(const __FlashStringHelper *msg) {
  adminMsg = msg;
  adminMsgUntil = millis() + ADMIN_MSG_MS;
  displayDirty = true;
}

void handleAdminKeypad() {
  // leave the menu on its own if nobody touches it for a while
  if (millis() - lastAdminKeyMs > ADMIN_IDLE_TIMEOUT_MS) {
    Serial.println(F("[ADMIN] idle timeout, closing menu"));
    setState(STATE_DISARMED);
    return;
  }

  char key = getKeyPress();
  if (key == 0) {
    return;
  }
  lastAdminKeyMs = millis();

  Serial.print(F("[ADMIN KEY] "));
  Serial.println(key);

  switch (adminScreen) {
    case ADMIN_MENU:
      if (key == '1') {
        adminScreen = ADMIN_ADD_WAIT;
        clickBeep();
        displayDirty = true;
      } else if (key == '2') {
        adminScreen = ADMIN_LIST;
        adminListIndex = 0;
        clickBeep();
        displayDirty = true;
      } else if (key == '*') {
        Serial.println(F("[ADMIN] menu closed"));
        setState(STATE_DISARMED);
      }
      break;

    case ADMIN_ADD_WAIT:
      if (key == '*') {
        adminScreen = ADMIN_MENU;
        clickBeep();
        displayDirty = true;
      }
      break;

    case ADMIN_LIST:
      if (key == '*') {
        adminScreen = ADMIN_MENU;
        clickBeep();
        displayDirty = true;
      } else if (key == '#') {
        if (storedUidCount > 0) {
          adminListIndex = (adminListIndex + 1) % storedUidCount;
        }
        clickBeep();
        displayDirty = true;
      } else if (key == 'D' && storedUidCount > 0) {
        deleteUidAt(adminListIndex);
        if (adminListIndex >= storedUidCount && storedUidCount > 0) {
          adminListIndex = storedUidCount - 1;
        }
        Serial.println(F("[ADMIN] card deleted"));
        showAdminMsg(F("Deleted"));
      }
      break;
  }
}

// only active on the "add card" screen; a scanned card is stored or rejected
void handleAdminRfid() {
  if (adminScreen != ADMIN_ADD_WAIT || !readNewCardUid()) {
    return;
  }
  lastAdminKeyMs = millis();   // a scan counts as activity too

  if (mfrc522.uid.size != UID_LEN) {
    showAdminMsg(F("Bad card"));
  } else if (findStoredUid(mfrc522.uid.uidByte) != -1) {
    showAdminMsg(F("Known"));
  } else if (addUid(mfrc522.uid.uidByte)) {
    Serial.println(F("[ADMIN] card added"));
    showAdminMsg(F("Saved"));
  } else {
    showAdminMsg(F("Full"));
  }
  adminScreen = ADMIN_MENU;
}

// ---------------------------------------------------------------------------
// Motion sensor - only relevant while armed
// ---------------------------------------------------------------------------
void handleMotion() {
  bool motion = (digitalRead(MOTION_PIN) == HIGH);

  // rising edge only, so one long detection does not retrigger endlessly
  if (motion && !lastMotion && state == STATE_ARMED) {
    triggerAlarm(F("motion detected"));
  }
  lastMotion = motion;
}

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------
void setState(SystemState next) {
  state = next;
  stateStart = millis();
  displayDirty = true;

  if (next != STATE_ALARM) {
    stopSound();
  }
  if (next == STATE_ARMED) {
    // ignore whatever the sensor sees at the moment of arming
    lastMotion = (digitalRead(MOTION_PIN) == HIGH);
  }
}

void triggerAlarm(const __FlashStringHelper *reason) {
  Serial.print(F("[ALARM] "));
  Serial.println(reason);
  setState(STATE_ALARM);
  startSound(SOUND_ALARM);
}

void updateState() {
  unsigned long now = millis();

  if (state == STATE_EXIT_DELAY) {
    // one short beep per second while the exit delay runs
    static unsigned long lastBeepSec = 999;
    unsigned long sec = (now - stateStart) / 1000UL;
    if (sound == SOUND_NONE && sec != lastBeepSec) {
      lastBeepSec = sec;
      tone(BUZZER_PIN, 1200, 60);
      currentFreq = -1;
    }
    if (now - stateStart >= EXIT_DELAY_MS) {
      setState(STATE_ARMED);
      Serial.println(F("[STATE] armed"));
    }
  } else if (state == STATE_ALARM) {
    if (now - stateStart >= ALARM_MS) {
      // siren time is over - stay armed and wait for the next event
      stopSound();
      setState(STATE_ARMED);
      Serial.println(F("[STATE] alarm finished - armed again"));
    }
  }
}

// ---------------------------------------------------------------------------
// Sound engine (non-blocking)
// ---------------------------------------------------------------------------
void startSound(SoundId id) {
  sound = id;
  soundStart = millis();
  currentFreq = -1;
}

void stopSound() {
  sound = SOUND_NONE;
  noTone(BUZZER_PIN);
  currentFreq = -1;
  displayDirty = true;   // e.g. clears the X icon once the error buzz ends
}

// only restart tone() when the frequency really changes (avoids clicking)
void setFreq(int freq) {
  if (freq == currentFreq) {
    return;
  }
  currentFreq = freq;
  if (freq <= 0) {
    noTone(BUZZER_PIN);
  } else {
    tone(BUZZER_PIN, freq);
  }
}

// short key click, only when no other sound is playing
void clickBeep() {
  if (sound == SOUND_NONE) {
    tone(BUZZER_PIN, 2000, 25);
    currentFreq = -1;
  }
}

void updateSound() {
  if (sound == SOUND_NONE) {
    return;
  }
  unsigned long t = millis() - soundStart;

  switch (sound) {
    case SOUND_OK:
      // two short bright notes: G6 then C7
      if (t < 110) {
        setFreq(1568);
      } else if (t < 140) {
        setFreq(0);
      } else if (t < 340) {
        setFreq(2093);
      } else {
        stopSound();
      }
      break;

    case SOUND_ERROR:
      // rough low buzz for 3 s (fast switching between two low tones)
      if (t >= ERROR_TONE_MS) {
        stopSound();
      } else {
        setFreq(((t / 35) % 2) ? 95 : 135);
      }
      break;

    case SOUND_ALARM: {
      // shrill siren sweep around the buzzer resonance (loudest range)
      unsigned long phase = t % 500UL;               // 500 ms up/down cycle
      unsigned long step  = (phase % 250UL) / 10UL;  // 0..24
      int freq = (phase < 250UL) ? (2500 + (int)step * 50)
                                 : (3700 - (int)step * 50);
      setFreq(freq);
      break;
    }

    default:
      stopSound();
      break;
  }
}

// ---------------------------------------------------------------------------
// Display - icons only, no status text
// ---------------------------------------------------------------------------
void updateDisplay() {
  if (!oledOk) {
    return;
  }
  // alarm and exit delay animate; an admin message needs a redraw once it
  // expires even without a key press, everything else redraws on change only
  bool animated = (state == STATE_ALARM || state == STATE_EXIT_DELAY ||
                    (state == STATE_ADMIN && adminMsg != nullptr));
  if (!displayDirty && !(animated && millis() - lastDraw >= 250)) {
    return;
  }
  displayDirty = false;
  lastDraw = millis();

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  // while a PIN is being typed the screen shows only the masked entry
  if (entryLen > 0) {
    drawEntry();
    display.display();
    return;
  }

  // wrong PIN / unknown card: show an X for the length of the reject buzz,
  // instead of whatever icon the current state would normally draw
  if (sound == SOUND_ERROR) {
    drawCross(64, 32, SH110X_WHITE);
    display.display();
    return;
  }

  switch (state) {
    case STATE_DISARMED:
      drawHeart(64, 30, SH110X_WHITE);
      break;

    case STATE_EXIT_DELAY:
      drawSkull(64, 24, SH110X_WHITE, SH110X_BLACK);
      drawSeconds(secondsLeft(EXIT_DELAY_MS), 2, 46);
      break;

    case STATE_ARMED:
      drawSkull(64, 34, SH110X_WHITE, SH110X_BLACK);
      break;

    case STATE_ALARM: {
      // flip the whole screen every 250 ms so it is impossible to miss
      bool flash = ((millis() / 250UL) % 2) == 0;
      uint16_t fg = flash ? SH110X_BLACK : SH110X_WHITE;
      uint16_t bg = flash ? SH110X_WHITE : SH110X_BLACK;

      if (flash) {
        display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
      }
      drawSkull(64, 26, fg, bg);
      display.setTextColor(fg);
      drawSeconds(secondsLeft(ALARM_MS), 2, 47);
      break;
    }

    case STATE_ADMIN:
      drawAdminScreen();
      break;
  }

  display.display();
}

// admin menu - the only place this sketch shows status text instead of icons
void drawAdminScreen() {
  display.setTextSize(1);

  if (adminMsg != nullptr) {
    if (millis() >= adminMsgUntil) {
      adminMsg = nullptr;   // expired - fall through to the normal screen below
    } else {
      display.setCursor(4, 28);
      display.print(adminMsg);
      return;
    }
  }

  switch (adminScreen) {
    case ADMIN_MENU:
      display.setCursor(4, 4);
      display.print(F("ADMIN"));
      display.setCursor(4, 24);
      display.print(F("1 Add card"));
      display.setCursor(4, 36);
      display.print(F("2 List/Del"));
      display.setCursor(4, 52);
      display.print(F("* Exit"));
      break;

    case ADMIN_ADD_WAIT:
      display.setCursor(4, 24);
      display.print(F("Scan card..."));
      display.setCursor(4, 52);
      display.print(F("* Cancel"));
      break;

    case ADMIN_LIST:
      if (storedUidCount == 0) {
        display.setCursor(4, 28);
        display.print(F("Empty"));
      } else {
        display.setCursor(4, 4);
        display.print(F("Card "));
        display.print(adminListIndex + 1);
        display.print('/');
        display.print(storedUidCount);
        display.setCursor(4, 24);
        printUidHex(storedUids[adminListIndex]);
      }
      display.setCursor(4, 52);
      display.print(F("# Next D Del * Back"));
      break;
  }
}

// prints a UID as space-separated hex byte pairs, e.g. "9C EB 1F 49"
void printUidHex(const byte *uid) {
  for (byte i = 0; i < UID_LEN; i++) {
    if (i > 0) {
      display.print(' ');
    }
    if (uid[i] < 0x10) {
      display.print('0');
    }
    display.print(uid[i], HEX);
  }
}

// masked PIN input, centred
void drawEntry() {
  byte size = (entryLen <= 6) ? 3 : 2;   // 8 chars at size 3 would not fit
  int w = entryLen * 6 * size;
  display.setTextSize(size);
  display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - 8 * size) / 2);
  for (byte i = 0; i < entryLen; i++) {
    display.print('*');
  }
}

// remaining seconds of the running state, rounded up
unsigned long secondsLeft(unsigned long total) {
  unsigned long done = millis() - stateStart;
  if (done >= total) {
    return 0;
  }
  return (total - done + 999UL) / 1000UL;
}

// centred "12s" countdown - printed digit by digit instead of snprintf
void drawSeconds(unsigned long secs, byte size, int y) {
  int chars = (secs >= 10) ? 3 : 2;   // digits + 's'
  display.setTextSize(size);
  display.setCursor((SCREEN_WIDTH - chars * 6 * size) / 2, y);
  display.print(secs);
  display.print('s');
}

// ---------------------------------------------------------------------------
// Icons (drawn with GFX primitives, no bitmaps needed)
// Skull is 38 x 40 px around (cx, cy): cy - 22 .. cy + 17
// Heart is 34 x 34 px around (cx, cy): cy - 15 .. cy + 18
// ---------------------------------------------------------------------------
void drawSkull(int cx, int cy, uint16_t fg, uint16_t bg) {
  display.fillRoundRect(cx - 19, cy - 22, 38, 32, 12, fg);  // cranium
  display.fillRoundRect(cx - 11, cy + 4, 22, 13, 4, fg);    // jaw

  display.fillCircle(cx - 9, cy - 7, 6, bg);                // eyes
  display.fillCircle(cx + 9, cy - 7, 6, bg);
  display.fillTriangle(cx, cy - 1, cx - 3, cy + 4, cx + 3, cy + 4, bg);

  display.drawFastHLine(cx - 11, cy + 5, 22, bg);           // teeth
  for (int i = 0; i < 4; i++) {
    display.drawFastVLine(cx - 7 + i * 5, cy + 6, 11, bg);
  }
}

void drawHeart(int cx, int cy, uint16_t fg) {
  display.fillCircle(cx - 8, cy - 6, 9, fg);
  display.fillCircle(cx + 8, cy - 6, 9, fg);
  display.fillTriangle(cx - 16, cy - 1, cx + 16, cy - 1, cx, cy + 18, fg);
}

// bold X, ~40x40 px around (cx, cy) - wrong PIN / unknown card
void drawCross(int cx, int cy, uint16_t fg) {
  for (int i = -2; i <= 2; i++) {
    display.drawLine(cx - 20, cy - 20 + i, cx + 20, cy + 20 + i, fg);
    display.drawLine(cx - 20, cy + 20 + i, cx + 20, cy - 20 + i, fg);
  }
}
