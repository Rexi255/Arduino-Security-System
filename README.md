<p align="center">
  <img src="docs/banner.svg" alt="Projekt GAS: mini security system with keypad, RFID reader and motion sensor" width="100%">
</p>

<p align="center">
  <a href="https://bbz-aifs51.github.io/LF7-Projekt_GAS/"><b>▶ Open the 3D model in your browser</b></a>
  &nbsp;·&nbsp; <a href="MDs/anleitung.md">User manual (German)</a>
  &nbsp;·&nbsp; <a href="Schaltplan/SCHALTPLAN.png">Schematic</a>
  &nbsp;·&nbsp; <a href="Code/alarm_system/alarm_system.ino">Firmware</a>
</p>

A small alarm system on an Arduino Uno R3, built as a school project for
learning field 7 (LF07). You arm and disarm it with a PIN on a 4x4 keypad or
with an RFID card. While it is armed, the motion sensor triggers a 30-second
siren. The OLED only shows icons: a heart when the system is disarmed, a skull
when it is armed, and a flashing skull during an alarm. You add cards right on
the device, no PC needed.

<p align="center">
  <img src="docs/how-it-works.svg" alt="State machine: disarmed, exit delay, armed, alarm and admin menu" width="100%">
</p>

## Features

- **Two ways to arm and disarm:** PIN + `#` on the keypad, or a stored RFID card. Both do exactly the same thing.
- **Exit delay:** After arming you have 10 s to leave the room. The buzzer beeps once per second in the meantime.
- **Alarm:** It starts on motion (only while ARMED) or after **3 wrong tries** (wrong PIN or unknown card). The siren runs for 30 s or until the correct PIN or a known card is used.
- **Feedback:** A bright double tone means OK, a rough buzz means an error. The PIN only shows up as `*`, and a wrong entry briefly shows a ✖.
- **Admin menu on the device:** `A` + admin PIN + `#` opens it. There you add cards (just hold them to the reader), or browse and delete the stored ones. Cards live in the internal EEPROM and survive a new upload.
- **RFID reset:** `B` restarts the RC522 in case a clone module hangs.

## Quick controls

| Key | Action |
|---|---|
| `0`–`9` | enter the PIN (shown as `*`) |
| `#` | confirm the entry |
| `*` | clear the entry · in the admin menu: back / exit |
| `A` | while DISARMED: enter the admin PIN → admin menu |
| `B` | while DISARMED: re-initialise the RFID reader |
| Hold a card to the reader | same as the correct PIN (if the card is stored) |

The full end-user manual (in German) is in [MDs/anleitung.md](MDs/anleitung.md),
and as a PDF in [Anleitung/](Anleitung/Mini%20Security%20System%20Manual.pdf).

## Hardware

| Part | Uno connection | Note |
|---|---|---|
| Arduino Uno R3 | – | ATmega328P, 2 KB SRAM |
| 4x4 keypad | rows R1–R4 → D9–D6, columns C1–C4 → D5–D2 | the sketch scans the matrix itself |
| 1.3" OLED 128x64 | SDA → A4, SCL → A5 | **SH1106** controller, I²C address `0x3C` |
| RFID-RC522 | SS → A0, RST → A1, MOSI → D11, MISO → D12, SCK → D13 | **VCC to 3.3 V**, 5 V destroys the module |
| Motion sensor (PIR) | OUT → A2 | active HIGH |
| Passive buzzer | signal → D10 | driven directly from the pin, no transistor |
| 9 V battery | barrel jack | power supply |

All other VCC pins go to 5 V, all GND pins to a common ground. This uses up
every pin on the Uno; only D0/D1 (serial) are left free. The full parts list
is in [Teile.xlsx](Teile.xlsx).

<p align="center">
  <img src="docs/wiring.svg" alt="Wiring of all parts to the Arduino Uno" width="100%">
</p>

The original Cirkit Designer schematic is in
[Schaltplan/](Schaltplan/SCHALTPLAN.png) (PNG and SVG).

## Getting started

1. Install the **Arduino IDE** and get these libraries from the Library Manager:
   - **Adafruit SH110X** (also install its dependencies *Adafruit GFX* and *Adafruit BusIO*)
   - **MFRC522** (GithubCommunity)
   - **Keypad** (Mark Stanley / Alexander Brevig), only needed for `hardware_test`
   - `Wire`, `SPI` and `EEPROM` already come with the IDE.
