# CLAUDE.md — Projekt GAS (Security/Access Circuit)

Guidance für Claude Code in diesem Projekt. Schulprojekt (LF07) — Umfang klein
halten, nichts überdimensionieren.

## Überblick

Arduino-basierte Zugangs-/Sicherheitsschaltung. Nutzer gibt einen Code über
ein 4x4-Keypad ein, Rückmeldung über OLED und passiven Buzzer, ein
Bewegungssensor erkennt Anwesenheit.

- Mikrocontroller: **Arduino Uno R3**
- Hardware-Spezifikation: [MDs/aufbau.md](MDs/aufbau.md) (Stand vor dem RFID-Umbau;
  maßgeblich für Pins/Verdrahtung ist die Tabelle weiter unten in dieser Datei)
- Schaltplan: [Schaltplan/](Schaltplan/)
- Teileliste: [Teile.xlsx](Teile.xlsx)

## Ordnerstruktur

- `Code/` — Arduino-Sketches. Jeder Sketch in eigenem Unterordner gleichen Namens
  (Arduino-Vorgabe), z. B. `Code/hardware_test/hardware_test.ino`.
- `MDs/` — Dokumentation (u. a. [MDs/entwicklungsverlauf.md](MDs/entwicklungsverlauf.md):
  Protokoll der Prompts und Antworten aus der Entwicklung mit Claude Code)
- `Schaltplan/` — Schaltplan
- `Gehaeuse/` — 3D-druckbares Gehäuse: `gehaeuse.scad` (OpenSCAD, parametrisch,
  Teil über `part` wählen), fertige STLs in `stl/`, Bilder in `bilder/`.
  Zwei Gehäuse: Bedienteil außen (Uno, Keypad, OLED, RC522, Buzzer) und
  Sensorteil innen (HC-SR501), verbunden per 3-adrigem Kabel. Planung und
  Druck-/Bauanleitung: [MDs/gehaeuse.md](MDs/gehaeuse.md). Modulmaße stammen
  aus Datenblättern, nicht vom echten Aufbau. **Ändert sich ein Bauteil oder
  dessen Position, STLs neu exportieren.**
- `Teile.xlsx` — Bauteilliste
- `docs/` — README-Grafiken als animierte SVGs (`banner.svg`, `how-it-works.svg`,
  `wiring.svg`, handgeschrieben, CSS-Animationen) und `docs/viewer/`: 3D-Modell
  (three.js, `three.min.js` liegt bei) mit nachgebauter Firmware-Logik aus
  `alarm_system.ino` (Demo-PINs `1234`/`0000`). Wird per
  `.github/workflows/pages.yml` auf GitHub Pages veröffentlicht. **Ändert sich
  Verhalten, Zeit oder Pin in der Firmware, den Viewer mitziehen.**

## Angeschlossene Bauteile & Pin-Belegung

| Bauteil | Anschluss am Uno |
|---|---|
| 4x4 Keypad — Reihen R1..R4 | D9, D8, D7, D6 |
| 4x4 Keypad — Spalten C1..C4 | D5, D4, D3, D2 |
| Passiver Buzzer — Signal | D10 |
| Bewegungssensor — OUT | **A2** (war D11, s. u.) |
| OLED — SDA | **A4** |
| OLED — SCL | **A5** |
| RFID-RC522 — SDA/SS | **A0** |
| RFID-RC522 — RST | **A1** |
| RFID-RC522 — MOSI | **D11** (fest, Hardware-SPI) |
| RFID-RC522 — MISO | **D12** (fest, Hardware-SPI) |
| RFID-RC522 — SCK | **D13** (fest, Hardware-SPI) |
| RFID-RC522 — IRQ | nicht angeschlossen |
| RFID-RC522 — VCC | **3.3V** (nicht 5V!) |
| Alle übrigen VCC | 5V |
| Alle GND | GND |

Damit ist die Pinbelegung des Uno vollständig ausgereizt — nur noch D0/D1
(Serial) sind frei.

Raw-Pin-Mapping fürs Keypad (nicht ändern, außer die Verdrahtung ändert sich):

```cpp
const byte ROW_PINS[4] = {9, 8, 7, 6};
const byte COL_PINS[4] = {5, 4, 3, 2};
```

## Wichtige Hardware-Fakten (verifiziert am Aufbau)

