#pragma once
#include <Arduino.h>

// =====================================================================
//  GERÄTE-ÜBERSICHT  (Webseite http://192.168.4.1:8080/hw)
//
//  Jeder Anschluss des Motorshields kann einen Namen bekommen:
//    Motoren 1-6, Servos 1-8, MCP-Pins B0-B6, freie ESP32-Pins.
//  Die Namen werden im Roboter gespeichert und gelten sofort überall:
//    * Motor/Servo/Ausgang:  Name = Anschluss-Nummer
//         motor rad 1500      (statt motor 3 1500)
//         servo klappe 90
//    * Eingang/Sensor:       Name = aktueller Messwert
//         wait until taster == 0
//         if austritt < 400
// =====================================================================

enum DeviceKind
{
    DevMotor,
    DevServo,
    DevMcp,
    DevPin
};

enum DeviceMode
{
    ModeIn = 0,     // digitaler Eingang mit Pull-up (Taster, Endschalter)
    ModeOut = 1,    // Ausgang (LED, Relais, ...)
    ModeAnalog = 2, // Analogwert 0..4095 (nur ESP32-Pins mit ADC1)
    ModeRc = 3      // Pololu-RC-Sensor (QTR-...RC), Zeit in us
};

void loadDevices();

// Name/Modus setzen (leerer Name = Name löschen). error enthält den Grund bei false.
bool setDevice(DeviceKind kind, int number, const String &name, int mode, String &error);

// Für Ausdrücke: Gerätename -> Nummer (Aktoren) bzw. Messwert (Eingänge)
bool lookupDevice(const char *name, float &value);

// Von den Befehlen gpio/pin aufgerufen, damit die Übersicht den Ausgang kennt
void noteDeviceOutput(DeviceKind kind, int number, int value);

void hardwareToJson(String &out, bool withCurrents); // /hw: alles mit Live-Werten
void deviceNamesToJson(String &out);                 // /info: nur vergebene Namen
