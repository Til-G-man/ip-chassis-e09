// =====================================================================
//  IP Motorshield – Hauptprogramm
//
//  Hier passiert nur das Aufstarten. Die eigentliche Arbeit steckt in:
//    my-tasks.cpp        <- EURE Befehle, Parameter und Missionen
//    robot.cpp/.h        <- Bausteine: fahren, Linie folgen, drehen, ...
//    robot-commands.cpp  <- eingebaute Befehle (motor, servo, drive, ...)
//    params / commands   <- Parameter- und Befehls-System
//    wifi-debug.cpp      <- Browser-Werkbank + kabelloses Hochladen
//
//  Bedienung: mit dem WLAN "IP-Motorshield-..." verbinden und
//  http://192.168.4.1:8080 öffnen.
// =====================================================================

#include <Arduino.h>

#include "commands.h"
#include "config.h"
#include "my-tasks.h"
#include "onboard-led.h"
#include "params.h"
#include "robot.h"
#include "shared-globals.h"
#include "wifi-debug.h"
#if ENABLE_WEB_INTERFACE
#include "web-interface.h"
#endif

Mcp23017 &mcp = shield.gpio();

bool autostart = !ENABLE_WEB_INTERFACE; // ohne WLAN: Mission "start" automatisch
constexpr float batteryWarn40PercentV = 13.1f;
constexpr float batteryWarn20PercentV = 12.9f;

#if ENABLE_WEB_INTERFACE
void webInterfaceTask(void *)
{
  for (;;)
  {
    wifiDebugWebLoop(); // Werkbank auf Port 8080
    TickType_t sleepTicks = pdMS_TO_TICKS(updateWebInterface());
    if (sleepTicks == 0)
    {
      sleepTicks = 1; // vTaskDelay(0) only yields; the core 0 idle task must run to feed the watchdog.
    }
    vTaskDelay(sleepTicks);
  }
}
#endif

// Befehle, die über das USB-Kabel (Serieller Monitor) getippt werden.
void readSerialCommands()
{
  static char line[160];
  static size_t length = 0;
  while (Serial.available() > 0)
  {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r')
    {
      if (length > 0)
      {
        line[length] = '\0';
        if (strcasecmp(line, "stop") == 0)
        {
          requestStop();
        }
        queueCommand(line);
        length = 0;
      }
    }
    else if (length < sizeof(line) - 1)
    {
      line[length++] = c;
    }
  }
}

// Läuft auch während langer Befehle (in waitMs / followLine / ...).
void background()
{
  wifiDebugOtaLoop();
  readSerialCommands();
  blinkPeriodically();
}

void warnIfBatteryLow()
{
  const float volts = shield.batteryVoltage();
  Debug.printf("Batterie: %.2f V\n", volts);
  if (volts < 0.1f)
  {
    Debug.println("Batterie-Warnung: keine Batterie angeschlossen oder Schalter aus.");
    blinkWarningPattern(128, 0, 0, 3);
  }
  else if (volts < batteryWarn20PercentV)
  {
    Debug.println("Batterie-Warnung: unter 20 %.");
    blinkWarningPattern(128, 0, 0, 5);
  }
  else if (volts < batteryWarn40PercentV)
  {
    Debug.println("Batterie-Warnung: unter 40 %.");
    blinkWarningPattern(128, 50, 0, 5);
  }
}

void setup()
{
  Serial.begin(115200);
  Debug.println("Motorshield startet...");
  initOnboardLed();
  setOnboardLedColor(255, 0, 0);

  if (!shield.begin())
  {
    Debug.println("Motorshield-Warnung: nicht alle I2C-Chips haben geantwortet.");
    blinkWarningPattern(128, 0, 255, 8); // violett: Shield-Fehler
  }

  qtr.setTypeRC();
  qtr.setSensorPins(QTR_SENSOR_PINS, QTR_SENSOR_COUNT);
  qtr.setTimeout(1000); // kleiner als Standard (2500) -> schnellere Regelschleife

  // Parameter und Befehle anmelden, dann gespeicherte Werte laden.
  registerRobotParams();
  addParam("autostart", &autostart, "Mission 'start' direkt nach dem Einschalten");
  registerRobotCommands();
  registerMyTasks();
  loadParams();
  setBackgroundHook(background);

  warnIfBatteryLow();

#if ENABLE_WEB_INTERFACE
  initWebInterface(linePid, &shield);
  wifiDebugBegin(shield); // braucht das WLAN, darum nach initWebInterface()
  xTaskCreatePinnedToCore(webInterfaceTask, "webInterface", 8192, nullptr, 1, nullptr, 0);
#endif

  clearOnboardLed();
  Debug.println("Bereit. 'help' zeigt alle Befehle.");

  if (autostart)
  {
    setOnboardLedColor(0, 200, 200); // cyan: Countdown
    delay(3000);
    setHeartbeatStandaloneActive();
    queueCommand("run start");
  }
}

void loop()
{
  background();
  if (wifiDebugMotionLocked())
  {
    // Upload läuft (oder ist fehlgeschlagen): nichts mehr bewegen.
    delay(50);
    return;
  }
  processCommandQueue();
  delay(1);
}
