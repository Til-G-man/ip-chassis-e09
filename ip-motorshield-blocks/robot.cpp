#include "robot.h"

#include "commands.h"
#include "config.h"
#include "params.h"
#include "pinout.h"
#include "shared-globals.h"
#include "wifi-debug.h"

Motorshield shield;
QTRSensors qtr;
PIDController linePid(pidControlKp, pidControlKi, pidControlKd, (float)(QTR_LINE_MAX_VALUE / 2));

int leftMotorNumber = 1;
int rightMotorNumber = 2;
bool invertLeft = false;
bool invertRight = false;
int turnSpeed = 1024;
int calibrationSpeed = 1024;
int lineThreshold = BLACKLINE_THRESHOLD;
bool lineCalibrated = false;

void registerRobotParams()
{
    // Linienfolger
    addParam("kp", &pidControlKp, 0, 100, "PID: P-Anteil (Linienfolger)");
    addParam("ki", &pidControlKi, 0, 100, "PID: I-Anteil");
    addParam("kd", &pidControlKd, 0, 100, "PID: D-Anteil");
    addParam("speed", &defaultMotorSpeed, 0, 4095, "Grundgeschwindigkeit beim Linienfolgen (PWM)");
    addParam("loopHz", &driveLoopHzTarget, 1, 500, "Regelschleife pro Sekunde");
    addParam("lineThreshold", &lineThreshold, 0, 1000, "ab diesem Sensorwert = schwarz");
    addParam("deadzone", &enableMotorDeadzoneCompensation, "Totzone der Motoren ausgleichen");
    addParam("minPwm", &minPWMtoMoveMotorBig, 0, 4095, "kleinster PWM, bei dem der Motor dreht");

    // Fahrwerk
    addParam("leftMotor", &leftMotorNumber, 1, 6, "Motor-Anschluss des linken Rads");
    addParam("rightMotor", &rightMotorNumber, 1, 6, "Motor-Anschluss des rechten Rads");
    addParam("invertLeft", &invertLeft, "linkes Rad dreht falsch herum");
    addParam("invertRight", &invertRight, "rechtes Rad dreht falsch herum");
    addParam("turnSpeed", &turnSpeed, 0, 4095, "PWM beim Drehen auf der Stelle");
    addParam("calibSpeed", &calibrationSpeed, 0, 4095, "PWM beim Kalibrieren");
    addParam("brakeOnStop", &brakeWhenStopped, "beim Anhalten bremsen statt ausrollen");
    addParam("monitoring", &enableSupplyAndM1M2CurrentMonitoring, "Batterie/Strom für Telemetrie messen (bremst etwas)");
}

// ---------------------------------------------------------------------
//  Motoren
// ---------------------------------------------------------------------

static Motor &leftMotor() { return shield.motor(constrain(leftMotorNumber, 1, 6)); }
static Motor &rightMotor() { return shield.motor(constrain(rightMotorNumber, 1, 6)); }

void stopAllMotors()
{
    for (uint8_t m = 1; m <= 6; ++m)
    {
        shield.motor(m).stop();
    }
    leftMotorSpeed = 0;
    rightMotorSpeed = 0;
}

void drive(int left, int right)
{
    if (wifiDebugMotionLocked())
    {
        return; // Upload läuft: Motoren bleiben aus
    }
    leftMotorSpeed = constrain(left, -PWM_MAX_VALUE, PWM_MAX_VALUE);
    rightMotorSpeed = constrain(right, -PWM_MAX_VALUE, PWM_MAX_VALUE);
    leftMotor().setSpeed(invertLeft ? -leftMotorSpeed : leftMotorSpeed);
    rightMotor().setSpeed(invertRight ? -rightMotorSpeed : rightMotorSpeed);
}

void stopDrive()
{
    if (brakeWhenStopped)
    {
        leftMotor().brake();
        rightMotor().brake();
    }
    else
    {
        leftMotor().stop();
        rightMotor().stop();
    }
    leftMotorSpeed = 0;
    rightMotorSpeed = 0;
}

bool driveFor(int left, int right, uint32_t ms)
{
    drive(left, right);
    const bool ok = waitMs(ms);
    stopDrive();
    return ok;
}

// Überwindet die Haftreibung bei kleinen PWM-Werten (Totzone).
static int compensateDeadzone(int speedCommand)
{
    if (!enableMotorDeadzoneCompensation || speedCommand == 0)
    {
        return speedCommand;
    }
    const int sign = speedCommand > 0 ? 1 : -1;
    const int magnitude = abs(speedCommand);
    if (magnitude >= minPWMtoMoveMotorBig)
    {
        return sign * min(magnitude, PWM_MAX_VALUE);
    }
    // Zu langsam zum Anfahren: Richtung umdrehen und über die Totzone heben.
    return -sign * min(2 * minPWMtoMoveMotorBig - magnitude, PWM_MAX_VALUE);
}

// ---------------------------------------------------------------------
//  Liniensensor
// ---------------------------------------------------------------------

uint16_t readLine()
{
    const uint16_t position = qtr.readLineBlack(qtrSensorValues);
    updateLineStatus();
    return position;
}

bool centerOnLine()
{
    return qtrSensorValues[QTR_SENSOR_COUNT / 2] > lineThreshold;
}

void printLineSensors(Print &out)
{
    const uint16_t position = lineCalibrated ? readLine() : 0;
    if (!lineCalibrated)
    {
        qtr.read(qtrSensorValues); // Rohwerte (Zeit in us), noch nicht kalibriert
    }
    out.print(lineCalibrated ? "Linie (0-1000): " : "Roh (us): ");
    for (uint8_t i = 0; i < QTR_SENSOR_COUNT; ++i)
    {
        out.printf("%5u ", qtrSensorValues[i]);
    }
    if (lineCalibrated)
    {
        static const char *statusNames[] = {"NORMAL", "STOP", "STATION", "ROTATION"};
        out.printf(" Position %4u  %s", position, statusNames[lineStatus]);
    }
    out.println();
}

