# Befehlsübersicht – Roboter E09 (Version 2 „Baukasten“)

Wo kann man Befehle eingeben?
- **Werkbank** http://192.168.4.1:8080 → Eingabefeld oben, Enter
- **Baukasten** http://192.168.4.1:8080/blocks → als Block oder in der Textansicht
- **Seriell** (USB-Kabel, Serial Monitor 115200)

**Hardware-Seite** http://192.168.4.1:8080/hw: alle Motoren, Servos und Pins mit Live-Werten, Test-Knöpfen und **Namen**.
Ein Name ersetzt überall die Nummer bzw. den Messwert, z.B. `motor rad 1500` oder `wait until taster == 0` (siehe [ANSCHLUSS.md](ANSCHLUSS.md)).

Schreibweise: `<…>` = muss angegeben werden, `[…]` = darf weggelassen werden (dann gilt der Standard).
Statt Zahlen darf man überall auch **Rechnungen und Variablen** schreiben, z.B. `drive speed speed*0.5 300`.
**STOP** (roter Knopf) bricht jeden Befehl sofort ab.

---

## 1. Fahren & Linie

| Befehl | Was er macht | Beispiel |
|---|---|---|
| `drive <links> <rechts> [ms]` | Beide Fahrmotoren mit PWM −4095…4095. **Mit ms:** fährt so lange, dann Stopp. **Ohne ms:** fährt weiter, bis ein anderer Befehl kommt. | `drive 1000 1000 500` = 0,5 s geradeaus |
| `turn <1\|-1> [linien] [timeout_ms]` | Dreht auf der Stelle (1 = rechts, −1 = links), bis der Sensor die *n*-te Linie mittig sieht. Standard: 1 Linie, 10 s Timeout. | `turn -1 2` = links bis zur 2. Linie |
| `calibrate` | Liniensensor kalibrieren. **Roboter dreht sich hin und her** über der Linie. Muss vor `follow` einmal laufen. | `calibrate` |
| `follow [checkpoints] [timeout_ms]` | Linie folgen (PID) bis zur *n*-ten Querlinie. `0` = endlos. Standard: 1 Querlinie. | `follow 3` |
| `sensors [ms]` | Zeigt die 5 Liniensensor-Werte (+ Position) an, bei ms-Angabe wiederholt alle 0,2 s. | `sensors 3000` |
| `stop` | Alle Motoren aus. | |

Geschwindigkeit, PID usw. sind **Parameter** (siehe Abschnitt 5): `speed`, `kp`, `ki`, `kd`, `turnSpeed` …

## 2. Einzelne Motoren, Servos, Sensoren

| Befehl | Was er macht | Beispiel |
|---|---|---|
| `motor <1-6> <speed> [ms]` | Einen Motor drehen, speed −4095…4095. Ohne ms läuft er weiter. | `motor 3 1500 1000` |
| `brake [1-6]` | Motor aktiv bremsen (ohne Nummer: alle). | `brake` |
| `servo <1-8> <winkel>` | Servo auf Winkel (Grad) stellen. | `servo 1 90` |
| `servous <1-8> <us>` | Servo direkt mit Pulsbreite 500…2500 µs (zum Finden der Endlagen). | `servous 1 1500` |
| `servooff <1-8>` | Servo kraftlos schalten (lässt sich von Hand drehen). | `servooff 1` |
| `current <1-6>` | Motorstrom messen (A) – z.B. ob ein Motor blockiert. | `current 3` |
| `enc <1-2>` | Encoder-Zählerstand der Fahrmotoren. | `enc 1` |
| `battery` | Akkuspannung. Unter ~10,5 V laden! | |

## 3. Sortierrad

Zuerst Parameter `wheelMotor`, `wheelEncPin`, `exitPin` eintragen (Anleitung Teil F).

| Befehl | Was er macht | Beispiel |
|---|---|---|
| `wheelsens [ms]` | Rad-Sensor und Austritts-Sensor live anzeigen (zum Prüfen, ob sie reagieren). | `wheelsens 3000` |
| `wheelcal [ms]` | Dreht nur das Rad und stellt die Hell/Dunkel-Schwelle (`wheelThresh`, `wheelHyst`) automatisch ein. | `wheelcal` |
| `autocal` | Kalibriert **beide** Sensoren: dreht das Rad (misst Zahn/Lücke-Unterschied → `wheelThresh`/`wheelHyst`) und misst danach den Austritts-Sensor im Ruhezustand (kein Würfel) → `exitThresh`. Läuft automatisch bei jedem Einschalten, solange Parameter `wheelAutoCal` an ist. Danach `save`, damit es bleibt. | `autocal` |
| `wheel [taschen]` | Rad genau *n* Taschen weiterdrehen (1 Tasche = 36°, 10 = 1 Umdrehung). Bei Klemmen: kurz zurück, neu versuchen. | `wheel 1` |
| `feed [max_taschen]` | Tasche für Tasche drehen, bis ein Würfel am Austritt erkannt wird (höchstens *n* Taschen). | `feed 10` |

**Parameter dazu:** `wheelAutoCal` (an/aus, ob `autocal` beim Einschalten automatisch läuft – beim Austritts-Sensor sollte dabei kein Würfel davor liegen) und `autoCalMs` (wie lange dabei gedreht wird, Standard 3000 ms).

## 4. Steckplätze & Hardware-Suche (für neue Sensoren)

