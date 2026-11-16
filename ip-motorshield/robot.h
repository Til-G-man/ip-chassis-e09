#pragma once
#include <Arduino.h>
#include <QTRSensors.h>

#include "line-follower.h"
#include "src/motorshield/motorshield.h"

// =====================================================================
//  ROBOTER-BAUSTEINE  –  fertige Funktionen, die man in eigenen Befehlen
//  (my-tasks.cpp) benutzen kann. Alle "langen" Funktionen geben false
//  zurück, wenn STOP gedrückt wurde oder ein Timeout kam.
// =====================================================================

extern Motorshield shield; // shield.motor(1..6), shield.servo(1..8), shield.gpio()
extern QTRSensors qtr;     // Liniensensor (5 Sensoren)
extern PIDController linePid;

// Fahrwerk-Parameter (im Browser änderbar)
extern int leftMotorNumber;  // welcher Motor-Anschluss (1..6) ist das linke Rad
extern int rightMotorNumber; // welcher Motor-Anschluss (1..6) ist das rechte Rad
extern bool invertLeft;      // Drehrichtung umkehren, falls das Rad rückwärts dreht
extern bool invertRight;
extern int turnSpeed;     // PWM beim Drehen auf der Stelle
extern int lineThreshold; // ab diesem Sensorwert gilt "schwarz"
extern bool lineCalibrated;

void registerRobotParams();
void registerRobotCommands();

// --- Motoren ---------------------------------------------------------
void stopAllMotors();                              // alle 6 Motoren aus
void drive(int left, int right);                   // -4095..4095, läuft weiter bis zum nächsten Aufruf
bool driveFor(int left, int right, uint32_t ms);   // fährt ms lang, dann Stopp
void stopDrive();                                  // Fahrmotoren stoppen (bremsen falls brakeOnStop)

// --- Liniensensor ----------------------------------------------------
bool calibrateLine();       // dreht links/rechts und lernt Schwarz/Weiss
uint16_t readLine();        // 0 (ganz links) .. 4000 (ganz rechts), 2000 = Mitte
bool centerOnLine();        // Mittelsensor sieht die Linie?
void printLineSensors(Print &out);

// Folgt der Linie bis "checkpoints" Querlinien überfahren wurden
// (0 = endlos bis STOP/Timeout). timeoutMs = 0 -> kein Timeout.
bool followLine(int checkpoints, uint32_t timeoutMs);

// Dreht auf der Stelle (direction +1 = rechts, -1 = links), bis der
// Mittelsensor "lines" mal eine neue Linie gefunden hat.
bool turnUntilLine(int direction, int lines, uint32_t timeoutMs);
