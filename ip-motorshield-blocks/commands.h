#pragma once
#include <Arduino.h>

// =====================================================================
//  BEFEHLE  –  jede Fähigkeit des Roboters ist ein Befehl mit Namen und
//  Zahlen-Argumenten, z.B.  drive 1000 1000 500   oder   servo 1 90
//
//  Befehle kann man
//    * im Browser eintippen (Werkbank, http://192.168.4.1:8080),
//    * im Seriellen Monitor eintippen (115200 Baud, mit Kabel),
//    * zu einem SKRIPT (Mission) zusammensetzen, das im Roboter
//      gespeichert wird – ohne neu zu kompilieren.
//
//  Eigenen Befehl anlegen (z.B. in my-tasks.cpp):
//
//      void cmdShoot(const Args &args) {
//          int balls = args.get(0, 1);     // 1. Argument, Standard 1
//          ...
//      }
//      addCommand("shoot", "shoot [anzahl] - schiesst Bälle", cmdShoot);
//
//  Als Argument darf auch ein Parametername stehen:
//      drive speed speed 1000   -> nimmt den aktuellen Wert von "speed".
//
//  Lange Befehle MÜSSEN waitMs() statt delay() benutzen (oder regelmässig
//  stopRequested() prüfen), damit der STOP-Knopf und das kabellose
//  Hochladen funktionieren.
// =====================================================================

struct Args
{
    static constexpr int kMax = 8;
    int count = 0;          // Anzahl Argumente nach dem Befehlsnamen
    float value[kMax] = {}; // Zahlenwert jedes Arguments (NAN wenn keine Zahl)
    String word[kMax];      // jedes Argument als Text
    String rest;            // alles nach dem Befehlsnamen als Text

    bool has(int i) const { return i < count && !isnan(value[i]); }
    float get(int i, float fallback) const { return has(i) ? value[i] : fallback; }
    int getInt(int i, int fallback) const { return has(i) ? static_cast<int>(lroundf(value[i])) : fallback; }
};

using CommandFunction = void (*)(const Args &args);

// true: jeden Befehl im Log anzeigen, auch innerhalb von Skripten
extern bool traceCommands;

void addCommand(const char *name, const char *usage, CommandFunction function);
bool commandExists(const char *name);

// Führt eine Befehlszeile sofort aus. Nur aus dem Hauptprogramm (loop) aufrufen.
bool runCommand(const char *line);

// Führt mehrere Befehle nacheinander aus (getrennt durch Zeilenumbruch oder ';',
// '#' leitet einen Kommentar ein). Bricht ab, sobald STOP gedrückt wurde.
bool runScriptText(const char *text);
bool runScript(const char *name);

// Skripte (Missionen): im Code vorgeben, im Browser ändern und speichern.
void addDefaultScript(const char *name, const char *text);
bool loadScript(const char *name, String &text);
bool storeScript(const char *name, const char *text); // leerer Text = löschen

// --- Zusammenspiel mit Browser / Seriell / OTA --------------------------
void queueCommand(const char *line); // aus jedem Task erlaubt
void processCommandQueue();          // in loop() aufrufen
void requestStop();                  // STOP: bricht laufende Befehle ab
bool stopRequested();
bool commandRunning();
String currentCommandLine();

// Wartet ms Millisekunden, hält dabei Browser/OTA am Leben.
// Gibt false zurück, wenn STOP gedrückt wurde (dann sofort aufhören!).
bool waitMs(uint32_t ms);

// Wird während waitMs() und in Regelschleifen aufgerufen (OTA, Seriell).
void setBackgroundHook(void (*hook)());
void serviceBackground();

void printCommands(Print &out);
void commandsToJson(String &out);
void scriptsToJson(String &out);
