/*
 * rfid_test.ino
 * Bring-up test for the RFID-RC522 reader. Reads the UID of every card that
 * is held to the reader and prints it over Serial in a form that can be
 * pasted straight into ALLOWED_UIDS in alarm_system.ino.
 *
 * Run this FIRST, before touching the main firmware:
 *   1. Open the Serial Monitor at 9600 baud.
 *   2. Check that the version line says 0x91 or 0x92.
 *   3. Hold each card / key fob to the reader and note the printed line.
 *
 * Wiring (RC522 -> Arduino Uno)
 *   SDA / SS  -> A0
 *   SCK       -> D13   (fixed, hardware SPI)
 *   MOSI      -> D11   (fixed, hardware SPI)
 *   MISO      -> D12   (fixed, hardware SPI)
 *   RST       -> A1
 *   GND       -> GND
 *   VCC       -> 3.3V  <-- NOT 5V, that destroys the module
 *   IRQ       -> not connected
 *
 * Libraries: MFRC522 (by GithubCommunity), SPI
 * Serial Monitor: 9600 baud
 */

#include <SPI.h>
#include <MFRC522.h>

const byte RFID_SS_PIN  = A0;
const byte RFID_RST_PIN = A1;

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  delay(50);                 // give the module time to come up

  // VersionReg answers 0x91 / 0x92 on a genuine RC522. 0x00 or 0xFF means
  // the reader is not talking to us at all.
  byte ver = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("[RFID] version 0x"));
  Serial.println(ver, HEX);

  if (ver == 0x00 || ver == 0xFF) {
    Serial.println(F("[RFID] no answer - check SS=A0, RST=A1, SCK/MOSI/MISO"));
    Serial.println(F("[RFID] and make sure VCC is on 3.3V, not 5V"));
  } else {
    Serial.println(F("=== ready - hold a card to the reader ==="));
  }

  Serial.print(F("free SRAM: "));
  Serial.println(freeRam());
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("UID ("));
  Serial.print(mfrc522.uid.size);
  Serial.print(F(" bytes): {"));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (i > 0) {
      Serial.print(F(", "));
    }
    Serial.print(F("0x"));
    if (mfrc522.uid.uidByte[i] < 0x10) {
      Serial.print('0');            // keep two hex digits
    }
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println(F("},"));

  // Halt the card so holding it still does not spam the output. It answers
  // again only after being taken away and presented once more.
  mfrc522.PICC_HaltA();
}

// free SRAM in bytes
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
