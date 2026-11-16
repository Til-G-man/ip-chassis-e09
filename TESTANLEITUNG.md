# Testanleitung – Werkbank (Version 1) und Baukasten (Version 2)

Zwei Ordner, zwei Versionen:

| Ordner | Was | Browser |
|---|---|---|
| `ip-motorshield/` | **Version 1 – Werkbank**: Konsole, Parameter, Skripte, kabelloses Hochladen | http://192.168.4.1:8080 |
| `ip-motorshield-blocks/` | **Version 2 – Baukasten**: alles aus Version 1 **plus** Block-Editor mit wiederhole / wenn / bis / Variablen / eigenen Funktionen | http://192.168.4.1:8080/blocks |

Reihenfolge morgen: **erst Version 1 per Kabel** (Teil A–C), dann **Version 2 kabellos** (Teil D). Zeitbedarf: ca. 1–1.5 h.

Bei jedem Schritt steht **Erwartet:** – stimmt etwas nicht, steht darunter **Wenn nicht:**.
Notiert euch, was nicht klappt, und kopiert den Text aus dem Log (Browser) – damit kann ich den Fehler finden.

---

## Vorbereitung

- [ ] Akku geladen, Roboter **aufgebockt** (Räder in der Luft) für die ersten Tests
- [ ] Laptop mit Netzteil, Energiemodus „Beste Leistung“
- [ ] USB-Kabel (nur für den allerersten Upload)
- [ ] Teststrecke mit schwarzer Linie und mindestens 2 Querlinien
- [ ] Arduino IDE: Werkzeuge → Board **ESP32S3 Dev Module**, Partition Scheme **Default 4MB with spiffs** (so lassen wie bisher – sonst wird alles neu kompiliert)

> Beide Ordner habe ich schon einmal mit euren IDE-Einstellungen kompiliert. Das erste Kompilieren in der IDE sollte deshalb schnell gehen. Falls es doch lange dauert (z.B. Einstellungen geändert): einmal durchlaufen lassen, danach geht es in ~20–30 s.

---

## Teil A – Version 1 per Kabel hochladen

1. In der Arduino IDE `ip-motorshield/ip-motorshield.ino` öffnen.
2. Kabel anstecken, Werkzeuge → Port → den **COM-Port** wählen.
3. Hochladen (Pfeil-Knopf).
   - **Erwartet:** „Hochladen abgeschlossen“.
   - **Wenn nicht:** Boot-Taste am ESP32 gedrückt halten, Reset kurz drücken, Boot loslassen, nochmal hochladen.
4. Seriellen Monitor öffnen (115200 Baud), Reset am ESP32 drücken.
   - **Erwartet:** `Motorshield startet...`, Batteriespannung, `Werkbank: http://192.168.4.1:8080`, `Bereit.`
   - **Wichtig:** Der Roboter bewegt sich beim Start **nicht** mehr (früher hat er sofort kalibriert).
5. Im Seriellen Monitor `help` eintippen → Enter.
   - **Erwartet:** Liste aller Befehle.

## Teil B – Werkbank im Browser (Version 1)

### B1 Verbinden
1. Laptop-WLAN: **IP-Motorshield-E09-…** verbinden, Passwort `motorshieldE09`.
2. Browser: **http://192.168.4.1:8080**
   - **Erwartet:** Seite „Roboter-Werkbank“, oben grün **verbunden**, im Log die Startmeldungen.
   - **Wenn nicht:** Ist das WLAN wirklich verbunden (Windows meldet evtl. „Kein Internet“ – das ist normal)? Seite neu laden. Mobile Daten am Handy aus, falls ihr das Handy nehmt.

Ab jetzt kann das USB-Kabel ab.

### B2 STOP testen (aufgebockt!)
1. Konsole: `motor 1 800` → Enter (Motor läuft ohne Ende).
2. Roten **STOP**-Knopf drücken.
   - **Erwartet:** Motor stoppt sofort, Log zeigt `> stop`.
3. Nochmal `motor 1 800`, dann **Esc**-Taste drücken → muss ebenfalls stoppen.

### B3 Fahrwerk einstellen (aufgebockt)
1. `drive 800 800 1000` – beide Räder sollen 1 s **vorwärts** drehen.
   - Ein Rad dreht rückwärts → bei **Parameter** `invertLeft` bzw. `invertRight` anhaken.
   - Links/Rechts vertauscht (beim Test `drive 800 0 1000` dreht das rechte Rad) → `leftMotor` und `rightMotor` tauschen.
2. `drive 800 -800 1000` – Roboter würde nach **rechts** drehen (linkes Rad vorwärts, rechtes rückwärts).
3. Wenn alles stimmt: Parameter **Speichern**.

