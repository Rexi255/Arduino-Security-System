# Arduino Security/Access Circuit — Project Specification

## Purpose

Use this document as the authoritative hardware specification for writing the complete Arduino firmware for this project with Claude Code.

The firmware must be based only on the components, pins, and wiring documented here. Do not invent components, pins, sensors, display controllers, keypad layouts, I²C addresses, or system behavior without clearly identifying the assumption.

Before writing final code, identify any missing hardware information and verify suspicious or nonstandard wiring.

---

## Project Status (updated 2026-09-15)

This document was the original hardware export, written before the firmware
existed and before the RFID reader was added. It has since been updated to
match the finished build. **For anything not covered here, the pin table in
[../CLAUDE.md](../CLAUDE.md) is authoritative.**

- Microcontroller: Arduino Uno R3
- Firmware status: complete, see [../Code/alarm_system/alarm_system.ino](../Code/alarm_system/alarm_system.ino).
- The project now also includes an RFID-RC522 reader (see component 6 below)
  as a second way to arm/disarm, alongside the keypad PIN.
- The motion sensor was moved from Pin 11 to **Pin A2**, because Pin 11 is the
  hardware SPI MOSI line needed by the RC522.
- The OLED I²C pins were originally recorded reversed (SDA on A5, SCL on A4).
  The real build uses the Uno's fixed hardware I²C pins correctly: **SDA = A4,
  SCL = A5**. The wiring table below reflects the corrected, actual wiring.

---

## Components

### 1. Arduino Uno R3

- Component name: `Arduino Uno R3`
- Instance label: `Arduino Uno R3`
- Instance ID: `062c9730-3b06-46d1-a97e-57ef584a5b23`
- Component version: `1`
- Unique ID: `817afc45-76de-4b7c-9dca-7b77c4e86f9e`

Available pins recorded in the project:

- Serial Clock Line
- Serial Data Line
- Analog Reference
- Ground
- Pin 13
- Pin 12
- Pin 11
- Pin 10
- Pin 9
- Pin 8
- Pin 7
- Pin 6
- Pin 5
- Pin 4
- Pin 3
- Pin 2
- Transmit / Pin 1
- Receive / Pin 0
- Pin A5
- Pin A4
- Pin A3
- Pin A2
- Pin A1
- Pin A0
- External voltage input
- Ground 2
- Ground 1
- 5 Volt
- 3.3 Volt
- Reset

---

### 2. Motion Sensor

- Component name: `motion sensor`
- Instance label: `motion sensor`
- Instance ID: `c31ed2dc-87bd-4d83-8258-bb4ba6429fcf`
- Component version: `3`
- Unique ID: `784cc7d2-816e-4aa1-9822-731976238642`

Pins:

- `GND`
- `OUT`
- `VCC`

The exact motion sensor model is not specified.