- **OLED-Controller: SH1106**, 128x64, I2C-Adresse **0x3C**. NICHT SSD1306.
  Verwende `Adafruit_SH1106G` aus der Library `Adafruit SH110X`.
- **I2C-Pins am Uno sind fest: SDA = A4, SCL = A5.** In `aufbau.md` sind SDA/SCL
  vertauscht notiert (SDA→A5, SCL→A4) — die reale Verdrahtung nutzt A4/A5 korrekt.
- **`MDs/aufbau.md` ist für den RFID-Umbau nicht aktuell** — der RC522 und der
  Umzug des Bewegungssensors auf A2 stehen dort nicht drin. Maßgeblich ist die
  Pin-Tabelle in dieser Datei.
- Bewegungssensor: OUT ist **active HIGH** bei erkannter Bewegung. Sitzt seit
  dem RFID-Umbau auf **A2**, weil D11 fest der SPI-MOSI-Pin ist. `A2` ist am
  Uno einfach Digitalpin 16 und funktioniert normal mit `digitalRead()`.
- **RFID-RC522 läuft auf 3,3 V.** 5 V an VCC zerstört das Modul. Der 3,3V-Pin
  des Uno liefert 50 mA, der RC522 zieht 13–26 mA — reicht. Die SPI-Leitungen
  sind 5V-Logik vom Uno; laut Datenblatt außerhalb der Spezifikation, in der
  Praxis funktioniert es.
- **`SPI.begin()` schaltet D10 (den AVR-SS-Pin) auf OUTPUT HIGH** — und D10 ist
  der Buzzer. Deshalb in `setup()` erst `SPI.begin()`, danach `pinMode()` /
  `digitalWrite(BUZZER_PIN, LOW)`, sonst bleibt der Buzzer-Pin auf HIGH.
- **RC522-Polling blockiert ~25 ms**, wenn keine Karte davor liegt (interner
  Antwort-Timeout). Deshalb nicht in jedem `loop()`-Durchlauf pollen, sondern
  nur alle `RFID_POLL_MS` (200 ms) — sonst wird das Keypad träge und der
  Sirenen-Sweep stockt.
- `VersionReg` des RC522 antwortet **0x91 oder 0x92**. 0x00 oder 0xFF heißt:
  Modul antwortet nicht (Verdrahtung oder 5 V statt 3,3 V).
- Passiver Buzzer: direkt an D10, kein Treiber/Transistor. Ansteuerung mit
  `tone()` / `noTone()`.
- **SRAM ist knapp**: Uno hat 2048 Byte, der OLED-Framebuffer belegt allein
  1024. Deshalb alle Strings mit `F(...)` ins Flash, kein `snprintf`, keine
  temporären Textpuffer — sonst wächst der Stack in den Framebuffer und die
  Anzeige wird zu Pixelmüll.
- Keypad-Layout (Standard 4x4):
  `1 2 3 A` / `4 5 6 B` / `7 8 9 C` / `* 0 # D`

## Benötigte Libraries (Arduino IDE — Library Manager)

| Library | Zweck |
|---|---|
| **Keypad** (Mark Stanley / Alexander Brevig) | 4x4 Tastenfeld — nur noch für `hardware_test` |
| **Adafruit SH110X** | OLED (SH1106) |
| **Adafruit GFX Library** | Text/Grafik (Abhängigkeit) |
| **Adafruit BusIO** | I2C-Hilfsschicht (Abhängigkeit) |
| **MFRC522** (GithubCommunity) | RFID-RC522 |
| `Wire`, `SPI`, `EEPROM` | in der Arduino IDE enthalten |

Beim Installieren von „Adafruit SH110X" die Abhängigkeiten mitinstallieren.
Adafruit SSD1306 wird nicht benötigt. Die MFRC522-Library kostet nur ~15 Byte
statisches SRAM (im Wesentlichen die `Uid`-Struktur), Flash rund 5–7 kB.

## Build / Upload

- Arduino IDE: Board = „Arduino Uno", korrekten Port wählen, Upload.
- Serieller Monitor: **9600 Baud** (Testausgaben).

## Sketches

- `Code/hardware_test/hardware_test.ino` — Bring-up-Test: prüft OLED, Keypad,
  Buzzer und Bewegungssensor einzeln, Ausgabe über Seriellen Monitor + OLED.
  Status: funktioniert, alle Bauteile ok.