bool calibrateLine()
{
    qtr.resetCalibration();
    const int turns = 4;
    const int samplesPerTurn = 40;
    const int halfTurn = samplesPerTurn / 2;
    const int totalSamples = turns * samplesPerTurn + halfTurn;
    bool nextLeft = true;

    for (int i = 0; i < totalSamples; ++i)
    {
        if (i == 0 || (i - halfTurn) % samplesPerTurn == 0)
        {
            stopDrive();
            if (!waitMs(100))
            {
                stopDrive();
                return false;
            }
            drive(nextLeft ? -calibrationSpeed : calibrationSpeed, nextLeft ? calibrationSpeed : -calibrationSpeed);
            nextLeft = !nextLeft;
        }
        qtr.calibrate();
        serviceBackground();
        if (stopRequested())
        {
            stopDrive();
            return false;
        }
    }

    // Zurück zur Mitte drehen, bis der Mittelsensor die Linie sieht.
    drive(nextLeft ? -calibrationSpeed : calibrationSpeed, nextLeft ? calibrationSpeed : -calibrationSpeed);
    lineCalibrated = true;
    const uint32_t start = millis();
    readLine();
    while (!centerOnLine() && millis() - start < 1000 && !stopRequested())
    {
        serviceBackground();
        delay(5);
        readLine();
    }
    stopDrive();

    if (!centerOnLine())
    {
        Debug.println("Kalibrierung: Linie danach nicht wiedergefunden. lineThreshold kleiner machen oder Verkabelung prüfen.");
    }
    Debug.print("Kalibrierung min: ");
    for (uint8_t i = 0; i < QTR_SENSOR_COUNT; ++i)
    {
        Debug.printf("%u ", qtr.calibrationOn.minimum[i]);
    }
    Debug.print(" max: ");
    for (uint8_t i = 0; i < QTR_SENSOR_COUNT; ++i)
    {
        Debug.printf("%u ", qtr.calibrationOn.maximum[i]);
    }
    Debug.println();
    return !stopRequested();
}

void resetFollow()
{
    linePid.reset();
}

void followLineStep(uint16_t position)
{
    // Live-Änderungen aus dem Browser übernehmen
    linePid.kp = pidControlKp;
    linePid.ki = pidControlKi;
    linePid.kd = pidControlKd;
    controlSignal = calculatePIDstep(linePid, position, driveLoopHzTarget);
    const int signal = static_cast<int>(controlSignal);
    drive(compensateDeadzone(defaultMotorSpeed + signal), compensateDeadzone(defaultMotorSpeed - signal));
}

bool followLine(int checkpoints, uint32_t timeoutMs)
{
    if (!lineCalibrated)
    {
        Debug.println("Erst 'calibrate' ausführen!");
        return false;
    }
    linePid.reset();
    int seen = 0;
    LineStatus previous = lineStatus;
    const uint32_t start = millis();
    uint32_t lastStepUs = micros();
    uint32_t loopsInWindow = 0;
    uint32_t windowStartMs = millis();

    while (true)
    {
        serviceBackground();
        if (stopRequested())
        {
            stopDrive();
            return false;
        }
        if (timeoutMs > 0 && millis() - start > timeoutMs)
        {
            stopDrive();
            Debug.println("follow: Timeout.");
            return false;
        }

        const uint32_t periodUs = 1000000UL / max(driveLoopHzTarget, 1);
        if (micros() - lastStepUs < periodUs)
        {
            delayMicroseconds(200);
            continue;
        }
        lastStepUs = micros();

        followLineStep(readLine());
        driveLoopExecUsMeasured = static_cast<float>(micros() - lastStepUs);

        if (lineStatus == STOP && previous != STOP)
        {
            ++checkpointCounter;
            lastCheckpointTimeMs = millis();
            ++seen;
            Debug.printf("Checkpoint %d (gesamt %d)\n", seen, checkpointCounter);
            if (checkpoints > 0 && seen >= checkpoints)
            {
                stopDrive();
                return true;
            }
        }
        previous = lineStatus;

        ++loopsInWindow;
        if (millis() - windowStartMs >= 1000)
        {
            driveLoopHzMeasured = 1000.0f * loopsInWindow / (millis() - windowStartMs);
            loopsInWindow = 0;
            windowStartMs = millis();
        }
    }
}

bool turnUntilLine(int direction, int lines, uint32_t timeoutMs)
{
    if (!lineCalibrated)
    {
        Debug.println("Erst 'calibrate' ausführen!");
        return false;
    }
    const int dir = direction >= 0 ? 1 : -1;
    const uint32_t start = millis();
    drive(dir * turnSpeed, -dir * turnSpeed);

    for (int i = 0; i < lines; ++i)
    {
        // erst von der aktuellen Linie herunter, dann bis zur nächsten drehen
        for (int phase = 0; phase < 2; ++phase)
        {
            const bool wantOnLine = phase == 1;
            readLine();
            while (centerOnLine() != wantOnLine)
            {
                serviceBackground();
                if (stopRequested() || millis() - start > timeoutMs)
                {
                    stopDrive();
                    if (!stopRequested())
                    {
                        Debug.println("turn: Timeout, keine Linie gefunden.");
                    }
                    return false;
                }
                delay(2);
                readLine();
            }
        }
    }
    stopDrive();
    return true;
}