| Befehl | Was er macht | Beispiel |
|---|---|---|
| `gpio <0-6> <0\|1>` | MCP-Pin B0…B6 als Ausgang setzen (z.B. LED, Relais). | `gpio 2 1` |
| `gpioread <0-6>` | MCP-Pin B0…B6 lesen (mit Pull-up: Taster gegen GND → 0 = gedrückt). | `gpioread 0` |
| `pin <gpio> <0\|1>` | ESP32-Pin direkt setzen. | `pin 5 1` |
| `pinread <gpio>` | ESP32-Pin lesen, digital **und** analog. | `pinread 4` |
| `i2cscan` | Sucht alle I²C-Geräte. Normal: 0x20, 0x40, 0x44, 0x48, 0x49. Neuer I²C-Sensor → neue Adresse erscheint. | |
| `led <r> <g> <b>` | Farbe der LED auf dem ESP32 (0…255). | `led 0 255 0` |

## 5. Einstellungen (Parameter)

Parameter sind Zahlen, die man **ohne Neukompilieren** ändert (z.B. Geschwindigkeit, PID, Motornummern).

| Befehl | Was er macht | Beispiel |
|---|---|---|
| `params` | Alle Parameter mit Wert, Standard und Erklärung. | |
| `set <name> <wert>` | Parameter ändern – gilt sofort, aber nur bis zum Neustart. | `set speed 1200` |
| `save` | Aktuelle Parameter **dauerhaft** speichern. | |
| `defaults` | Alle Parameter zurück auf Standard. | |

In der Werkbank geht das bequemer über die Parameter-Tabelle (Wert ändern → „Speichern“).

## 6. Abläufe, Funktionen, Variablen

| Befehl | Was er macht | Beispiel |
|---|---|---|
| `run <skript> [arg1] [arg2] …` | Ein gespeichertes Skript (Funktion/Mission) ausführen. Kurzform: einfach den Namen schreiben. | `run folgeLinie 2` oder `folgeLinie 2` |
| `show <skript>` | Text eines Skripts anzeigen. | `show start` |
| `wait <ms>` | Warten. | `wait 500` |
| `vars` | Alle Variablen anzeigen. | |
| `eval <ausdruck>` | Rechnung/Bedingung testen, ohne etwas zu bewegen. | `eval qtr(2) > 500` |
| `help` | Liste aller Befehle (auf dem Roboter). | |

**Fertige Skripte:** `start` (Start-Knopf), `folgeLinie [n]`, `zurStation`, `hwtest`, `radTest`, `wuerfelAusgeben [n]`.
Im Baukasten kann man sie ändern und neue anlegen.

## 7. System

| Befehl | Was er macht | Beispiel |
|---|---|---|
| `wifi` | WLAN-Status anzeigen (eigenes Netz + Handy-Hotspot). | |
| `wifi <name> <passwort>` | Handy-Hotspot einspeichern → nach `reboot` hat der Roboter Internet-WLAN **und** sein eigenes Netz. Name mit Leerzeichen in "…". | `wifi "Fairphone 6" geheim123` |
| `wifi off` | Hotspot wieder vergessen. | |
| `reboot` | Neustart. | |

## 8. Beispiel-Befehle (zum Abschauen in `my-tasks.cpp`)

| Befehl | Was er macht |
|---|---|
| `grip [1\|0]` | Greifer an Servo `gripServo` zu (1) / auf (0). Winkel = Parameter `gripOpen`/`gripClosed`. |
| `tool [speed] [ms]` | Zusatzmotor `toolMotor` (Standard 4) eine Zeit lang drehen. |
| `waitbutton [timeout_ms]` | Wartet, bis ein Taster an B0 gedrückt wird. |

---

## Skript-Sprache (Baukasten-Textansicht)

| Zeile | Bedeutung |
|---|---|
| `repeat 3` … `end` | 3-mal wiederholen |
| `while <bedingung>` … `end` | wiederholen, solange Bedingung wahr |
| `if <bedingung>` … `else` … `end` | wenn / sonst |
| `let x = <rechnung>` | Variable setzen |
| `print Text {x}` | Text ausgeben, `{…}` wird ausgerechnet |
| `# Kommentar` | wird ignoriert |
| `<aktion> until <bedingung>` | Aktion laufen lassen, bis Bedingung wahr |

**`until` geht mit:** `follow`, `drive <l> <r>`, `turn <1|-1>`, `motor <n> <speed>`, `wait`. Beispiele:
```
follow until checkpoints >= 2
drive 800 800 until time > 1000
turn 1 until time > 300 and lineCenter
motor 3 1500 until cube
wait until gpio(0) == 0
```

**Werte, die man in Bedingungen/Rechnungen benutzen kann:**

| Name | Bedeutung |
|---|---|
| `time` | ms seit Start der aktuellen `until`-Aktion |
| `millis` | ms seit Einschalten |
| `checkpoints` | gezählte Querlinien in der aktuellen Aktion |
| `lineCenter` | 1, wenn die Linie mittig unter dem Sensor ist |
| `linePos` | Linienposition 0…4000 (2000 = Mitte) |
| `battery` | Akkuspannung |
| `cube` | 1, wenn am Austritt ein Würfel erkannt wird |
| `wheelSensor`, `exitSensor` | Rohwerte der Sortierrad-Sensoren |
| `qtr(0..4)` | Rohwert eines Liniensensors |
| `gpio(0..6)`, `pin(n)`, `analog(n)` | Steckplatz-/Pin-Werte |
| `enc(n)`, `current(n)` | Encoder / Motorstrom |
| `arg1` … `arg8` | Argumente, mit denen eine Funktion aufgerufen wurde |
| `abs()`, `min(a,b)`, `max(a,b)`, `random(a,b)` | Rechenfunktionen |
| jeder Parametername | z.B. `speed`, `turnSpeed` |

Verknüpfen mit `and`, `or`, `not`, vergleichen mit `> < >= <= == !=`, rechnen mit `+ - * / %` und Klammern.
