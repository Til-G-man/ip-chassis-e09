# Was wird wo angeschlossen? – Motorshield V4 (E09)

Quelle: `docs/Pinout_Motorshield_ESP32_IP26_V4_25082026.pdf` (Bild auf Seite 1 zeigt die Stecker).
**Immer erst den Akku ausschalten, dann umstecken.** Alle ESP32-Pins vertragen **höchstens 3,3 V**.

Nach dem Anschliessen: http://192.168.4.1:8080/hw öffnen → Wert prüfen → Namen eintragen (Enter).

## Belegung für unseren Roboter

| Teil | Anschluss | Kabel |
|---|---|---|
| Antrieb links / rechts | **DC-Motor 1 / 2** (Schraubklemmen) | + / − |
| Sortierrad (TT-Motor) | **DC-Motor 3** | + / − |
| Sortierrad Rad-Sensor (QTR-HD-01RC) | OUT → **GPIO 14**, VIN → **3V3**, GND → **GND** | 3 Kabel |
| Sortierrad Austritts-Sensor (QTR-HD-01RC) | OUT → **GPIO 48**, VIN → **3V3**, GND → **GND** | 3 Kabel |
| Liniensensor (5×) | schon fest: GPIO 16, 15, 41, 7, 42 | – |
| weitere Motoren | DC-Motor 4, 5, 6 | + / − |
| Servos | Servo 1–8 | Signal / 5V / GND |

GPIO 14 und 48 sind schon als Standard eingestellt. Wer andere Pins nimmt, stellt sie auf der Hardware-Seite unter „Sortierrad“ um.

## Welche Art Teil wohin

| Teil | Wohin | Hinweis |
|---|---|---|
| **DC-Motor** | Schraubklemme DC-Motor 1–6 | Dreht falsch herum → Kabel tauschen oder negatives Tempo |
| **Servo** | Stiftleiste Servo 1–8 | Signal (gelb/orange/weiss) auf die Signal-Reihe, rot = 5V, braun/schwarz = GND |
| **RC-Liniensensor** (Pololu QTR-…RC) | freier ESP32-Pin, Modus „RC-Sensor“ | VIN an 3V3. Nicht GPIO 39/40 (deren Pull-up verfälscht die Messung) |
| **Taster / Endschalter** | MCP **B0–B6** + GND | Pull-up ist eingebaut: offen = 1, gedrückt = 0 |
| **Analoger Sensor** (Poti, IR-Distanz) | **GPIO 6 oder 10** | Nur diese zwei können mit WLAN analog messen |
| **LED / Relais** | MCP B0–B6 oder freier ESP32-Pin, Modus „Ausgang“ | LED mit ~220 Ω Vorwiderstand |
| **I²C-Sensor** | Qwiic-Buchse oder SDA = GPIO 8, SCL = GPIO 9 | Mit `i2cscan` prüfen, ob die Adresse erscheint |

**Freie ESP32-Pins:** 6, 14, 48, 39, 40 und 10–13 (diese nur, solange kein SPI benutzt wird).

**Nicht benutzen:** GPIO 1, 2, 4, 5 (Motoren 1/2), 17, 18, 21, 47 (Encoder), 7, 15, 16, 41, 42 (Liniensensor),
8, 9 (I²C), 38 (RGB-LED), 0, 3, 19, 20, 43–46 (Boot/USB/Testpunkte), MCP B7.

Die GPIO-Nummern stehen auf der Platine neben dem Header. Die Header-Reihen haben je eine Spalte
3V3 (untere Reihen, „3V3 Header“) bzw. GND (obere Reihen, „GND Header“), siehe Bild im Pinout-PDF.

## Namen benutzen

Auf der Hardware-Seite vergebene Namen gelten sofort überall (Werkbank, Skripte, Baukasten):

- **Motor, Servo, Ausgang:** Name = Nummer → `motor rad 1500 500`, `servo klappe 90`, `gpio lampe 1`
- **Eingang/Sensor:** Name = Messwert → `wait until taster == 0`, `if austritt < 400`, `print {radSensor}`

Im Baukasten erscheinen die Namen in den ▾-Vorschlägen neben jedem Feld.
