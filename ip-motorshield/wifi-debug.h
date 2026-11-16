#pragma once
#include <Arduino.h>
#include "config.h"

class Motorshield;

// =====================================================================
//  WERKBANK  –  kabelloses Hochladen (OTA) + Browser-Seite auf Port 8080
//  mit Log, Befehlskonsole, Parametern und Skripten.
//
//  Debug.print / println / printf  schreibt in den Seriellen Monitor UND
//  in das Log im Browser. Immer Debug statt Serial benutzen.
// =====================================================================

#if ENABLE_WIFI_DEBUG
class DebugOutput : public Print {
public:
    using Print::write;
    size_t write(uint8_t value) override;
    size_t write(const uint8_t *data, size_t length) override;
};

extern DebugOutput Debug;

void wifiDebugBegin(Motorshield &shield); // nach initWebInterface() aufrufen
void wifiDebugWebLoop();                  // im Web-Task (Kern 0)
void wifiDebugOtaLoop();                  // im Hauptprogramm (Kern 1)
bool wifiDebugMotionLocked();             // true, sobald ein Upload begonnen hat
#else
#define Debug Serial
inline void wifiDebugBegin(Motorshield &) {}
inline void wifiDebugWebLoop() {}
inline void wifiDebugOtaLoop() {}
inline bool wifiDebugMotionLocked() { return false; }
#endif
