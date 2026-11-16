#include "sorter.h"

#include "commands.h"
#include "params.h"
#include "robot.h"
#include "wifi-debug.h"

namespace
{
// ---- Parameter (im Browser änderbar) ---------------------------------
int wheelMotor = 3;       // Motor-Anschluss des Rad-Motors (TT-Motor)
int wheelSpeed = 1500;    // PWM, Vorzeichen = Drehrichtung
int wheelEncPin = 14;     // ESP32-Pin des Rad-Positionssensors (-1 = nicht angeschlossen)
int exitPin = 48;         // ESP32-Pin des Austritts-Sensors (-1 = nicht angeschlossen)
int wheelThresh = 500;    // Schwelle Rad-Sensor in us (wird von wheelcal gesetzt)
int wheelHyst = 60;       // Hysterese gegen Flattern an der Kante
int exitThresh = 400;     // Würfel erkannt, wenn Austritts-Wert UNTER dieser Schwelle
bool exitInvert = false;  // true: Würfel erkannt, wenn Wert ÜBER der Schwelle
int edgesPerPocket = 4;   // laut CAD: 40 Flanken / 10 Taschen
int wheelJamMs = 800;     // so lange keine Flanke -> Rad klemmt
int wheelBackMs = 250;    // so lange zurückdrehen zum Lösen
int wheelRetries = 3;     // so oft lösen, dann Fehler
int rcTimeoutUs = 2500;   // max. Messzeit eines RC-Sensors
bool wheelBrake = true;   // am Ziel bremsen statt ausrollen
bool wheelAutoCal = true; // beim Einschalten automatisch kalibrieren (autoCalibrateSorter)
int autoCalMs = 3000;     // wie lange beim Auto-Kalibrieren gedreht wird

Motor &wheelMotorRef()
{
    return shield.motor(constrain(wheelMotor, 1, 6));
}

void stopWheel()
{
    if (wheelBrake)
        wheelMotorRef().brake();
    else
        wheelMotorRef().stop();
}

bool checkPins(bool needExit)
{
    if (wheelEncPin < 0)
    {
        Debug.println("Rad-Sensor-Pin fehlt: set wheelEncPin <esp32-gpio>");
        return false;
    }
    if (needExit && exitPin < 0)
    {
        Debug.println("Austritts-Sensor-Pin fehlt: set exitPin <esp32-gpio>");
        return false;
    }
    return true;
}

// Dreht das Rad für `duration` ms und misst dabei hell/dunkel am Rad-Sensor.
// Setzt bei Erfolg wheelThresh/wheelHyst und gibt true zurück.
bool calibrateWheelThreshold(uint32_t duration)
{
    int lo = 100000, hi = 0;
    wheelMotorRef().setSpeed(wheelSpeed);
    const uint32_t start = millis();
    while (millis() - start < duration)
    {
        serviceBackground();
        if (stopRequested())
        {
            stopWheel();
            return false;
        }
        const int v = readWheelSensor();
        lo = min(lo, v);
        hi = max(hi, v);
        delayMicroseconds(300);
    }
    stopWheel();
    Debug.printf("Rad-Sensor: hell %d us, dunkel %d us (Unterschied %d us)\n", lo, hi, hi - lo);
    if (hi - lo < 4 * wheelHyst)
    {
        Debug.println("Zu wenig Unterschied hell/dunkel! Sensor-Abstand/Pin pruefen oder rcTimeoutUs erhoehen.");
        return false;
    }
    wheelThresh = (lo + hi) / 2;
    wheelHyst = max(20, (hi - lo) / 8);
    return true;
}

// Misst den Austritts-Sensor im Ruhezustand (kein Wuerfel davor) und setzt
// exitThresh so, dass ein Wuerfel deutlich vom Normalwert abweicht.
bool calibrateExitBaseline()
{
    if (exitPin < 0)
        return false;
    long sum = 0;
    const int samples = 40;
    for (int i = 0; i < samples; ++i)
    {
        sum += readExitSensor();
        delayMicroseconds(300);
    }
    const int baseline = static_cast<int>(sum / samples);
    exitThresh = exitInvert ? baseline + baseline / 4 : baseline - baseline / 4;
    Debug.printf("Austritt-Normalwert (kein Wuerfel): %d us -> exitThresh = %d\n", baseline, exitThresh);
    return true;
}

// ---- Befehle ----------------------------------------------------------

void cmdWheelSens(const Args &args)
{
    const uint32_t duration = args.getInt(0, 3000);
    const uint32_t start = millis();
    do
    {
        const int w = readWheelSensor();
        const int e = readExitSensor();
        Debug.printf("Rad: %5d us (%s)   Austritt: %5d us (%s)\n",
                     w, w < 0 ? "kein Pin" : (w > wheelThresh ? "dunkel" : "hell"),
                     e, e < 0 ? "kein Pin" : (cubeAtExit() ? "WÜRFEL" : "leer"));
    } while (millis() - start < duration && waitMs(150));
}

void cmdWheelCal(const Args &args)
{
    if (!checkPins(false))
        return;
    const uint32_t duration = args.getInt(0, 4000);
    if (calibrateWheelThreshold(duration))
    {
        Debug.printf("wheelThresh = %d, wheelHyst = %d gesetzt (mit 'save' dauerhaft speichern). Test: 'wheel 10' = genau 1 Umdrehung?\n",
                     wheelThresh, wheelHyst);
    }
}

void cmdAutoCal(const Args &args)
{
    autoCalibrateSorter();
}

void cmdWheel(const Args &args)
{
    rotateWheelPockets(args.getInt(0, 1), false);
}

void cmdFeed(const Args &args)
{
    const int maxPockets = args.getInt(0, 10);
    if (!checkPins(true))
        return;
    if (cubeAtExit())
    {
        Debug.println("feed: Austritt meldet schon einen Würfel (exitThresh prüfen?).");
    }
    if (rotateWheelPockets(maxPockets, true))
    {
        if (cubeAtExit())
            Debug.println("feed: Würfel ausgegeben.");
    }
}
} // namespace

