#include "devices.h"

#include <Preferences.h>

#include "commands.h"
#include "params.h"
#include "robot.h"
#include "shared-globals.h"
#include "sorter.h"

namespace
{
// Freie ESP32-Pins laut Pinout_Motorshield_ESP32_IP26_V4 (ohne Motor-, Encoder-,
// QTR-, I2C- und LED-Pins). 10-13 sind nur frei, solange kein SPI benutzt wird.
const int kPins[] = {6, 14, 48, 10, 11, 12, 13, 39, 40};
constexpr int kPinCount = sizeof(kPins) / sizeof(kPins[0]);

struct Dev
{
    char name[16];
    uint8_t mode;
    int8_t out; // letzter geschriebener Ausgangswert, -1 = unbekannt
};

Dev motors[6], servos[8], mcp[7], pins[kPinCount];
portMUX_TYPE devMutex = portMUX_INITIALIZER_UNLOCKED;

bool hasAdc(int gpio) { return gpio >= 1 && gpio <= 10; } // ADC1; ADC2 geht nicht mit WLAN

int pinIndex(int gpio)
{
    for (int i = 0; i < kPinCount; ++i)
        if (kPins[i] == gpio)
            return i;
    return -1;
}

Dev *find(DeviceKind kind, int n)
{
    switch (kind)
    {
    case DevMotor:
        return (n >= 1 && n <= 6) ? &motors[n - 1] : nullptr;
    case DevServo:
        return (n >= 1 && n <= 8) ? &servos[n - 1] : nullptr;
    case DevMcp:
        return (n >= 0 && n <= 6) ? &mcp[n] : nullptr;
    case DevPin:
    {
        const int i = pinIndex(n);
        return i >= 0 ? &pins[i] : nullptr;
    }
    }
    return nullptr;
}

String keyOf(DeviceKind kind, int n)
{
    const char *k[] = {"m", "s", "b", "g"};
    return String(k[kind]) + n;
}

bool isSorterPin(int gpio) { return gpio >= 0 && (gpio == sorterWheelPin() || gpio == sorterExitPin()); }

int effectiveMode(DeviceKind kind, int n, const Dev &d)
{
    if (kind == DevPin && isSorterPin(n))
        return ModeRc;
    return d.mode;
}

float readInput(DeviceKind kind, int n, int mode, int8_t out)
{
    if (mode == ModeOut)
        return out < 0 ? 0 : out;
    if (kind == DevMcp)
    {
        const auto p = static_cast<Mcp23017::Pin>(n);
        shield.gpio().pinMode(p, INPUT_PULLUP);
        return shield.gpio().digitalRead(p);
    }
    switch (mode)
    {
    case ModeAnalog:
        return hasAdc(n) ? analogRead(n) : 0;
    case ModeRc:
        return readRcSensor(n);
    default:
        pinMode(n, INPUT_PULLUP);
        return digitalRead(n);
    }
}

const char *kReserved[] = {"true", "false", "time", "millis", "checkpoints", "lineCenter", "linePos",
                           "battery", "cube", "wheelSensor", "exitSensor", "qtr", "gpio", "pin", "analog",
                           "enc", "current", "abs", "min", "max", "random", "and", "or", "not", "repeat",
                           "while", "if", "else", "end", "let", "print", "until", "arg1", "arg2", "arg3",
                           "arg4", "arg5", "arg6", "arg7", "arg8"};

void appendEscaped(String &out, const char *s)
{
    out += '"';
    for (; *s; ++s)
    {
        if (*s == '"' || *s == '\\')
            out += '\\';
        out += *s;
    }
    out += '"';
}

void appendDev(String &out, DeviceKind kind, int n, const Dev &d, bool live, const char *tag)
{
    out += "{\"n\":";
    out += n;
    out += ",\"name\":";
    appendEscaped(out, d.name);
    const int mode = effectiveMode(kind, n, d);
    out += ",\"mode\":";
    out += mode;
    if (live)
    {
        out += ",\"v\":";
        out += String(readInput(kind, n, mode, d.out), 0);
    }
    if (kind == DevPin)
        out += hasAdc(n) ? ",\"adc\":true" : ",\"adc\":false";
    if (tag && *tag)
    {
        out += ",\"tag\":";
        appendEscaped(out, tag);
    }
    out += '}';
}

int paramInt(const char *name, int fallback)
{
    float v = 0;
    return getParam(name, v) ? static_cast<int>(v) : fallback;
}
} // namespace

