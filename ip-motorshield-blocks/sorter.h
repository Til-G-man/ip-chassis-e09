#pragma once
#include <Arduino.h>

// =====================================================================
//  SORTIERRAD  (CAD: Produktion/Loading mechanism/Wheel_v2, Sorter v9)
//
//  Mechanik laut CAD:
//    * Rad Ø140 mm, 10 Taschen (Würfel 15 mm) -> 36° pro Tasche
//    * Antrieb: TT-Getriebemotor an einem Motor-Anschluss (Parameter wheelMotor)
//    * Rad-Positionssensor: Pololu QTR-HD-01RC schaut durch die Rückwand auf
//      die Speichen bei r = 66 mm. Speichenkanten + Kerbe in jeder Speiche
//      -> 40 Hell/Dunkel-Wechsel ("Flanken") pro Umdrehung = 4 pro Tasche
//    * Austritts-Sensor: zweiter QTR-HD-01RC unter dem Kanalboden, sieht den
//      Würfel, wenn er durch den Austritt fällt
//
//  Beide Sensoren sind RC-Sensoren und brauchen je einen DIREKTEN ESP32-Pin
//  (nicht den MCP). Pins als Parameter einstellen: wheelEncPin, exitPin.
//
//  Befehle (auch als Blöcke im Baukasten):
//    wheelsens [ms]       beide Sensoren live anzeigen (zum Einstellen)
//    wheelcal [ms]        Rad drehen, Schwelle des Rad-Sensors automatisch setzen
//    wheel [taschen]      Rad genau n Taschen weiterdrehen (Standard 1)
//    feed [max]           Tasche für Tasche drehen, bis ein Würfel austritt
//  Bedingungen im Baukasten:
//    cube                 1 = Austritts-Sensor sieht gerade einen Würfel
//    wheelSensor, exitSensor   Rohwerte (Mikrosekunden, klein = hell)
// =====================================================================

void registerSorter();

// Kalibriert beim Einschalten (wenn wheelAutoCal=true) oder per Befehl 'autocal':
// dreht das Rad einmal, misst hell/dunkel am Rad-Sensor -> wheelThresh/wheelHyst,
// misst den Austritts-Sensor im Ruhezustand (kein Würfel) -> exitThresh.
void autoCalibrateSorter();

// Ruft autoCalibrateSorter() nur auf, wenn der Parameter wheelAutoCal gesetzt ist.
// Im setup() nach loadParams() aufrufen.
void maybeAutoCalibrateOnBoot();

bool rotateWheelPockets(int pockets, bool stopOnCube);
bool cubeAtExit();
int readWheelSensor(); // -1 = Pin nicht eingestellt
int readExitSensor();  // -1 = Pin nicht eingestellt
int readRcSensor(int pin); // beliebiger Pololu-RC-Sensor (Zeit in us, klein = hell)
int sorterWheelPin();
int sorterExitPin();