### B4 Weitere Hardware
- `servo 1 90`, dann `servo 1 0` – Servo bewegt sich?
- `motor 3 1500 1000` – Zusatzmotor 1 s
- `battery` – Spannung plausibel (~12–13 V)?
- `i2cscan` – **Erwartet:** 0x20, 0x40, 0x44, 0x48, 0x49 (das Shield). Weitere Adressen = zusätzliche Sensoren.
- `grip 1` / `grip 0` – Beispiel-Greifer an Servo 1 (Winkel über Parameter `gripOpen`/`gripClosed`).

### B5 Liniensensor (auf der Strecke)
1. Roboter **mittig auf die Linie** stellen.
2. `sensors 2000` – Rohwerte (noch unkalibriert).
3. `calibrate` – Roboter dreht sich links/rechts und wieder zur Mitte.
   - **Erwartet:** im Log `Kalibrierung min: … max: …`, Roboter steht wieder auf der Linie.
   - **Wenn nicht** (findet Linie nicht): Parameter `lineThreshold` kleiner (z.B. 200), nochmal.
4. `sensors 3000` – Roboter von Hand über die Linie schieben.
   - **Erwartet:** Werte 0–1000, Mittelsensor hoch auf der Linie, `Position` ≈ 2000 in der Mitte, `STOP` auf einer Querlinie.

### B6 Linie folgen
1. `follow 1` – fährt bis zur nächsten Querlinie und hält.
2. Während er fährt bei Parameter `kp` den Wert ändern (z.B. 0.2 / 0.5) – wirkt **sofort**.
   - Pendelt stark → `kp` kleiner. Kommt in Kurven nicht mit → `kp` grösser oder `speed` kleiner.
3. `turn 1` – dreht rechts bis zur nächsten Linie, `turn -1` links.
4. Gute Werte → **Speichern**.

### B7 Skript (Mission)
1. Unter **Skripte** `start` auswählen, ansehen, evtl. anpassen, **Speichern**.
2. **▶ Ausführen** – Roboter fährt die Mission. Mit STOP abbrechen geht jederzeit.

### B8 Speichern über Neustart
1. `set speed 1100`, dann **Speichern** (oder `save`).
2. `reboot`. Nach dem Neustart Seite neu laden.
   - **Erwartet:** `speed` = 1100 (blau markiert = anders als im Code).
3. **Standardwerte** setzt alles zurück.

### B9 Alte Telemetrie-Seite (optional)
http://192.168.4.1 → Connect → Plot läuft. **Start Moving** führt jetzt das Skript `start` aus, **Stop Moving** stoppt.

## Teil C – Kabellos hochladen (OTA) mit Version 1

1. Laptop ist im Roboter-WLAN, Kabel ist **ab**.
2. In `ip-motorshield/ip-motorshield.ino` in `setup()` nach `Debug.println("Bereit. ...");` eine Zeile einfügen:
   ```cpp
   Debug.println("Hallo OTA!");
   ```
3. Arduino IDE → Werkzeuge → Port → **Netzwerk-Ports → ip26-e09**.
   - **Wenn der Port fehlt:** 30 s warten; IDE neu starten; Windows-Firewall-Abfrage für die Arduino IDE erlauben („Private Netzwerke“); prüfen, dass der Laptop im Roboter-WLAN ist.
4. Hochladen. Passwort: `motorshieldE09`.
   - **Erwartet:** Motoren stoppen, Upload ~20–40 s, Roboter startet neu, im Browser-Log (Seite neu laden) steht `Hallo OTA!`.
   - **Wenn es abbricht:** Roboter bleibt still stehen (gewollt). Einfach nochmal hochladen oder Reset.
5. Zeit messen: Kompilieren + Upload. (Kompilieren sollte ~20–30 s dauern.)

✅ Wenn A–C klappen, ist die Grundlage fertig.

---

## Teil D – Version 2 (Baukasten), kabellos

### D1 Hochladen
1. In der IDE `ip-motorshield-blocks/ip-motorshield-blocks.ino` öffnen.
2. Port **ip26-e09** (Netzwerk) wählen, hochladen, Passwort wie oben.
   - **Erwartet:** Neustart, im Log `Baukasten: http://192.168.4.1:8080/blocks`.
3. Parameter und Skripte aus Version 1 bleiben erhalten (liegen im Flash).

### D2 Baukasten öffnen
http://192.168.4.1:8080/blocks (oder auf der Werkbank oben „Baukasten →“).
- **Erwartet:** links die Block-Palette (Ablauf, Bewegung bis…, Variablen, Befehle, Meine Funktionen), in der Mitte das Skript `folgeLinie` als Blöcke, rechts Log und Variablen.

