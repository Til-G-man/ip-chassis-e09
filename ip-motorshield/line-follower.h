#pragma once
#include <Arduino.h>

enum LineStatus
{
  NORMAL,   // Linie ist in der Mitte (oder leicht links/rechts)
  STOP,     // alle Sensoren sehen schwarz, z.B. eine Querlinie (Checkpoint)
  STATION,  // äussere Sensoren sehen die Linie, innere nicht
  ROTATION  // alles andere (z.B. Linie verloren / beim Drehen)
};

class PIDController
{
public:
    float kp;
    float ki;
    float kd;
    float target;
    float integral;
    float prevError;

    PIDController(float kp, float ki, float kd, float target)
        : kp(kp), ki(ki), kd(kd), target(target), integral(0.0f), prevError(0.0f) {}

    void reset()
    {
        integral = 0.0f;
        prevError = 0.0f;
    }
};

// Ein Schritt des PID-Reglers. measurement = Linienposition (0..4000),
// executionFrequency = wie oft pro Sekunde aufgerufen wird.
float calculatePIDstep(PIDController &pid, float measurement, float executionFrequency);

// Wertet qtrSensorValues aus und setzt qtrSensorStatus[] und lineStatus.
void updateLineStatus();
