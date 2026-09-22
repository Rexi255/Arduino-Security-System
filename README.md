# Projekt GAS — Mini-Sicherheitssystem (Arduino Uno R3)

Schulprojekt (LF07): Arduino-basierte Zugangs-/Sicherheitsschaltung. Ein Code
wird über ein 4x4-Keypad oder eine RFID-Karte eingegeben, Rückmeldung erfolgt
über ein OLED-Display und einen passiven Buzzer, ein Bewegungssensor löst im
scharfen Zustand den Alarm aus.

## Funktionen

- Scharf-/Unscharfschalten per PIN (Keypad) oder RFID-Karte
- Ausgangsverzögerung (10 s) nach dem Scharfschalten
- Alarm bei Bewegung (nur im scharfen Zustand) oder nach 3 Fehlversuchen
- OLED zeigt Herz (unscharf) bzw. Totenkopf (scharf/Alarm)
- Admin-Menü am Gerät (Keypad + OLED) zum Anlernen/Löschen von RFID-Karten,
  Speicherung im internen EEPROM

Bedienungsanleitung für Endnutzer: [MDs/anleitung.md](MDs/anleitung.md)

## Ordnerstruktur

| Ordner/Datei | Inhalt |
|---|---|
| [Code/](Code/) | Arduino-Sketches, je ein Unterordner pro Sketch |
| [MDs/](MDs/) | Dokumentation (Hardware-Spezifikation, Bedienungsanleitung, Entwicklungsverlauf) |
| [Schaltplan/](Schaltplan/) | Schaltplan (PNG/SVG) |
| [Teile.xlsx](Teile.xlsx) | Bauteilliste |
| [CLAUDE.md](CLAUDE.md) | Projektkontext für die Entwicklung mit Claude Code |

## Sketches

| Sketch | Zweck |
|---|---|
| [Code/alarm_system/](Code/alarm_system/alarm_system.ino) | Hauptfirmware (Zustandsautomat, PIN, RFID, Admin-Menü) |
| [Code/hardware_test/](Code/hardware_test/hardware_test.ino) | Bring-up-Test aller Bauteile |
| [Code/display_test/](Code/display_test/display_test.ino) | Isolierter OLED-Test |
| [Code/rfid_test/](Code/rfid_test/rfid_test.ino) | Isolierter RC522-Test (Version-Check, UID-Ausgabe) |

## Hardware

- Mikrocontroller: Arduino Uno R3
- Vollständige Pin-Belegung und Hardware-Details: siehe [CLAUDE.md](CLAUDE.md#angeschlossene-bauteile--pin-belegung)
- Ursprüngliche Hardware-Spezifikation (vor RFID-Umbau): [MDs/aufbau.md](MDs/aufbau.md)

## Benötigte Libraries (Arduino IDE — Library Manager)

- **Keypad** (Mark Stanley / Alexander Brevig) — nur für `hardware_test`
- **Adafruit SH110X** (inkl. Abhängigkeiten Adafruit GFX, Adafruit BusIO)
- **MFRC522** (GithubCommunity)
- `Wire`, `SPI`, `EEPROM` — in der Arduino IDE enthalten

## Build / Upload

Arduino IDE, Board = „Arduino Uno", passenden Port wählen, Upload. Serieller
Monitor: 9600 Baud.

## Entwicklung

Protokoll der Entwicklung (Prompts/Antworten mit Claude Code):
[MDs/entwicklungsverlauf.md](MDs/entwicklungsverlauf.md)
