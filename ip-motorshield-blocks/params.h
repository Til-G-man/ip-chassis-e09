#pragma once
#include <Arduino.h>

// =====================================================================
//  PARAMETER  –  Zahlen, die man im Browser ändern kann, ohne neu zu
//  kompilieren. Werte werden auf Wunsch im Flash gespeichert und sind
//  nach einem Neustart wieder da.
//
//  Benutzung (z.B. in my-tasks.cpp):
//      int shootSpeed = 2000;                       // Standardwert
//      addParam("shootSpeed", &shootSpeed, 0, 4095, "PWM der Schussräder");
//
//  Danach erscheint "shootSpeed" automatisch auf der Werkbank-Seite
//  (http://192.168.4.1:8080) und kann mit  set shootSpeed 2500  geändert
//  werden. Im Code einfach die Variable shootSpeed benutzen.
//
//  Namen: max. 15 Zeichen, keine Leerzeichen.
// =====================================================================

void addParam(const char *name, float *value, float minValue, float maxValue, const char *help);
void addParam(const char *name, int *value, int minValue, int maxValue, const char *help);
void addParam(const char *name, bool *value, const char *help);

bool setParam(const char *name, float value); // false, wenn es den Namen nicht gibt
bool getParam(const char *name, float &value);

void loadParams();  // gespeicherte Werte aus dem Flash laden (einmal in setup())
void saveParams();  // aktuelle Werte im Flash speichern
void resetParams(); // gespeicherte Werte löschen -> Standardwerte aus dem Code

void printParams(Print &out);
void paramsToJson(String &out);
