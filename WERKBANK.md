# Roboter-Werkbank – Kurzanleitung

Ziel: Roboter testen und verbessern, **ohne jedes Mal neu zu kompilieren** und **ohne Kabel**.

Es gibt drei Geschwindigkeitsstufen, etwas zu ändern:

| Was ändern? | Wie | Dauer |
|---|---|---|
| Eine Zahl (Geschwindigkeit, PID, Servo-Winkel, …) | Browser → Parameter | sofort |
| Ablauf der Mission (Reihenfolge der Aufgaben) | Browser → Skripte | sofort |
| Neue Funktion / neuer Sensor / neuer Motor | Code in `my-tasks.cpp`, kabellos hochladen | 1 Kompilieren + ~20 s Upload |

## 1. Verbinden

1. Roboter einschalten. Er bewegt sich beim Start **nicht** mehr (kein automatisches Kalibrieren).
2. Laptop mit WLAN `IP-Motorshield-E09-…` verbinden (Passwort aus `config.h`).
3. Browser: **http://192.168.4.1:8080** → Werkbank.
   (Die alte Telemetrie-Seite mit Plot ist weiterhin auf http://192.168.4.1.)

Der rote **STOP**-Knopf (oder die Taste `Esc`) bricht jeden laufenden Befehl ab.

## 2. Befehle

In die Konsole tippen (oder im Seriellen Monitor, 115200 Baud):

```
help                     alle Befehle
motor 3 1500 1000        Motor 3 mit PWM 1500 für 1 s
servo 1 90               Servo 1 auf 90°
drive 1000 1000 500      geradeaus 0.5 s
calibrate                Liniensensor kalibrieren (Roboter dreht sich!)
follow 2                 Linie folgen bis zur 2. Querlinie
turn 1                   rechts drehen bis zur nächsten Linie (-1 = links)
sensors 3000             Liniensensor 3 s lang anzeigen
i2cscan                  angeschlossene I2C-Sensoren finden
pinread 6                ESP32-Pin lesen (neuer Sensor)
set speed 1200           Parameter ändern
save                     Parameter dauerhaft speichern
run start                Mission "start" ausführen
```

Mehrere Befehle in einer Zeile: `drive 800 800 500; servo 1 0`.
Statt einer Zahl darf ein Parametername stehen: `drive speed speed 1000`.

## 3. Missionen (Skripte)

Im Browser unter **Skripte**: ein Befehl pro Zeile, `#` = Kommentar.

```
calibrate
follow 2        # bis zur 2. Querlinie
grip 1          # Greifer zu
turn 1 1
follow 1
grip 0
```

**Speichern** legt das Skript im Roboter ab (bleibt nach Neustart). Der Start-Knopf auf der alten Seite
führt das Skript `start` aus. Mit dem Parameter `autostart = 1` (und `save`) startet `start` direkt
nach dem Einschalten – für den Wettbewerb ohne Laptop.

## 4. Neue Funktion programmieren (`my-tasks.cpp`)

```cpp
int shootSpeed = 2000;                        // 1. Einstellung
void cmdShoot(const Args &args) {             // 2. Funktion
    int balls = args.getInt(0, 1);            //    1. Argument, Standard 1
    for (int i = 0; i < balls; i++) {
        shield.motor(4).setSpeed(shootSpeed);
        if (!waitMs(800)) break;              //    STOP gedrückt -> aufhören
        shield.motor(4).stop();
        if (!waitMs(300)) break;
    }
    shield.motor(4).stop();
}

void registerMyTasks() {
    addParam("shootSpeed", &shootSpeed, 0, 4095, "PWM Schussrad");   // im Browser änderbar
    addCommand("shoot", "shoot [anzahl] - Bälle schiessen", cmdShoot); // als Befehl aufrufbar
    ...
}
```

Danach: `shoot 3` in der Konsole, oder `shoot` in eine Mission schreiben.

Regeln: immer `waitMs()` statt `delay()`, in Schleifen `stopRequested()` prüfen, Ausgaben mit `Debug.println()`.

## 5. Kabellos hochladen (OTA)

Voraussetzung: einmal per Kabel hochladen, danach nie mehr nötig.

1. Laptop ist mit dem Roboter-WLAN verbunden.
2. Arduino IDE → **Werkzeuge → Port → Netzwerk-Ports → `ip26-e09`** auswählen.
   (Erscheint der Port nicht: IDE neu starten, oder 30 s warten.)
3. Hochladen klicken. Passwort = WLAN-Passwort aus `config.h`.
4. Der Roboter stoppt alle Motoren, lädt, und startet neu. Log im Browser lädt sich selbst neu.

Board-Einstellung muss bleiben: **ESP32S3 Dev Module**, Partition Scheme mit **OTA**
(Standard „Default 4MB with spiffs“ ist ok).

## 6. Wo ist was?

| Datei | Inhalt |
|---|---|
| `my-tasks.cpp` | **eure** Befehle, Parameter und Standard-Missionen |
| `robot.h / robot.cpp` | Bausteine: `drive`, `driveFor`, `followLine`, `turnUntilLine`, `calibrateLine`, `readLine` |
| `robot-commands.cpp` | eingebaute Befehle (`motor`, `servo`, `drive`, `follow`, …) |
| `params.*` | Parameter-System (Browser, Flash-Speicher) |
| `commands.*` | Befehls-System, Skripte, STOP, `waitMs` |
| `wifi-debug.*` | Browser-Werkbank, Log, kabelloses Hochladen |
| `line-follower.*` | PID-Regler und Linien-Erkennung (NORMAL / STOP / STATION / ROTATION) |
| `web-interface.*` | alte Telemetrie-Seite (Plot) + WLAN-Zugangspunkt |
| `src/` | Treiber für die Chips auf dem Motorshield (nicht anfassen nötig) |
| `config.h`, `pinout.h` | feste Einstellungen und Pinbelegung |