void loadDevices()
{
    Preferences prefs;
    prefs.begin("devices", true);
    auto load = [&](DeviceKind kind, int n)
    {
        Dev *d = find(kind, n);
        d->out = -1;
        d->mode = (kind == DevPin || kind == DevMcp) ? ModeIn : ModeOut;
        const String v = prefs.getString(keyOf(kind, n).c_str(), "");
        const int bar = v.indexOf('|');
        if (bar < 0)
            return;
        strlcpy(d->name, v.substring(0, bar).c_str(), sizeof(d->name));
        d->mode = v.substring(bar + 1).toInt();
    };
    for (int n = 1; n <= 6; ++n)
        load(DevMotor, n);
    for (int n = 1; n <= 8; ++n)
        load(DevServo, n);
    for (int n = 0; n <= 6; ++n)
        load(DevMcp, n);
    for (int i = 0; i < kPinCount; ++i)
        load(DevPin, kPins[i]);
    prefs.end();
}

bool setDevice(DeviceKind kind, int number, const String &rawName, int mode, String &error)
{
    Dev *d = find(kind, number);
    if (!d)
    {
        error = "unbekannter Anschluss";
        return false;
    }
    String name = rawName;
    name.trim();
    if (name.length())
    {
        if (name.length() > 15 || !isalpha(static_cast<unsigned char>(name[0])))
        {
            error = "Name: mit Buchstabe beginnen, max. 15 Zeichen";
            return false;
        }
        for (size_t i = 0; i < name.length(); ++i)
        {
            if (!isalnum(static_cast<unsigned char>(name[i])) && name[i] != '_')
            {
                error = "Name: nur Buchstaben, Zahlen und _ (keine Umlaute/Leerzeichen)";
                return false;
            }
        }
        for (const char *r : kReserved)
        {
            if (name.equalsIgnoreCase(r))
            {
                error = "'" + name + "' ist schon ein eingebautes Wort";
                return false;
            }
        }
        float dummy;
        String text;
        if (commandExists(name.c_str()) || getParam(name.c_str(), dummy) || loadScript(name.c_str(), text))
        {
            error = "'" + name + "' gibt es schon als Befehl, Parameter oder Skript";
            return false;
        }
        Dev *all[] = {motors, servos, mcp, pins};
        const int counts[] = {6, 8, 7, kPinCount};
        for (int k = 0; k < 4; ++k)
            for (int i = 0; i < counts[k]; ++i)
                if (&all[k][i] != d && name.equalsIgnoreCase(all[k][i].name))
                {
                    error = "'" + name + "' ist schon vergeben";
                    return false;
                }
    }
    if (kind == DevMotor || kind == DevServo)
        mode = ModeOut;
    if (kind == DevMcp && mode != ModeOut)
        mode = ModeIn;
    if (kind == DevPin && mode == ModeAnalog && !hasAdc(number))
    {
        error = "GPIO " + String(number) + " kann nicht analog messen (nur GPIO 6 und 10)";
        return false;
    }
    mode = constrain(mode, 0, 3);

    portENTER_CRITICAL(&devMutex);
    strlcpy(d->name, name.c_str(), sizeof(d->name));
    d->mode = mode;
    portEXIT_CRITICAL(&devMutex);

    Preferences prefs;
    prefs.begin("devices", false);
    const String key = keyOf(kind, number);
    const bool defaultMode = mode == ((kind == DevPin || kind == DevMcp) ? ModeIn : ModeOut);
    if (name.length() == 0 && defaultMode)
        prefs.remove(key.c_str());
    else
        prefs.putString(key.c_str(), name + "|" + mode);
    prefs.end();
    return true;
}

bool lookupDevice(const char *name, float &value)
{
    DeviceKind kind = DevMotor;
    int n = -1, mode = 0;
    int8_t out = -1;
    portENTER_CRITICAL(&devMutex);
    auto check = [&](DeviceKind k, Dev *arr, int count, int first)
    {
        for (int i = 0; i < count && n < 0; ++i)
            if (arr[i].name[0] && strcasecmp(arr[i].name, name) == 0)
            {
                kind = k;
                n = (k == DevPin) ? kPins[i] : first + i;
                mode = arr[i].mode;
                out = arr[i].out;
            }
    };
    check(DevMotor, motors, 6, 1);
    check(DevServo, servos, 8, 1);
    check(DevMcp, mcp, 7, 0);
    check(DevPin, pins, kPinCount, 0);
    portEXIT_CRITICAL(&devMutex);
    if (n < 0)
        return false;

    if (kind == DevMotor || kind == DevServo || mode == ModeOut)
        value = n; // Aktoren: Name steht für die Anschluss-Nummer
    else
        value = readInput(kind, n, (kind == DevPin && isSorterPin(n)) ? ModeRc : mode, out);
    return true;
}

void noteDeviceOutput(DeviceKind kind, int number, int value)
{
    Dev *d = find(kind, number);
    if (!d)
        return;
    d->out = value ? 1 : 0;
    d->mode = ModeOut; // ab jetzt nicht mehr als Eingang lesen (sonst wäre der Ausgang weg)
}

