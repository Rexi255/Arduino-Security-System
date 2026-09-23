# Entwicklungsverlauf — Projekt GAS

Dokumentation der Firmware-Entwicklung mit Claude Code (Claude Opus 5).
Chronologisches Protokoll: Prompts (Jan Mummert) und Antworten (Claude).

- Projekt: LF07 — Mini-Sicherheitssystem / Access Lock (Arduino Uno R3)
- Daten der Sitzungen: 08.09.2026, 22.09.2026, 23.09.2026
- Grundlagen im Kontext: [../CLAUDE.md](../CLAUDE.md), [aufbau.md](aufbau.md)

---

## 1. Prompt — Auftrag: komplette Firmware

> Use @CLAUDE.md und @MDs/aufbau.md.
>
> Programmiere den kompletten Code für unser Projekt.
>
> Was wir wollen: Ein mini Sicherheitssystem/Doorlock/Access Lock.
> Der Buzzer soll einen Alarmton abspielen, wenn der Motion sensor ausgelöst
> ist. Man soll die Alarmanlage mit einem Pin (später auch mit rfid, aber das
> ist nur eine Idee für ein feature wenn alles läuft) entschärfen können und
> auch wieder mit dem Pin scharf schalten können. Das Display soll im scharfen
> zustand einen Totenkopf anzeigen, wenn man Pin eingibt sollen anstelle des
> Pins nur Sternchen "*" angezeigt werden, die Eingabe des Pins soll man mit
> "#" bestätigen können. Bei richtigem Pin soll ein heller angenehmer
> Bestätigungston kommen, Bei Falsch eingabe ein Rauer längerer Ton (3sek).
> Bei 3 facher fehleingabe soll der Alarm der Anlage ertönen.
> Im Entsperrten zustand soll das Display ein Herz anzeigen.
>
> Im Unscharfen zustand soll der Motion Sensor natürlich keinen Alarm auslösen,
> nur wenn die Anlage scharf ist.
> Der Alarm soll extrem laut und schrill sein, er soll 30 sekunden dauern.

### Antwort

Fertig: [../Code/alarm_system/alarm_system.ino](../Code/alarm_system/alarm_system.ino)
— die komplette Firmware, Pins genau wie in aufbau.md/CLAUDE.md
(SDA=A4, SCL=A5, SH1106 @0x3C).

**Zustände**

- `UNSCHARF` — Herz auf dem Display, Bewegungssensor wird ignoriert.
- `SCHARF IN xs` — Ausgangsverzögerung (10 s, Piep pro Sekunde), damit du nach
  dem Scharfschalten den Raum verlassen kannst, ohne dich selbst auszulösen.
  Das hattest du nicht verlangt, ohne das löst die Anlage aber sofort beim
  eigenen Zurücktreten aus — Wert steht als `EXIT_DELAY_MS` oben, auf 0 setzen
  wenn du es nicht willst.
- `SCHARF` — Totenkopf, Bewegung (steigende Flanke auf D11) löst Alarm aus.
- `ALARM` — blinkendes „ALARM!!", Totenkopf, Restsekunden; Sirene 30 s, danach
  automatisch wieder scharf.

**Bedienung am Keypad**

- `0-9` PIN eingeben, auf dem Display nur `*` (Größe 2, mittig)
- `#` bestätigen → richtiger PIN schaltet scharf/unscharf und beendet den
  laufenden Alarm sofort
- `*` Eingabe löschen; halb getippte PIN verfällt nach 10 s von selbst
- PIN steht oben als `const char PIN_CODE[] = "1234";`

**Töne** (nicht-blockierend, Keypad bleibt währenddessen bedienbar)

- Richtig: zwei helle kurze Noten (G6 → C7)
- Falsch: rauer tiefer Brummton, 3 s (schnelles Wechseln 95/135 Hz)
- 3× falsch: voller Alarm — das gilt in jedem Zustand, also auch im unscharfen
  (Sabotage). Wenn du das nur im scharfen Zustand willst, sag Bescheid.
- Alarm: Sirenen-Sweep 2500↔3700 Hz, bewusst im lautesten Bereich des Piezos,
  30 s