- `Code/display_test/display_test.ino` — isolierter OLED-Test (nur Display):
  zeigt Rahmen/Text, Herz, Totenkopf und `****` im Wechsel und gibt den freien
  SRAM über Serial aus. Zum Eingrenzen, wenn die Anzeige im Hauptsketch spinnt.
- `Code/rfid_test/rfid_test.ino` — isolierter RC522-Test: prüft `VersionReg`
  und gibt die UID jeder aufgelegten Karte über Serial aus. Nützlich zur
  Fehlersuche (z. B. prüfen, ob eine Karte überhaupt korrekt gelesen wird),
  auch wenn Karten inzwischen über den Admin-Modus angelernt werden (s. u.)
  statt von Hand eingetippt.
- `Code/alarm_system/alarm_system.ino` — **Hauptfirmware**: Zustandsautomat
  (UNSCHARF → Ausgangsverzögerung → SCHARF → ALARM → ADMIN), PIN über Keypad
  (Maskierung mit `*`, Bestätigung mit `#`, Abbruch mit `*`), Totenkopf/Herz
  auf dem OLED, nicht-blockierende Töne (OK / Fehler 3 s / Sirene 30 s).
  PIN und Zeiten stehen als Konstanten oben im Sketch.
  **Kein Keypad-Library-Einsatz**: der Sketch scannt die Matrix selbst
  (`scanKeypad()`, Keymap im PROGMEM). Die Library kostete ~120 Byte SRAM,
  wodurch nur ~205 Byte frei blieben — das führte zu sporadischen Resets und
  verfälschten Tasteneingaben.
  Scharf/Unscharf geht auf zwei Wegen: PIN + `#` am Keypad **oder** eine
  gespeicherte Karte an den RC522 halten. Beide laufen über `accessGranted()` /
  `accessDenied()`, 3 Fehlversuche (falsche PIN oder unbekannte Karte) lösen
  den Alarm aus.
  **RFID-Rechtevergabe (Admin-Modus):** Erlaubte UIDs stehen nicht mehr fest
  im Code, sondern im internen EEPROM des Uno (`loadUidsFromEeprom()` /
  `saveUidsToEeprom()`, Layout: Magic-Byte, Anzahl, dann `UID_LEN`-Byte-Blöcke)
  — sie überstehen also ein Neu-Hochladen des Sketches. `DEFAULT_UIDS`
  (PROGMEM) ist nur die Werksvorgabe, mit der ein leeres EEPROM beim allerersten
  Start gefüllt wird. Verwaltet wird komplett über Keypad + OLED, ohne PC:
  Taste `A` im Zustand UNSCHARF + `ADMIN_PIN` + `#` öffnet das Menü
  (`STATE_ADMIN`, `handleAdminKeypad()`/`handleAdminRfid()`) mit `1` = Karte
  anlernen (auflegen genügt, kein manuelles Eintippen der Hex-UID mehr), `2` =
  Liste durchblättern/löschen (`D` löscht sofort, keine Sperrliste), `*` =
  verlassen. Menü schließt nach `ADMIN_IDLE_TIMEOUT_MS` Inaktivität automatisch.
  Eine falsche Admin-PIN zählt als Fehlversuch wie eine falsche normale PIN
  (Bruteforce-Schutz, gleicher Alarm nach 3 Versuchen). `MAX_STORED_UIDS` (8)
  ist bewusst klein gehalten, wegen des knappen SRAMs (s. Memory Note); nach
  dem Lesen einer Karte wird weiterhin `PICC_HaltA()` gerufen, damit eine
  liegengelassene Karte nicht dauerhaft neu triggert.
  **RFID-Reset:** Taste `B` im Zustand UNSCHARF ruft `resetRfid()` auf
  (`PCD_Reset()` + `PCD_Init()`, danach `VersionReg` neu prüfen). Manuelle
  Abhilfe, falls der RC522-Klon mal nicht mehr auf SPI antwortet (0x00/0xFF
  statt 0x91/0x92) — ohne das bliebe nur ein kompletter Neustart des Uno.

## Konventionen

- Kommentare auf Englisch.
- Kein Feature-Overkill — Schulprojekt.