### D3 Ausdrücke in der Konsole testen (Werkbank-Seite)
- `eval 1 + 2` → `= 3`
- `eval speed * 2` → doppelte Geschwindigkeit
- `eval qtr(2)` → Wert Mittelsensor (nach `calibrate`)
- `eval gpio(0)` → 1 (nichts angeschlossen) bzw. 0 (Taster gedrückt)
- `eval lineCenter and time > 0` → 0 oder 1

### D4 Erstes eigenes Skript (aufgebockt)
1. **Neu** klicken, Name `test1`.
2. Links **Fahren bis** klicken → Block erscheint. Felder: links `800`, rechts `800`, bis `time > 1000`.
3. **▶ Ausführen**.
   - **Erwartet:** Räder drehen genau ~1 s, dann Stopp. Log: `--- Skript 'test1' fertig ---`.
4. Nochmal ausführen und mittendrin **STOP** → `abgebrochen`.

### D5 Wiederholen + Variablen
1. Neues Skript `test2`:
   - **setze Variable** `zaehler = 0`
   - **wiederhole** `3` mal, darin (erst auf „+ hier einfügen“ im wiederhole-Block klicken, damit er gelb ist):
     - **servo** `1` `0`
     - **warte ms** `500`
     - **servo** `1` `90`
     - **warte ms** `500`
     - **erhöhe Variable** (`zaehler = zaehler + 1`)
     - **Ausgabe ins Log** `Runde {zaehler}`
2. Ausführen.
   - **Erwartet:** Servo wackelt 3×, Log zeigt Runde 1/2/3, rechts unter Variablen `zaehler = 3`.
3. Auf **Text** klicken → so sieht das Skript als Text aus (kann man auch direkt bearbeiten).

### D6 Linie folgen mit Bedingung (auf der Strecke)
1. Werkbank: `calibrate` (oder als ersten Block `calibrate` ins Skript).
2. Skript `test3`: **Linie folgen bis** `checkpoints >= 2`.
   - **Erwartet:** fährt über die 1. Querlinie hinweg und hält an der 2.
3. Variante: **Linie folgen bis** `time > 3000` → fährt 3 s.
4. Variante: **Drehen bis** `1` bis `time > 300 and lineCenter` → dreht rechts bis zur nächsten Linie.

### D7 Eigene Funktion
1. Skript `folgeLinie` öffnen – das ist eine **Funktion**: `arg1` ist das, was man beim Aufruf mitgibt.
2. In einem neuen Skript links unter **Meine Funktionen** `folgeLinie` klicken, Feld `2` eintragen → folgt bis zur 2. Querlinie.
3. `folgeLinie` ändern (z.B. `drive speed speed until time > 300`), speichern → wirkt überall, wo die Funktion benutzt wird.

### D8 Ganze Mission
Skript `start` öffnen (Beispiel mit wiederhole, wenn/sonst, Funktionen), an eure Strecke anpassen, **▶ Ausführen**.

### D9 Fehler absichtlich provozieren
1. Skript mit **warte bis** `qtr(9) > 1` ausführen.
   - **Erwartet:** Log `FEHLER Zeile 1: qtr(0..4)`, Roboter steht still.
2. Skript mit **Linie folgen bis** ohne vorheriges `calibrate` nach einem Neustart → `erst 'calibrate' ausführen`.

---

## Teil E – Internet und Roboter gleichzeitig (Handy-Hotspot, nur Version 2)

Idee: Laptop **und** Roboter verbinden sich mit dem Hotspot eines Handys. Der Laptop hat dann Internet und erreicht trotzdem den Roboter. Das eigene WLAN `IP-Motorshield-E09-…` bleibt **immer zusätzlich** an – klappt der Hotspot nicht, ist alles wie vorher.

1. Am Handy den **Hotspot einschalten**. Name und Passwort notieren (Passwort mind. 8 Zeichen, **ohne `#`**).
   - iPhone: Einstellungen → Persönlicher Hotspot → „Anderen erlauben“ an, und **„Kompatibilität maximieren“** an (sonst nutzt das iPhone 5 GHz, das der ESP32 nicht kann).
   - Android: Hotspot auf **2.4 GHz** stellen, falls einstellbar.
2. Laptop noch im Roboter-WLAN: Werkbank-Konsole
   ```
   wifi MeinHandy meinpasswort
   ```
   (Name mit Leerzeichen: `wifi "Mein Handy" meinpasswort`) → dann `reboot`.
