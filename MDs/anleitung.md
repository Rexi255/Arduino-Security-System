<!--
Prompt für Claude (claude.ai): Erstelle aus dem folgenden Inhalt eine kurze,
einfache PDF-Bedienungsanleitung (1 Seite, wenn möglich). Zielgruppe sind
Endnutzer des Sicherheitssystems, keine Techniker - klar, kurz, ohne
Code-Details. Gerne mit den Symbolen (Herz/Totenkopf) als einfache Icons oder
Beschreibung, sauber gegliedert mit den Abschnitten unten als
Überschriften/Tabelle.
-->

# Bedienungsanleitung — Mini-Sicherheitssystem

Kleines Zugangs-/Alarmsystem mit Keypad, Display und RFID-Kartenleser.

## Anzeige

| Symbol | Bedeutung |
|---|---|
| ❤️ Herz | Unscharf — Anlage ist aus |
| 💀 Totenkopf (steht) | Scharf, oder Ausgangsverzögerung läuft |
| 💀 Totenkopf (blinkt) | ALARM |
| ✖️ Kreuz (kurz) | Falsche PIN oder unbekannte Karte |

## Scharf- / Unscharfschalten

**Mit PIN-Code:**
1. PIN über das Tastenfeld eingeben (erscheint als `*`)
2. Mit `#` bestätigen
3. Bei falscher Eingabe: `*` löscht die Eingabe und man kann neu beginnen

**Mit Chipkarte:**
- Karte einfach an den Kartenleser halten — fertig

Beide Wege funktionieren identisch: aus dem unscharfen Zustand schalten sie
scharf, aus scharf/Alarm schalten sie wieder unscharf.

Nach dem Scharfschalten bleiben **10 Sekunden Zeit**, um den Raum zu
verlassen (kurzes Piepen), bevor die Anlage wirklich scharf ist.

## Alarm

- Der Alarm löst aus bei Bewegung (nur im scharfen Zustand) oder nach
  **3 falschen** PIN-Eingaben bzw. unbekannten Karten.
- Der Alarm dauert 30 Sekunden oder endet sofort mit der richtigen PIN oder
  einer bekannten Karte.

## Neue Karten hinzufügen oder löschen

Karten werden direkt am Gerät verwaltet — dafür ist kein PC nötig.

1. Im unscharfen Zustand Taste **A** drücken
2. Die Admin-PIN eingeben und mit `#` bestätigen
3. Im Menü:
   - **1** = neue Karte anlernen → Karte an den Leser halten
   - **2** = gespeicherte Karten durchblättern (`#`) oder löschen (`D`)
   - **\*** = Menü verlassen

Das Menü schließt sich nach 20 Sekunden ohne Tastendruck automatisch.

## Wichtig

- PIN-Code und Admin-PIN nicht weitergeben.
- Die Admin-PIN ermöglicht das Anlernen neuer Karten — entsprechend
  vertraulich behandeln.
