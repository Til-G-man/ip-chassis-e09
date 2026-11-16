#pragma once
#include <Arduino.h>

// =====================================================================
//  ABLAUF-SPRACHE  –  das, was der Baukasten im Browser erzeugt.
//  Man kann sie auch direkt als Text schreiben:
//
//    let runde = 0
//    repeat 3
//      follow until checkpoints >= 1
//      servo 1 90
//      let runde = runde + 1
//    end
//    if gpio(0) == 0
//      print Taster gedrückt in Runde {runde}
//    else
//      turn 1 until lineCenter and time > 300
//    end
//    while battery > 11.5 and runde < 10
//      ...
//    end
//
//  Bewegungen mit "until" laufen, bis die Bedingung wahr ist:
//    follow until <bedingung>          Linie folgen
//    drive <links> <rechts> until ...  fahren
//    turn <1|-1> until ...             auf der Stelle drehen
//    motor <n> <speed> until ...       einen Motor drehen
//    wait until ...                    nur warten
//
//  In Bedingungen/Zahlen erlaubt:
//    + - * / ( )  < > <= >= == !=  and or not
//    time          ms seit Start des "until"-Blocks (sonst seit Skriptstart)
//    checkpoints   Querlinien seit Start des "until"-Blocks
//    lineCenter    1, wenn der Mittelsensor die Linie sieht
//    linePos       0..4000 (2000 = Mitte)
//    qtr(i)        Liniensensor i (0..4), 0..1000
//    gpio(n)       MCP-Eingang B0..B6 (mit Pull-up)
//    pin(n)        ESP32-Pin digital (mit Pull-up), analog(n) analog 0..4095
//    enc(n)        Encoder von Motor n,   current(n) Motorstrom in A
//    battery       Batteriespannung
//    abs(x) min(a,b) max(a,b) random(a,b)
//    Variablen (let), Parameter (z.B. speed) und arg1..arg8 (Argumente
//    beim Aufruf eines Skripts als Funktion: "zurStation 2" -> arg1 = 2)
//
//  Jedes gespeicherte Skript ist automatisch eine Funktion und kann in
//  anderen Skripten mit seinem Namen aufgerufen werden.
// =====================================================================

// Wertet einen Ausdruck aus. ok = false bei Fehler (z.B. unbekannter Name).
float evalExpression(const char *text, bool &ok, String *error = nullptr);

// Führt einen Ablauf (mehrere Zeilen) aus. false = STOP oder Fehler.
bool runProgram(const char *text);

// Ruft ein gespeichertes Skript wie eine Funktion auf (setzt arg1..arg8).
bool callScript(const char *name, const float *args, int argCount);

void setVariable(const char *name, float value);
bool getVariable(const char *name, float &value);
void clearVariables();
void variablesToJson(String &out);

void resetProgramClock(); // "time" ausserhalb von until-Blöcken
int programDepth();       // 0 = kein Skript läuft gerade
