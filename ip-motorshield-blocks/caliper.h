#pragma once
#include <Arduino.h>

// =====================================================================
//  MESSSCHIEBER (digitaler Taster) ueber Takt+Daten-Leitung
//
//  Protokoll (24 Bit pro Paket, LSB zuerst je Byte):
//    Byte0          = unterste 8 Bit des Betrags
//    Byte1          = naechste 8 Bit
//    Byte2 Bit0-4   = oberste 5 Bit des Betrags (20 Bit Betrag gesamt)
//    Byte2 Bit5     = Vorzeichen (1 = negativ)
//  mm = Betrag / 200.0   (1 Count = 0.005 mm)
//
//  Hardware: Transistor-Pegelwandler zwischen Messschieber (~1.2-1.5V)
//  und ESP32 (3.3V) ist bereits aufgebaut, invertiert das Signal.
//  Pull-up-Seite der Schaltung MUSS an 3V3 haengen, nicht 5V!
// =====================================================================

void registerCaliper();
void initCaliper();      // Interrupts anmelden - im setup() nach loadParams() aufrufen
void serviceCaliper();   // Paket-Timeout pruefen - regelmaessig aufrufen (z.B. aus background())
float caliperMm();       // letzter gueltiger Messwert in mm (NAN, falls noch kein Paket da)