// Liest einen Pololu-RC-Sensor: Kondensator laden, dann messen, wie lange
// die Leitung HIGH bleibt. Viel Licht (helle Fläche) -> kurze Zeit.
int readRcSensor(int pin)
{
    if (pin < 0)
    {
        return -1;
    }
    // Werkbank (Kern 0) und Programme (Kern 1) dürfen nicht gleichzeitig messen
    static SemaphoreHandle_t lock = xSemaphoreCreateMutex();
    xSemaphoreTake(lock, portMAX_DELAY);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delayMicroseconds(10);
    pinMode(pin, INPUT);
    const uint32_t start = micros();
    while (digitalRead(pin) == HIGH && micros() - start < static_cast<uint32_t>(rcTimeoutUs))
    {
    }
    const int us = static_cast<int>(micros() - start);
    xSemaphoreGive(lock);
    return us;
}


int readWheelSensor() { return readRcSensor(wheelEncPin); }
int readExitSensor() { return readRcSensor(exitPin); }

bool cubeAtExit()
{
    const int v = readExitSensor();
    if (v < 0)
        return false;
    return exitInvert ? v > exitThresh : v < exitThresh;
}

bool rotateWheelPockets(int pockets, bool stopOnCube)
{
    if (!checkPins(stopOnCube) || pockets <= 0)
        return false;

    const int target = pockets * max(edgesPerPocket, 1);
    int edges = 0;
    int retries = 0;
    bool dark = readWheelSensor() > wheelThresh;
    uint32_t lastEdgeMs = millis();
    Motor &motor = wheelMotorRef();
    motor.setSpeed(wheelSpeed);

    while (edges < target)
    {
        serviceBackground();
        if (stopRequested())
        {
            stopWheel();
            return false;
        }
        if (stopOnCube && cubeAtExit())
        {
            stopWheel();
            Debug.printf("Würfel erkannt nach %d Flanken.\n", edges);
            return true;
        }

        // Flanke zählen (mit Hysterese, damit eine Kante nicht doppelt zählt)
        const int v = readWheelSensor();
        if (dark && v < wheelThresh - wheelHyst)
        {
            dark = false;
            ++edges;
            lastEdgeMs = millis();
        }
        else if (!dark && v > wheelThresh + wheelHyst)
        {
            dark = true;
            ++edges;
            lastEdgeMs = millis();
        }

        // Klemmt das Rad? Kurz zurück und nochmal.
        if (millis() - lastEdgeMs > static_cast<uint32_t>(wheelJamMs))
        {
            if (++retries > wheelRetries)
            {
                stopWheel();
                Debug.printf("Rad blockiert! %d von %d Flanken. (Würfel verklemmt? wheelThresh falsch?)\n", edges, target);
                return false;
            }
            Debug.printf("Rad klemmt – Versuch %d: kurz zurück.\n", retries);
            motor.setSpeed(-wheelSpeed);
            if (!waitMs(wheelBackMs))
            {
                stopWheel();
                return false;
            }
            motor.setSpeed(wheelSpeed);
            dark = readWheelSensor() > wheelThresh; // Zustand nach dem Zurückdrehen neu lesen
            lastEdgeMs = millis();
        }
    }
    stopWheel();
    if (stopOnCube)
    {
        Debug.printf("feed: %d Taschen gedreht, kein Würfel erkannt.\n", pockets);
        return false;
    }
    return true;
}