void hardwareToJson(String &out, bool withCurrents)
{
    out.reserve(3000);
    const bool busy = commandRunning();
    out += "{\"running\":";
    appendEscaped(out, currentCommandLine().c_str());
    out += ",\"battery\":";
    out += String(shield.batteryVoltage(), 2);

    // Liniensensor: während ein Programm läuft, nur die zuletzt gemessenen Werte zeigen
    uint16_t line[QTR_SENSOR_COUNT];
    if (busy)
        memcpy(line, qtrSensorValues, sizeof(line));
    else if (lineCalibrated)
        qtr.readCalibrated(line);
    else
        qtr.read(line);
    out += ",\"line\":{\"cal\":";
    out += lineCalibrated ? "true" : "false";
    out += ",\"v\":[";
    for (size_t i = 0; i < QTR_SENSOR_COUNT; ++i)
    {
        if (i)
            out += ',';
        out += line[i];
    }
    out += "]}";

    const int wheelMotor = paramInt("wheelMotor", -1);
    const int left = paramInt("leftMotor", 1), right = paramInt("rightMotor", 2);
    const int grip = paramInt("gripServo", -1), tool = paramInt("toolMotor", -1);
    out += ",\"sorter\":{\"wheelPin\":";
    out += sorterWheelPin();
    out += ",\"exitPin\":";
    out += sorterExitPin();
    out += ",\"wheelMotor\":";
    out += wheelMotor;
    out += '}';

    out += ",\"motors\":[";
    for (int n = 1; n <= 6; ++n)
    {
        if (n > 1)
            out += ',';
        const char *tag = n == left ? "Antrieb links" : n == right ? "Antrieb rechts" : n == wheelMotor ? "Sortierrad" : n == tool ? "tool-Befehl" : "";
        out += "{\"n\":";
        out += n;
        out += ",\"name\":";
        appendEscaped(out, motors[n - 1].name);
        if (*tag)
        {
            out += ",\"tag\":";
            appendEscaped(out, tag);
        }
        if (n <= 2)
        {
            out += ",\"enc\":";
            out += shield.motor(n).position();
        }
        if (withCurrents)
        {
            const float a = shield.motor(n).readCurrentAmps();
            out += ",\"a\":";
            out += isnan(a) ? String("null") : String(a, 2);
        }
        out += '}';
    }
    out += "],\"servos\":[";
    for (int n = 1; n <= 8; ++n)
    {
        if (n > 1)
            out += ',';
        appendDev(out, DevServo, n, servos[n - 1], false, n == grip ? "grip-Befehl" : "");
    }
    out += "],\"mcp\":[";
    for (int n = 0; n <= 6; ++n)
    {
        if (n)
            out += ',';
        appendDev(out, DevMcp, n, mcp[n], true, n == 0 ? "waitbutton-Befehl" : "");
    }
    out += "],\"pins\":[";
    for (int i = 0; i < kPinCount; ++i)
    {
        if (i)
            out += ',';
        const int g = kPins[i];
        const char *tag = g == sorterWheelPin() ? "Sortierrad-Sensor" : g == sorterExitPin() ? "Austritts-Sensor" : (g == 39 || g == 40) ? "Pull-up, nicht für RC" : (g >= 10 && g <= 13) ? "SPI-Pin" : "";
        // während ein Programm läuft, RC-Sensoren nicht stören (Messung dauert bis 2,5 ms)
        appendDev(out, DevPin, g, pins[i], !(busy && effectiveMode(DevPin, g, pins[i]) == ModeRc), tag);
    }
    out += "]}";
}

void deviceNamesToJson(String &out)
{
    out += '[';
    bool first = true;
    auto add = [&](const char *k, int n, const Dev &d, int mode)
    {
        if (!d.name[0])
            return;
        if (!first)
            out += ',';
        first = false;
        out += "{\"k\":\"";
        out += k;
        out += "\",\"n\":";
        out += n;
        out += ",\"name\":";
        appendEscaped(out, d.name);
        out += ",\"mode\":";
        out += mode;
        out += '}';
    };
    for (int n = 1; n <= 6; ++n)
        add("motor", n, motors[n - 1], ModeOut);
    for (int n = 1; n <= 8; ++n)
        add("servo", n, servos[n - 1], ModeOut);
    for (int n = 0; n <= 6; ++n)
        add("gpio", n, mcp[n], mcp[n].mode);
    for (int i = 0; i < kPinCount; ++i)
        add("pin", kPins[i], pins[i], effectiveMode(DevPin, kPins[i], pins[i]));
    out += ']';
}