**Nicht verifiziert:** Ich konnte den Sketch hier nicht kompilieren —
arduino-cli ist auf dem Rechner nicht vorhanden. Bitte einmal in der Arduino
IDE bauen und hochladen; die Bibliotheken sind dieselben wie beim
`hardware_test`.

---

## 2. Prompt — Display zeigt Pixelmüll

> Das Display bugt ein wenig, es zeigt weder das herz, noch einen totenkopf
> oder ähnliches an. Auch der Code etc. wird nicht richtig angezeigt. man kann
> die sekundne timer erahnen, aber das wars. Das display hat diese specs:
>
> - Module dimensions: 36 mm x 34 mm x 3 mm
> - Dot Matrix: 128 x 64
> - Pixel size: 0.21 mm × 0.21 mm
> - Pixel pitch: 0.23 mm × 0.23 mm
> - Display Mode: Passive Matrix
> - Duty: 1/64 Duty
> - Display Color: Blue
> - Interface: I2C
> - Power consumption: Less then 11mA
> - Size: 1.3 inch

### Antwort

Das klingt nach einem Anzeige-/Speicherproblem, nicht nach falscher Geometrie —
1,3" 128x64 I2C ist praktisch immer der SH1106, und der `hardware_test` lief ja.
Ich mache zwei Dinge: einen isolierten Display-Test zum Eingrenzen und die
Hauptfirmware deutlich RAM-sparsamer.

Der wahrscheinlichste Verdacht: Der Uno hat 2 KB SRAM, der Framebuffer belegt
davon allein 1 KB. `snprintf("%lu")` und die Text-Literale im RAM treiben den
Stack so weit hoch, dass er in den Framebuffer läuft — das ergibt genau so ein
Bild, in dem man Reste (die Sekundenzahlen) noch erahnen kann.

Zwei Sachen, bitte in dieser Reihenfolge testen:

**1. [../Code/display_test/display_test.ino](../Code/display_test/display_test.ino)
hochladen** (nur das Display, sonst nichts). Er zeigt im 2-Sekunden-Takt:
Rahmen+Text → Herz → Totenkopf → `****`, und schreibt vor jedem Bild den freien
SRAM in den Seriellen Monitor.

- Sehen die vier Bilder korrekt aus → das Display kann alles, das Problem ist
  der Speicher im Hauptsketch (siehe Punkt 2).
- Sind sie auch hier kaputt → es liegt am Display selbst; dann sag mir bitte,
  *wie* kaputt (verschoben, gestreift, Rauschen) und was der Rahmen macht. Dann
  ist der Controller vermutlich doch kein SH1106, und ich baue auf SSD1306 bzw.
  auf einen Spaltenversatz um.

**2. [../Code/alarm_system/alarm_system.ino](../Code/alarm_system/alarm_system.ino)
ist überarbeitet.** Was ich geändert habe und warum:

Der Uno hat 2048 Byte SRAM, davon frisst der OLED-Framebuffer allein 1024. Dazu
Keypad-Library, Wire- und Serial-Puffer — es bleiben nur ein paar hundert Byte
Stack. `snprintf("%lu")` braucht davon über 100 Byte auf einen Schlag, und die
Text-Literale `"UNSCHARF"` usw. lagen zusätzlich im RAM. Wenn der Stack in den
Framebuffer läuft, sieht man genau das, was du beschreibst: Pixelmüll, in dem
man Reste (deine Sekundenzahlen) noch erahnen kann.

Deshalb jetzt:

- kein `snprintf` mehr — der Countdown wird direkt mit `display.print()`
  ausgegeben
- alle Display-Texte über `F(...)` im Flash statt im RAM
- keine temporären `char`-Puffer mehr (die Sternchen werden in einer Schleife
  gedruckt)
- `setup()` gibt `free SRAM:` über Serial aus — der Wert sollte deutlich über
  250 liegen

Der Funktionsumfang ist unverändert. Die Icons habe ich zusätzlich leicht
anders positioniert, damit sich Totenkopf und die Sternchen-Zeile nicht mehr
überlappen.

Melde mir die vier Testbilder und den SRAM-Wert, dann weiß ich, in welche
Richtung es geht. Kompilieren kann ich hier weiterhin nicht — auf dem Rechner
ist keine arduino-cli installiert.