void registerSorter()
{
    addParam("wheelMotor", &wheelMotor, 1, 6, "Sortierrad: Motor-Anschluss (TT-Motor)");
    addParam("wheelSpeed", &wheelSpeed, -4095, 4095, "Sortierrad: PWM, Vorzeichen = Richtung");
    addParam("wheelEncPin", &wheelEncPin, -1, 48, "Sortierrad: ESP32-Pin Rad-Sensor (-1 = keiner)");
    addParam("exitPin", &exitPin, -1, 48, "Sortierrad: ESP32-Pin Austritts-Sensor (-1 = keiner)");
    addParam("wheelThresh", &wheelThresh, 0, 10000, "Rad-Sensor Schwelle in us (wheelcal setzt sie)");
    addParam("wheelHyst", &wheelHyst, 0, 2000, "Rad-Sensor Hysterese in us");
    addParam("exitThresh", &exitThresh, 0, 10000, "Austritt: Würfel wenn Wert unter Schwelle (us)");
    addParam("exitInvert", &exitInvert, "Austritt: Würfel wenn Wert ÜBER Schwelle");
    addParam("edgesPerPocket", &edgesPerPocket, 1, 40, "Flanken pro Tasche (CAD: 4)");
    addParam("wheelJamMs", &wheelJamMs, 50, 10000, "ms ohne Flanke -> Rad klemmt");
    addParam("wheelBackMs", &wheelBackMs, 0, 5000, "ms zurückdrehen zum Lösen");
    addParam("wheelRetries", &wheelRetries, 0, 10, "Lösungsversuche bis Fehler");
    addParam("rcTimeoutUs", &rcTimeoutUs, 100, 10000, "max. Messzeit RC-Sensoren (us)");
    addParam("wheelBrake", &wheelBrake, "Rad am Ziel bremsen");
    addParam("wheelAutoCal", &wheelAutoCal, "Beim Einschalten automatisch kalibrieren (autocal)");
    addParam("autoCalMs", &autoCalMs, 500, 10000, "Drehdauer beim Auto-Kalibrieren (ms)");

    addCommand("wheelsens", "wheelsens [ms] - Rad- und Austritts-Sensor live anzeigen", cmdWheelSens);
    addCommand("wheelcal", "wheelcal [ms] - Rad drehen und Sensorschwelle automatisch setzen", cmdWheelCal);
    addCommand("autocal", "autocal - Rad- UND Austritts-Sensor automatisch kalibrieren (wie beim Einschalten)", cmdAutoCal);
    addCommand("wheel", "wheel [taschen] - Sortierrad n Taschen weiterdrehen", cmdWheel);
    addCommand("feed", "feed [max_taschen] - drehen, bis ein Würfel austritt", cmdFeed);
}

int sorterWheelPin() { return wheelEncPin; }
int sorterExitPin() { return exitPin; }

void autoCalibrateSorter()
{
    Debug.println("Sortierrad-Kalibrierung laeuft...");
    if (wheelEncPin < 0)
    {
        Debug.println("autocal: wheelEncPin nicht gesetzt - Rad-Sensor wird nicht kalibriert.");
    }
    else if (calibrateWheelThreshold(static_cast<uint32_t>(autoCalMs)))
    {
        Debug.printf("autocal: wheelThresh = %d, wheelHyst = %d\n", wheelThresh, wheelHyst);
    }

    if (exitPin < 0)
    {
        Debug.println("autocal: exitPin nicht gesetzt - Austritts-Sensor wird nicht kalibriert.");
    }
    else
    {
        calibrateExitBaseline();
    }
    Debug.println("Sortierrad-Kalibrierung fertig. Mit 'save' dauerhaft speichern.");
}

void maybeAutoCalibrateOnBoot()
{
    if (wheelAutoCal)
        autoCalibrateSorter();
}
