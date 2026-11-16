// =====================================================================
//  MY TASKS  –  HIER ARBEITET IHR.
//
//  Eine neue Fähigkeit des Roboters hinzufügen geht in 3 Schritten:
//    1. Variablen für die Einstellungen anlegen und mit addParam()
//       anmelden  -> im Browser änderbar, ohne neu zu kompilieren.
//    2. Eine Funktion  void cmdXyz(const Args &args)  schreiben und mit
//       addCommand() anmelden -> im Browser/Seriell als "xyz" aufrufbar.
//    3. Die Befehle im Baukasten (http://192.168.4.1:8080/blocks) zu
//       Funktionen und Missionen zusammenklicken – oder hier mit
//       addDefaultScript() als Text vorgeben.
//
//  Regeln:
//    * Statt delay() immer waitMs() benutzen (sonst geht STOP nicht).
//      if (!waitMs(500)) return;   // STOP gedrückt -> sofort aufhören
//    * In Schleifen regelmässig  if (stopRequested()) { ...; return; }
//    * Ausgaben mit Debug.println(...) – erscheinen im Browser-Log.
//
//  Fertige Bausteine: siehe robot.h (drive, followLine, turnUntilLine,
//  shield.motor(n), shield.servo(n), shield.gpio(), ...).
// =====================================================================

#include "my-tasks.h"

#include "caliper.h"
#include "commands.h"
#include "params.h"
#include "robot.h"
#include "sorter.h"
#include "wifi-debug.h"

// ---------------------------------------------------------------------
//  Beispiel 1: Greifer an Servo 1
// ---------------------------------------------------------------------
int gripperServo = 1;
float gripperOpenDeg = 30;
float gripperClosedDeg = 120;

void cmdGrip(const Args &args)
{
    // grip 1 = schliessen, grip 0 = öffnen
    const bool close = args.getInt(0, 1) != 0;
    shield.servo(gripperServo).setAngle(close ? gripperClosedDeg : gripperOpenDeg);
    waitMs(400); // dem Servo Zeit geben
}

// ---------------------------------------------------------------------
//  Beispiel 2: Zusatzmotor (z.B. Förderband/Schussrad) für eine Zeit
// ---------------------------------------------------------------------
int toolMotor = 4; // 3 ist als Standard für das Sortierrad vorgesehen
int toolSpeed = 1500;
int toolTimeMs = 2000;

void cmdTool(const Args &args)
{
    // tool [speed] [ms]  – ohne Argumente gelten die Parameter
    const int speed = args.getInt(0, toolSpeed);
    const int ms = args.getInt(1, toolTimeMs);
    shield.motor(toolMotor).setSpeed(speed);
    waitMs(ms);
    shield.motor(toolMotor).stop();
}

// ---------------------------------------------------------------------
//  Beispiel 3: Neuer Sensor – Taster an MCP-Pin B0 (gegen GND)
// ---------------------------------------------------------------------
void cmdWaitButton(const Args &args)
{
    // waitbutton [timeout_ms]: wartet, bis der Taster gedrückt wird
    const uint32_t timeout = args.getInt(0, 10000);
    shield.gpio().pinMode(Mcp23017::Pin::B0, INPUT_PULLUP);
    const uint32_t start = millis();
    while (shield.gpio().digitalRead(Mcp23017::Pin::B0) == HIGH)
    {
        if (millis() - start > timeout || !waitMs(10))
        {
            Debug.println("waitbutton: kein Tastendruck.");
            return;
        }
    }
    Debug.println("Taster gedrückt.");
}

void registerMyTasks()
{
    addParam("gripServo", &gripperServo, 1, 8, "Servo-Anschluss des Greifers");
    addParam("gripOpen", &gripperOpenDeg, 0, 270, "Greifer offen (Grad)");
    addParam("gripClosed", &gripperClosedDeg, 0, 270, "Greifer zu (Grad)");
    addParam("toolMotor", &toolMotor, 1, 6, "Motor-Anschluss des Werkzeugs");
    addParam("toolSpeed", &toolSpeed, -4095, 4095, "PWM des Werkzeugs");
    addParam("toolTimeMs", &toolTimeMs, 0, 60000, "Laufzeit des Werkzeugs");

    addCommand("grip", "grip [1|0] - Greifer schliessen/öffnen", cmdGrip);
    addCommand("tool", "tool [speed] [ms] - Werkzeugmotor laufen lassen", cmdTool);
    addCommand("waitbutton", "waitbutton [timeout_ms] - auf Taster an B0 warten", cmdWaitButton);

    // Sortierrad (sorter.cpp): Parameter wheel..., Befehle wheel/feed/wheelcal/wheelsens
    registerSorter();
    addDefaultScript("wuerfelAusgeben",
                     "# Funktion: arg1 Würfel einzeln ausgeben (Standard 1)\n"
                     "repeat max(arg1, 1)\n"
                     "  feed 10\n"
                     "  wait 300\n"
                     "end\n");
    addDefaultScript("radTest",
                     "# Sortierrad prüfen: Sensoren, Kalibrieren, 1 Umdrehung\n"
                     "wheelsens 2000\n"
                     "wheelcal 4000\n"
                     "wait 500\n"
                     "wheel 10\n"
                     "print Rad sollte genau 1 Umdrehung gemacht haben\n");

    // Skripte = Funktionen und Missionen. Im Baukasten (http://192.168.4.1:8080/blocks)
    // änderbar – die dort gespeicherte Version hat Vorrang vor dieser hier.
    // "start" wird vom Start-Knopf (und autostart) ausgeführt.
    addDefaultScript("folgeLinie",
                     "# Funktion: Linie folgen bis zur arg1-ten Querlinie (Standard 1),\n"
                     "# dann kurz weiterfahren, damit wir nicht auf der Querlinie stehen.\n"
                     "follow until checkpoints >= max(arg1, 1)\n"
                     "drive speed speed until time > 150\n");
    addDefaultScript("zurStation",
                     "folgeLinie 1\n"
                     "grip 1\n"
                     "turn 1 until time > 300 and lineCenter\n");
    addDefaultScript("start",
                     "calibrate\n"
                     "let runde = 0\n"
                     "repeat 2\n"
                     "  folgeLinie 1\n"
                     "  let runde = runde + 1\n"
                     "  print Runde {runde} geschafft\n"
                     "end\n"
                     "if gpio(0) == 0\n"
                     "  grip 1\n"
                     "else\n"
                     "  turn 1 until time > 300 and lineCenter\n"
                     "end\n"
                     "folgeLinie 1\n"
                     "grip 0\n");
    addDefaultScript("hwtest",
                     "battery\n"
                     "i2cscan\n"
                     "drive 800 800 until time > 500\n"
                     "drive -800 -800 until time > 500\n"
                     "servo 1 90\n"
                     "sensors 2000\n");
}