**Update:** `OUT` now connects to Uno pin **A2** instead of Pin 11 (freed up
for the RC522's hardware SPI MOSI line). `A2` is simply digital pin 16 on the
Uno and works normally with `digitalRead()`. OUT is active HIGH when motion
is detected.

---

### 3. 4x4 Keypad

- Component name: `4x4 Keypad`
- Instance label: `4x4 Keypad`
- Instance ID: `748425ae-fc0d-44f4-b68e-b193353719c1`
- Component version: `3`
- Unique ID: `eec1bde3-86a8-4ac8-81a4-99525bb4829f`

Pins:

- `R1`
- `R2`
- `R3`
- `R4`
- `C1`
- `C2`
- `C3`
- `C4`

The physical key layout and library-specific keypad mapping are not specified.

---

### 4. OLED Display

- Component name: `oled`
- Instance label: `oled`
- Instance ID: `79e9f97f-990f-47c9-8a24-aa2966888a1f`
- Component version: `1`
- Unique ID: `80eff8b9-9492-4dbe-85ad-05da03fe76af`

Pins:

- `GND`
- `VCC`
- `SCL`
- `SDA`

**Update:** Verified as an SH1106-based 128x64 module at I²C address `0x3C`,
using the `Adafruit_SH1106G` library (not SSD1306). `SDA` connects to **A4**
and `SCL` to **A5** — the Uno's fixed hardware I²C pins.

---

### 5. Passive Buzzer

- Component name: `Buzzer Passive`
- Instance label: `Buzzer Passive`
- Instance ID: `b10ea444-40cf-4c9b-9fb3-a80931e5396f`
- Component version: `1`
- Unique ID: `7f4e8101-f6ea-4a9f-abea-db3207b54942`

Pins:

- `S output signal`
- `+VCC`
- `GND`

No resistor, transistor, or external buzzer driver is present in the documented circuit.

---

### 6. RFID-RC522 Reader

Added after this document's original export, so it has no Cirkit Designer
instance/unique ID. Provides a second way to arm/disarm: holding a card
listed in `ALLOWED_UIDS` does the same as entering the correct PIN.

Pins:

- `SDA` / `SS`
- `SCK`
- `MOSI`
- `MISO`
- `IRQ` (not connected)
- `GND`
- `RST`
- `VCC`

Runs on **3.3V, not 5V** — 5V on VCC destroys the module. `SCK`/`MOSI`/`MISO`
are the Uno's fixed hardware SPI pins (D13/D11/D12) and are shared with no
other component. `VersionReg` answers `0x91` or `0x92` on a genuine module;
`0x00` or `0xFF` means it is not responding (wiring or wrong voltage).

---

## Complete Wiring Table

### Arduino Uno Pin Assignments

| Arduino Uno pin | Connected component | Component pin | Signal/function |
|---|---|---|---|
| Pin 2 | 4x4 Keypad | C4 | Keypad column |
| Pin 3 | 4x4 Keypad | C3 | Keypad column |
| Pin 4 | 4x4 Keypad | C2 | Keypad column |
| Pin 5 | 4x4 Keypad | C1 | Keypad column |
| Pin 6 | 4x4 Keypad | R4 | Keypad row |
| Pin 7 | 4x4 Keypad | R3 | Keypad row |
| Pin 8 | 4x4 Keypad | R2 | Keypad row |
| Pin 9 | 4x4 Keypad | R1 | Keypad row |
| Pin 10 | Passive Buzzer | S output signal | Buzzer control signal |
| Pin 11 | RFID-RC522 | MOSI | Hardware SPI (fixed) |
| Pin 12 | RFID-RC522 | MISO | Hardware SPI (fixed) |
| Pin 13 | RFID-RC522 | SCK | Hardware SPI (fixed) |
| Pin A0 | RFID-RC522 | SDA / SS | Chip select |
| Pin A1 | RFID-RC522 | RST | Reset |
| Pin A2 | Motion sensor | OUT | Motion detection signal (moved off Pin 11) |
| Pin A4 | OLED | SDA | Fixed Uno I2C data pin |
| Pin A5 | OLED | SCL | Fixed Uno I2C clock pin |
| 5 Volt | Shared positive rail | OLED VCC, Buzzer +VCC, Motion VCC, Keypad (unpowered) | +5 V supply |
| 3.3 Volt | RFID-RC522 | VCC | +3.3 V supply — NOT 5V |
| Ground 1 | Shared ground rail | OLED GND, Buzzer GND, Motion GND, RFID-RC522 GND | Common ground |

---

## Keypad Wiring

The 4x4 keypad is connected directly to Arduino digital pins 2 through 9.

### Keypad Column Connections

| Keypad pin | Arduino pin |
|---|---|
| `C1` | Pin 5 |
| `C2` | Pin 4 |
| `C3` | Pin 3 |
| `C4` | Pin 2 |

### Keypad Row Connections

| Keypad pin | Arduino pin |
|---|---|
| `R1` | Pin 9 |
| `R2` | Pin 8 |
| `R3` | Pin 7 |
| `R4` | Pin 6 |

### Keypad Firmware Notes

The firmware must use the following raw pin mapping unless the physical wiring is intentionally changed:

```cpp
const byte ROW_PINS[4] = {9, 8, 7, 6};
const byte COL_PINS[4] = {5, 4, 3, 2};
```