---

## 3. Prompt — Dokumentation

> Packe alle Prompts von mir und deine antworten in einer .md datei. wir
> brauchen das als dokumentation

### Antwort

Dieses Dokument. Es wird bei weiteren Änderungen fortgeschrieben.

---

## 4. Prompt — Resets beim Scharfschalten, Text soll weg

> Funktioniert soweit.
>
> 1. Sehr oft, nachdem es einmal armed und disarmed wurde kann man es nicht
>    erneut armen. Es zeigt im serial pin wrong an obwohl dieser richtig
>    eingegeben wurde:
>
>    ```
>    free SRAM: 205
>    [PIN] wrong 1
>    [PIN] correct
>    [STATE] arming - exit de=== Alarm system ready (disarmed) ===
>    PIN + '#' arms / disarms, '*' clears the entry.
>    free SRAM: 205
>    [PIN] wrong 1
>    ```
>
> 2. Entferne den Text bei den Symbolen, es soll nur Totenkopf oder Herz ohne
>    Text angezeigt werden. Der Sekundentimer ist etwas zu groß, die hälfte der
>    Zahl verschwindet weil es über den display rand hinaus geht.

### Antwort

**Zu 1 — der Arduino startet neu.** Im Log bricht die Zeile
`[STATE] arming - exit de` mitten im Wort ab und danach kommt wieder der
Startbanner `=== Alarm system ready ===`. Das ist ein Reset, kein Logikfehler.
Ursache ist der freie SRAM: **205 Byte**. Davon muss der komplette Stack leben,
inklusive der Interrupts, die `tone()` erzeugt (bei 3700 Hz sind das 7400
Interrupts pro Sekunde). Wenn so ein Interrupt zuschlägt, während der
Display-Code tief im Stack steht, läuft der Stack in den Framebuffer und in die
globalen Variablen — mal gibt es einen Reset, mal wird nur `entry[]` verfälscht,
und dann meldet der Sketch „PIN wrong" obwohl richtig getippt wurde. Das erklärt
auch, warum es erst *nach* dem ersten Scharfschalten auftritt: vorher hat noch
kein Ton gespielt.

Der größte Speicherfresser nach dem Framebuffer war die **Keypad-Library**: sie
hält 10 `Key`-Objekte, eine Bitmap und die Keymap im RAM, zusammen rund 120 Byte.
Die habe ich rausgeworfen und den 4x4-Scan selbst geschrieben — für eine Matrix
sind das ungefähr 30 Zeilen:

- Zeilen auf `INPUT_PULLUP`, Spalten hochohmig (`INPUT`)
- pro Scan eine Spalte auf `OUTPUT`/LOW ziehen und die vier Zeilen lesen
- Keymap liegt jetzt im Flash (`PROGMEM` + `pgm_read_byte`)
- eigenes Entprellen (20 ms), eine Taste wird genau einmal beim Drücken gemeldet

Damit sollten rund **320–330 Byte** frei sein. Der Wert steht wie vorher beim
Start im Seriellen Monitor — bitte einmal nachsehen und mir sagen, was dasteht.
Zusätzlich gibt der Sketch jetzt jede erkannte Taste (`[KEY] 4`) und die
komplette Eingabe vor der Prüfung (`[PIN] entered: 1234`) aus. Falls doch noch
etwas verschluckt wird, sieht man es damit sofort.

Die Keypad-Library wird für diesen Sketch nicht mehr gebraucht; der
`hardware_test` benutzt sie weiterhin, also bitte installiert lassen.

**Zu 2 — Anzeige.** Aller Statustext ist raus, es gibt nur noch die Icons:

- UNSCHARF: Herz, mittig
- SCHARF: Totenkopf, mittig
- Ausgangsverzögerung: Totenkopf + Countdown darunter
- ALARM: Totenkopf, der im 250-ms-Takt das ganze Display invertiert, darunter
  die Restsekunden (auch das „ALARM!!" ist weg — sagt Bescheid, wenn ihr das
  Wort zurückhaben wollt)

Der abgeschnittene Countdown war ein vertikales Problem, kein horizontales: die
Zahl stand in Größe 2 auf y = 52, das sind 16 Pixel Höhe bis y = 68 — das
Display hört bei 63 auf, also fehlte die untere Hälfte. Jetzt sitzt sie auf
y = 46 bzw. y = 47 und passt.