2. Set the PIN and admin PIN at the top of [alarm_system.ino](Code/alarm_system/alarm_system.ino) (`PIN_CODE`, `ADMIN_PIN`).
3. Select the **Arduino Uno** board and the right port, then upload.
4. Set the serial monitor to **9600 baud**. On start-up it shows the RC522 version (`0x91`/`0x92` = OK), the number of stored cards and the free SRAM.
5. On the very first start the EEPROM is empty, so the sketch writes the factory list `DEFAULT_UIDS` into it. After that you manage cards only through the admin menu.

If something doesn't work, the test sketches check each part on its own:

| Sketch | Purpose |
|---|---|
| [Code/alarm_system/](Code/alarm_system/alarm_system.ino) | **Main firmware**: state machine, PIN, RFID, admin menu, sounds |
| [Code/hardware_test/](Code/hardware_test/hardware_test.ino) | bring-up test: OLED, keypad, buzzer, motion sensor |
| [Code/display_test/](Code/display_test/display_test.ino) | OLED only (icons + free SRAM) |
| [Code/rfid_test/](Code/rfid_test/rfid_test.ino) | RC522 only (version, UID of every card) |

## 3D model in the browser

**[▶ bbz-aifs51.github.io/LF7-Projekt_GAS](https://bbz-aifs51.github.io/LF7-Projekt_GAS/)**

An interactive 3D model of the build (three.js), laid out after the schematic.
It runs the firmware logic from `alarm_system.ino`, so the states, timings,
sounds and OLED graphics are the same:

- Press keys on the 3D keypad or in the side panel. Your keyboard works too (Enter = `#`, Esc = `*`).
- Click the cards or the key fob: they float over to the RC522 and the SPI lines light up.
- Click the motion sensor to trigger the alarm while ARMED.
- A signal pulse travels along each wire. A key press lights up exactly its row and column wire, and the L LED on D13 flickers during SPI traffic.
- The buzzer plays through WebAudio, and there is a serial monitor and an EEPROM card list.

The model uses demo PINs (`1234` and admin `0000`), not the real ones from the
sketch. To use it offline, just open
[docs/viewer/index.html](docs/viewer/index.html) in your browser.

## Pitfalls we solved

- **The OLED is an SH1106, not an SSD1306.** With the SSD1306 library it stays blank or only shows stripes. The right class is `Adafruit_SH1106G`.
- **SRAM is tight.** The OLED buffer alone takes 1024 of the 2048 bytes. With the Keypad library only ~205 bytes were left, which caused resets and wrong key presses. So the sketch scans the matrix itself, and all strings are kept in flash with `F()`.
- **`SPI.begin()` drives D10 HIGH**, because D10 is the AVR's SS pin. That is exactly where the buzzer sits, so the sketch sets the buzzer pin LOW only *after* `SPI.begin()`.
- **The motion sensor moved to A2**, because D11 is the fixed SPI MOSI pin. A2 works just like any digital pin with `digitalRead()`.
- **Polling the RC522 blocks for ~25 ms** when no card is present. So the sketch only polls the reader every 200 ms, otherwise the keypad gets sluggish and the siren stutters.

## Project structure

| Folder/file | Contents |
|---|---|
| [Code/](Code/) | Arduino sketches, one subfolder per sketch |
| [docs/](docs/) | animated README graphics (SVG) and the 3D viewer (`docs/viewer/`) |
| [MDs/](MDs/) | docs (German): [hardware spec](MDs/aufbau.md), [user manual](MDs/anleitung.md), [development log](MDs/entwicklungsverlauf.md) |
| [Anleitung/](Anleitung/) | user manual as a PDF |
| [Schaltplan/](Schaltplan/) | schematic (PNG/SVG) |
| [Teile.xlsx](Teile.xlsx) | parts list |
| [CLAUDE.md](CLAUDE.md) | project context and hardware facts for development with Claude Code |

The firmware was written together with Claude Code. Every prompt and answer is
logged (in German) in [MDs/entwicklungsverlauf.md](MDs/entwicklungsverlauf.md).

## License

[MIT](LICENSE). The 3D viewer uses [three.js](https://threejs.org/) (MIT,
bundled as `docs/viewer/three.min.js`).
