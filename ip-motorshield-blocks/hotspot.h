#pragma once
#include <Arduino.h>

// =====================================================================
//  HOTSPOT  –  der Roboter verbindet sich zusätzlich mit einem fremden
//  WLAN (z.B. Handy-Hotspot). Dann können Laptop und Roboter im selben
//  Netz sein und der Laptop hat trotzdem Internet.
//
//  Einstellen (Werkbank-Konsole oder Serieller Monitor):
//      wifi MeinHandy geheim123          Name + Passwort speichern
//      wifi "Mein Handy" geheim123       Name mit Leerzeichen in "..."
//      wifi                               Status anzeigen
//      wifi off                           Hotspot-Modus ausschalten
//  Danach  reboot.
//
//  Das eigene WLAN "IP-Motorshield-..." bleibt IMMER zusätzlich an.
//  Findet der Roboter den Hotspot beim Start nicht (10 s), läuft alles
//  wie bisher nur über das eigene WLAN.
// =====================================================================

// Beim Start aufrufen, BEVOR das eigene WLAN startet. true = verbunden.
bool connectHotspotIfConfigured();

bool hotspotConnected();
void saveHotspotCredentials(const char *ssid, const char *password);
void clearHotspotCredentials();
void printWifiStatus(Print &out);