Während der PIN-Eingabe zeigt das Display nur noch die Sternchen, groß und
mittig. Grund: Totenkopf und Sternchen brauchen zusammen mehr als 64 Pixel Höhe,
sie hätten sich sonst überlappt.

---

## 5. Prompt — Kollisionserkennung und Rechtevergabe bei RFID

> Erkläre uns was unsere Lehrerin mit Kollisionserkennung und Rechtevergabe im
> bezug auf RFID meint

### Antwort

Zwei verschiedene Ebenen, die beim RFID-Teil oft durcheinander geworfen werden:

**Kollisionserkennung — Protokoll-Ebene (ISO 14443A)**

Liegen mehrere Karten gleichzeitig im Feld des RC522, würden bei einer
einfachen Anfrage alle gleichzeitig antworten — das ist eine *Kollision*. Der
ISO-14443A-Standard löst das über ein Anti-Kollisions-Verfahren: Der Reader
fragt die UID Bit für Bit in Kaskaden ab, bis genau eine Karte eindeutig
identifiziert ist. Bei uns läuft das komplett innerhalb der MFRC522-Library ab,
über die Aufrufe

```cpp
mfrc522.PICC_IsNewCardPresent()
mfrc522.PICC_ReadCardSerial()
```

in [readRfidUid()](../Code/alarm_system/alarm_system.ino#L326). Der Aufruf
[mfrc522.PICC_HaltA()](../Code/alarm_system/alarm_system.ino#L339) danach
schickt die gelesene Karte in den HALT-Zustand, damit sie nicht bei jedem
Poll-Zyklus erneut "kollidiert" bzw. neu gelesen wird, solange sie liegen
bleibt.

**Rechtevergabe — Anwendungsebene (Access Control)**

Unsere jetzige Logik ist rein binär: Eine UID steht in
[ALLOWED_UIDS](../Code/alarm_system/alarm_system.ino#L77) oder nicht, geprüft
in [isAllowedUid()](../Code/alarm_system/alarm_system.ino#L350) — jede erlaubte
Karte darf alles, was das System kann (scharf/unscharf schalten). Mit
"Rechtevergabe" ist vermutlich eine feinere Unterscheidung gemeint, z. B.:

- unterschiedliche Rollen pro Karte (manche dürfen nur entschärfen, andere
  auch neue Karten anlernen)
- Karten zur Laufzeit hinzufügen/entziehen können, statt sie fest im Code
  (`PROGMEM`) zu hinterlegen
- Protokollierung, wer wann welche Karte benutzt hat, statt nur ja/nein

Für den Umfang dieses Schulprojekts bleibt es bei der einfachen
Allow-List-Prüfung — das oben ist als konzeptionelle Erklärung für die
Dokumentation/Präsentation gedacht, nicht als Implementierungsauftrag.

---

## 6. Prompt — Rechtevergabe umsetzen: UIDs außerhalb des Codes verwalten

> Thema Rechtevergabe. Man soll die uids außerhalb des Codes verändern, bzw.
> sperren oder hinzufügen können. Wie könnte man das lösen. Plane erst

### Antwort

Aufbauend auf Eintrag 5 (dort nur konzeptionell besprochen, bewusst nicht
umgesetzt) jetzt tatsächlich implementiert, nach Rückfrage zur Bedienung:

- Admin-Interface komplett über Keypad + OLED (keine Serial-Kommandos, kein
  PC nötig)
- Löschen entfernt eine UID sofort, keine separate Sperrliste

**Speicherung:** UIDs liegen jetzt im internen EEPROM des Uno statt fest im
Code (`DEFAULT_UIDS` bleibt nur als Werksvorgabe fürs allererste Booten).
Kein Zusatzmodul (z. B. SD-Karte) nötig und laut CLAUDE.md auch gar nicht
möglich — kein Pin mehr frei. EEPROM übersteht ein Neu-Hochladen des Sketches.

**Bedienung:** Taste `A` im UNSCHARF-Zustand + eine eigene `ADMIN_PIN` + `#`
öffnet einen neuen Zustand `STATE_ADMIN` mit kleinem Text-Menü auf dem OLED
(bisher zeigte das Display nur Icons — hier bewusste Ausnahme, siehe
[Code/alarm_system/alarm_system.ino](../Code/alarm_system/alarm_system.ino#L885)):

- `1` Karte anlernen — auflegen genügt, kein manuelles Abtippen der Hex-UID
  mehr (das war vorher nötig, siehe `rfid_test.ino`)
- `2` Liste durchblättern (`#`), Karte löschen (`D`)
- `*` Menü verlassen; nach `ADMIN_IDLE_TIMEOUT_MS` (20 s) auch automatisch

Eine falsche Admin-PIN läuft über denselben `accessDenied()`-Pfad wie eine
falsche normale PIN — zählt also zu den 3 Fehlversuchen und löst genauso
Alarm aus (Bruteforce-Schutz für beide PINs einheitlich).

`MAX_STORED_UIDS` bewusst klein (8) gewählt wegen des knappen SRAMs (siehe
Memory Note in CLAUDE.md) — mehr Platz wäre im EEPROM (1024 Byte) locker da,
das RAM ist der eigentliche Engpass.

Vor der Änderung wurde eine Kopie der alten Version unter
`Code/alarm_system/alarm_system.ino.bak` gesichert.

**Offener Punkt:** Wie in Eintrag 4 — kein `arduino-cli` auf diesem Rechner,
der Sketch wurde nicht kompiliert. Vor dem Hochladen in der Arduino IDE
verifizieren und danach die Punkte aus dem Plan durchgehen (u. a.: passt
`free SRAM` noch, wandert die bestehende `DEFAULT_UIDS`-Karte korrekt ins
EEPROM, funktioniert Anlernen/Löschen, löst 3x falsche Admin-PIN den Alarm
aus, übersteht eine neue Karte ein erneutes Hochladen).

---

## 7. Prompt — RFID-Reset (22.09.2026)

> Unsere Lehrerin hat bei einer anderen Gruppe angemerkt, dass sie keinen
> RFID-Reset haben, da sich das RFID-Modul wohl öfters mal aufhängen kann.
> Wie könnten wir sowas bei uns einbauen, bzw. ist das überhaupt nötig? Falls
> ja könnte man da ja auch z. B. den Buchstaben B beim Tastenfeld setzen.

### Antwort

Ja, sinnvoll: RC522-Klone können SPI-seitig hängen bleiben (`VersionReg`
antwortet dann wieder mit `0x00`/`0xFF` statt `0x91`/`0x92`), erkennbar an
Karten, die plötzlich nicht mehr gelesen werden — sonst hilft nur ein
kompletter Neustart des Uno. `RST_PIN` (A1) ist bei uns ohnehin schon
verdrahtet, ein Reset zur Laufzeit ist also reine Software.

Rückfrage, ob nur manuell per Taste, zusätzlich automatisch im Hintergrund,
oder gar nicht — Antwort: nur manuell. Umgesetzt in
[../Code/alarm_system/alarm_system.ino](../Code/alarm_system/alarm_system.ino):

- Neue Funktion `resetRfid()` (`PCD_Reset()` + `PCD_Init()`, danach
  `VersionReg` neu geprüft und `rfidOk` aktualisiert; OK-/Fehlerton als
  Feedback)
- Taste `B` im Zustand UNSCHARF ruft `resetRfid()` auf — bisher unbelegt,
  analog zur `A`-Abfrage fürs Admin-Menü (nur bei `entryLen == 0`)
- Kopf-Kommentar im Sketch und [../CLAUDE.md](../CLAUDE.md) (Abschnitt
  „Sketches") entsprechend ergänzt

**Offener Punkt:** wie in den vorherigen Einträgen nicht kompiliert (kein
`arduino-cli` auf dem Rechner) — vor dem nächsten Hochladen in der Arduino
IDE prüfen, außerdem am echten Aufbau testen, ob `B` den Reader nach einem
gezielten Verbindungsabbruch (z. B. RST kurz trennen) tatsächlich wiederbelebt.

---

## 8. Prompt — 3D-Animationen und neue README (23.09.2026)

> Schaue dir dieses Github Repo von einem freund an:
> https://github.com/BBZ-AIFS51/LF7-MiniCasino
>
> Ich möchte das in dem Github repo von diesem Projekt auch so coole 3d
> animationen wie bei denen sind. Verwende dafür unsere dokumentation. Dabei
> kannst du Auch gleich die README unseres Projekts einmal komplett überarbeiten

### Antwort

Das MiniCasino-Repo nutzt zwei Dinge: animierte SVG-Grafiken direkt in der
README (CSS-Animationen, laufen auch auf GitHub) und einen three.js-3D-Viewer,
der über GitHub Pages läuft. Beides gibt es jetzt auch bei uns, aufgebaut aus
CLAUDE.md, aufbau.md, anleitung.md, dem Schaltplan und der Firmware:

- [../docs/banner.svg](../docs/banner.svg): isometrisches Gerät, das in 10 s
  einmal UNSCHARF → PIN → SCHARF → ALARM durchläuft. Herz und Totenkopf mit
  denselben Koordinaten wie `drawHeart()`/`drawSkull()`, die Alarm-Anzeige
  invertiert wie im Sketch, dazu eine schwebende RFID-Karte.
- [../docs/how-it-works.svg](../docs/how-it-works.svg): Zustandsautomat mit
  laufenden Pfeilen, dem jeweils aktiven Zustand und Fortschrittsbalken für
  10 s / 30 s.
- [../docs/wiring.svg](../docs/wiring.svg): Verdrahtung am Uno, Signalpulse
  zeigen die Richtung, Hinweis auf 3,3 V beim RC522.
- [../docs/viewer/index.html](../docs/viewer/index.html): 3D-Modell des
  Steckbrett-Aufbaus nach dem Schaltplan. Die Firmware-Logik ist 1:1 nach
  JavaScript übertragen (Zustände, Zeiten, Töne über WebAudio, OLED als
  128x64-Framebuffer mit denselben GFX-Aufrufen, Admin-Menü, EEPROM im
  localStorage). Tasten, Karten und PIR sind anklickbar, Signalpulse laufen
  über die Kabel. Im Modell gelten Demo-PINs (`1234`/`0000`), nicht die echten.
- [../.github/workflows/pages.yml](../.github/workflows/pages.yml): veröffentlicht
  den Viewer auf GitHub Pages.
- [../README.md](../README.md) komplett neu geschrieben.

Getestet: SVGs und Viewer im Edge (headless) gerendert, die Viewer-Logik mit
einem Skript durchgespielt (PIN, Ausgangszeit, Bewegung → Alarm,
3 Fehlversuche, Admin-PIN, Karte anlernen/löschen, 7-Byte-UID abgelehnt,
Eingabe-Timeout). Alle Prüfungen sind bestanden.

**Offener Punkt:** GitHub Pages muss einmalig unter *Settings → Pages →
Source: GitHub Actions* aktiviert werden. Der Fehlerton dauert laut Sketch
`ERROR_TONE_MS = 2000` (2 s), Kommentare und CLAUDE.md sagen noch 3 s.

---

## Anhang — Stand der Dateien

| Datei | Zweck |
|---|---|
| [../Code/hardware_test/hardware_test.ino](../Code/hardware_test/hardware_test.ino) | Bring-up-Test aller Bauteile (vor dieser Sitzung entstanden) |
| [../Code/alarm_system/alarm_system.ino](../Code/alarm_system/alarm_system.ino) | Hauptfirmware: Zustandsautomat, PIN, Icons, Töne |
| [../Code/display_test/display_test.ino](../Code/display_test/display_test.ino) | Isolierter OLED-Test + SRAM-Anzeige zur Fehlersuche |
| [../CLAUDE.md](../CLAUDE.md) | Projektvorgaben, Pinbelegung, Hardware-Fakten, SRAM-Hinweis |

**Offener Punkt:** Die Reset-Diagnose (zu wenig freier SRAM) gilt erst als
bestätigt, wenn der neue `free SRAM:`-Wert im Seriellen Monitor deutlich über
300 liegt und die Resets ausbleiben. Kein Sketch
dieser Sitzung wurde kompiliert, da auf dem Rechner keine arduino-cli vorhanden
ist.