3. Laptop mit dem **Handy-Hotspot** verbinden (jetzt hat er Internet).
4. Browser: **http://ip26-e09.local:8080**
   - **Wenn das nicht geht:** Laptop kurz zurück ins Roboter-WLAN, Konsole `wifi` → zeigt die IP im Hotspot (z.B. `172.20.10.5`), dann im Hotspot **http://172.20.10.5:8080** öffnen. Die IP steht auch beim Start im Log.
5. **Kabellos hochladen** geht auch im Hotspot: IDE → Port → Netzwerk-Ports → `ip26-e09`.
6. Ausschalten: `wifi off`, dann `reboot`.

Hinweise:
- Findet der Roboter den Hotspot beim Einschalten nicht (10 s), versucht er es erst beim nächsten Neustart wieder → Handy-Hotspot **vor** dem Roboter einschalten.
- Mit Hotspot funkt der Roboter etwas stärker (11 dBm statt 5 dBm), damit das Handy nicht direkt daneben liegen muss.
- Eduroam (ETH-WLAN) geht dafür **nicht**.

---

## Teil F – Sortierrad (nur Version 2)

Grundlage: CAD Sorter v9 (Rad Ø140, 10 Taschen, TT-Motor, 2× Pololu QTR-HD-01RC).
Die Anschlüsse sind **Parameter** – einstellen, kein Neu-Kompilieren nötig.

1. **Anschlüsse eintragen** (Werkbank → Parameter, danach **Speichern**):
   - `wheelMotor` = Motor-Anschluss des Rad-Motors (M1–M6, Standard 3)
   - `wheelEncPin` = ESP32-Pin des Rad-Sensors (hinter dem Rad)
   - `exitPin` = ESP32-Pin des Austritts-Sensors
   - Die QTR-HD-01RC brauchen **direkte ESP32-Pins** (nicht MCP B0–B6), dazu 3.3 V und GND. Freie Pins vorher mit dem Pinout-PDF prüfen.
2. `wheelsens 5000` – Werte live. Rad von Hand drehen:
   - **Erwartet:** Rad-Wert springt zwischen „hell“ (klein) und „dunkel“ (gross).
   - Würfel vor den Austritts-Sensor halten → „WÜRFEL“. Sonst `exitThresh` anpassen (zwischen den Werten mit/ohne Würfel), oder `exitInvert` anhaken, wenn es genau umgekehrt ist.
3. `wheelcal` – Rad dreht 4 s, die Schwelle `wheelThresh` wird automatisch gesetzt → **Speichern**.
   - Dreht das Rad falsch herum → `wheelSpeed` negativ machen (z.B. −1500).
4. `wheel 1` – genau **eine Tasche** (36°) weiter. `wheel 10` – genau **eine Umdrehung**.
   - Zu weit/zu kurz → `edgesPerPocket` prüfen (CAD sagt 4). Überdreht leicht → `wheelBrake` an, `wheelSpeed` kleiner.
5. `feed` – dreht Tasche für Tasche, bis ein Würfel durch den Austritt fällt.
6. **Klemmen testen:** Rad beim `wheel 10` mit der Hand festhalten → „Rad klemmt – kurz zurück“, nach 3 Versuchen „Rad blockiert“.
7. Im Baukasten: Funktionen `radTest` und `wuerfelAusgeben` (z.B. `wuerfelAusgeben 3`), Bedingung `cube` z.B. in **warte bis** `cube`.

---

## Zurück zu Version 1
`ip-motorshield/ip-motorshield.ino` öffnen und kabellos hochladen. Achtung: Skripte, die mit dem Baukasten gemacht wurden (`until`, `let`, `repeat` …), kann Version 1 nicht ausführen.

## Was ich danach von euch brauche
- Welche Schritte haben **nicht** geklappt (Nummer, z.B. „B5.3“)?
- Text aus dem Log dazu (markieren → kopieren).
- Gemessene Zeiten: Kompilieren und kabelloser Upload.
- Was war im Baukasten umständlich / was fehlt?

## Spickzettel Bedingungen (Baukasten)

| Bedingung | Bedeutung |
|---|---|
| `checkpoints >= 2` | 2 Querlinien seit Start des Blocks |
| `time > 3000` | 3 s seit Start des Blocks |
| `lineCenter` / `not lineCenter` | Mittelsensor sieht Linie / sieht keine |
| `linePos < 1500` | Linie liegt links (0 = ganz links, 4000 = ganz rechts) |
| `qtr(0) > 600` | einzelner Liniensensor 0..4 |
| `gpio(0) == 0` | Taster an MCP-Pin B0 gedrückt (gegen GND) |
| `pin(6) == 1`, `analog(6) > 2000` | ESP32-Pin digital / analog |
| `enc(1) > 500` | Encoder Motor 1 |
| `battery < 11.5` | Akku fast leer |
| `a and b`, `a or b`, `not a` | verknüpfen |
