// Eingebaute Befehle der Werkbank. Jeder Befehl ist eine kleine Funktion,
// die die Bausteine aus robot.h benutzt. Eigene Befehle gehören nach
// my-tasks.cpp – hier nur, wenn sie für alle Roboter nützlich sind.

#include "commands.h"
#include "onboard-led.h"
#include "params.h"
#include "robot.h"
#include "shared-globals.h"
#include "wifi-debug.h"

#include <Wire.h>

namespace
{
bool checkMotorNumber(int n)
{
    if (n < 1 || n > 6)
    {
        Debug.println("Motornummer 1..6");
        return false;
    }
    return true;
}

bool checkServoNumber(int n)
{
    if (n < 1 || n > 8)
    {
        Debug.println("Servonummer 1..8");
        return false;
    }
    return true;
}

// ---- Allgemein ------------------------------------------------------

void cmdHelp(const Args &)
{
    Debug.println("Befehle ([..] = optional):");
    printCommands(Debug);
    Debug.println("Mehrere Befehle mit ';' trennen. Statt Zahlen gehen auch Parameternamen.");
}

void cmdParams(const Args &)
{
    printParams(Debug);
}

void cmdSet(const Args &args)
{
    if (args.count < 2 || !args.has(1))
    {
        Debug.println("set <name> <wert>");
        return;
    }
    if (setParam(args.word[0].c_str(), args.value[1]))
    {
        float value = 0;
        getParam(args.word[0].c_str(), value);
        Debug.printf("%s = %g  (mit 'save' dauerhaft speichern)\n", args.word[0].c_str(), value);
    }
    else
    {
        Debug.printf("Parameter '%s' gibt es nicht ('params' zeigt alle).\n", args.word[0].c_str());
    }
}

void cmdSave(const Args &) { saveParams(); }
void cmdDefaults(const Args &) { resetParams(); }

void cmdStop(const Args &)
{
    stopAllMotors();
    Debug.println("Alle Motoren aus.");
}

void cmdWait(const Args &args)
{
    waitMs(args.getInt(0, 1000));
}

void cmdRun(const Args &args)
{
    if (args.count < 1)
    {
        Debug.println("run <skriptname>");
        return;
    }
    runScript(args.word[0].c_str());
    stopAllMotors();
}

void cmdShow(const Args &args)
{
    String text;
    if (args.count < 1 || !loadScript(args.word[0].c_str(), text))
    {
        Debug.println("show <skriptname>");
        return;
    }
    Debug.println(text);
}

void cmdReboot(const Args &)
{
    stopAllMotors();
    Debug.println("Neustart...");
    delay(200);
    ESP.restart();
}

void cmdLed(const Args &args)
{
    setOnboardLedColor(args.getInt(0, 0), args.getInt(1, 0), args.getInt(2, 0));
}

// ---- Motoren & Servos ------------------------------------------------

void cmdMotor(const Args &args)
{
    const int n = args.getInt(0, 0);
    if (!checkMotorNumber(n) || !args.has(1))
    {
        Debug.println("motor <1-6> <-4095..4095> [ms]");
        return;
    }
    shield.motor(n).setSpeed(args.getInt(1, 0));
    if (args.has(2))
    {
        waitMs(args.getInt(2, 0));
        shield.motor(n).stop();
    }
}

void cmdBrake(const Args &args)
{
    if (args.has(0))
    {
        if (checkMotorNumber(args.getInt(0, 0)))
        {
            shield.motor(args.getInt(0, 0)).brake();
        }
        return;
    }
    for (int m = 1; m <= 6; ++m)
    {
        shield.motor(m).brake();
    }
}

void cmdServo(const Args &args)
{
    const int n = args.getInt(0, 0);
    if (!checkServoNumber(n) || !args.has(1))
    {
        Debug.println("servo <1-8> <winkel 0..270>");
        return;
    }
    shield.servo(n).setAngle(args.get(1, 0));
}

void cmdServoUs(const Args &args)
{
    const int n = args.getInt(0, 0);
    if (!checkServoNumber(n) || !args.has(1))
    {
        Debug.println("servous <1-8> <puls 500..2500 us>");
        return;
    }
    shield.servo(n).writeMicroseconds(args.getInt(1, 1500));
}

void cmdServoOff(const Args &args)
{
    const int n = args.getInt(0, 0);
    if (checkServoNumber(n))
    {
        shield.servo(n).disable();
    }
}

void cmdCurrent(const Args &args)
{
    const int n = args.getInt(0, 0);
    if (checkMotorNumber(n))
    {
        Debug.printf("Motor %d: %.3f A\n", n, shield.motor(n).readCurrentAmps());
    }
}

void cmdEncoder(const Args &args)
{
    const int n = args.getInt(0, 1);
    if (checkMotorNumber(n))
    {
        Debug.printf("Encoder Motor %d: %ld\n", n, shield.motor(n).position());
    }
}

// ---- Fahren ---------------------------------------------------------

void cmdDrive(const Args &args)
{
    if (!args.has(0) || !args.has(1))
    {
        Debug.println("drive <links> <rechts> [ms]");
        return;
    }
    if (args.has(2))
    {
        driveFor(args.getInt(0, 0), args.getInt(1, 0), args.getInt(2, 0));
    }
    else
    {
        drive(args.getInt(0, 0), args.getInt(1, 0));
    }
}

void cmdTurn(const Args &args)
{
    turnUntilLine(args.getInt(0, 1), args.getInt(1, 1), args.getInt(2, 10000));
}

void cmdCalibrate(const Args &)
{
    calibrateLine();
}

void cmdFollow(const Args &args)
{
    followLine(args.getInt(0, 1), args.getInt(1, 0));
}

void cmdSensors(const Args &args)
{
    const uint32_t duration = args.getInt(0, 0);
    const uint32_t start = millis();
    do
    {
        printLineSensors(Debug);
    } while (millis() - start < duration && waitMs(200));
}

void cmdBattery(const Args &)
{
    Debug.printf("Batterie: %.2f V   Servo-Schiene: %.2f V\n", shield.batteryVoltage(), shield.servoRailVoltage());
}

// ---- Ein-/Ausgänge (für neue Sensoren) ---------------------------------

void cmdGpio(const Args &args)
{
    const int pin = args.getInt(0, -1);
    if (pin < 0 || pin > 6 || !args.has(1))
    {
        Debug.println("gpio <0-6> <0|1>   (MCP-Erweiterung B0..B6)");
        return;
    }
    const auto p = static_cast<Mcp23017::Pin>(pin);
    shield.gpio().pinMode(p, OUTPUT);
    shield.gpio().digitalWrite(p, args.getInt(1, 0) ? HIGH : LOW);
}

void cmdGpioRead(const Args &args)
{
    const int pin = args.getInt(0, -1);
    if (pin < 0 || pin > 6)
    {
        Debug.println("gpioread <0-6>   (mit Pull-up)");
        return;
    }
    const auto p = static_cast<Mcp23017::Pin>(pin);
    shield.gpio().pinMode(p, INPUT_PULLUP);
    Debug.printf("B%d = %d\n", pin, shield.gpio().digitalRead(p));
}

void cmdPin(const Args &args)
{
    if (!args.has(0) || !args.has(1))
    {
        Debug.println("pin <esp-gpio> <0|1>   VORSICHT: nur freie Pins (siehe pinout.h)");
        return;
    }
    pinMode(args.getInt(0, 0), OUTPUT);
    digitalWrite(args.getInt(0, 0), args.getInt(1, 0) ? HIGH : LOW);
}

void cmdPinRead(const Args &args)
{
    if (!args.has(0))
    {
        Debug.println("pinread <esp-gpio>");
        return;
    }
    const int pin = args.getInt(0, 0);
    pinMode(pin, INPUT_PULLUP);
    Debug.printf("GPIO %d digital = %d, analog = %d\n", pin, digitalRead(pin), analogRead(pin));
}

void cmdI2cScan(const Args &)
{
    Debug.println("I2C-Geräte:");
    for (uint8_t address = 1; address < 127; ++address)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            Debug.printf("  0x%02X\n", address);
        }
    }
    Debug.println("(0x20 MCP23017, 0x40/0x44 PCA9685, 0x48/0x49 ADS1015 sind auf dem Shield)");
}
} // namespace

void registerRobotCommands()
{
    addCommand("help", "help - alle Befehle anzeigen", cmdHelp);
    addCommand("params", "params - alle Parameter anzeigen", cmdParams);
    addCommand("set", "set <name> <wert> - Parameter ändern", cmdSet);
    addCommand("save", "save - Parameter dauerhaft speichern", cmdSave);
    addCommand("defaults", "defaults - Parameter auf Standardwerte", cmdDefaults);
    addCommand("stop", "stop - alle Motoren aus", cmdStop);
    addCommand("wait", "wait <ms> - warten", cmdWait);
    addCommand("run", "run <skript> - Skript/Mission ausführen", cmdRun);
    addCommand("show", "show <skript> - Skript anzeigen", cmdShow);
    addCommand("reboot", "reboot - Neustart", cmdReboot);
    addCommand("led", "led <r> <g> <b> - LED-Farbe 0..255", cmdLed);

    addCommand("motor", "motor <1-6> <-4095..4095> [ms] - einen Motor drehen", cmdMotor);
    addCommand("brake", "brake [1-6] - bremsen (ohne Nummer: alle)", cmdBrake);
    addCommand("servo", "servo <1-8> <winkel> - Servo auf Winkel", cmdServo);
    addCommand("servous", "servous <1-8> <us> - Servo-Puls 500..2500", cmdServoUs);
    addCommand("servooff", "servooff <1-8> - Servo kraftlos", cmdServoOff);
    addCommand("current", "current <1-6> - Motorstrom messen", cmdCurrent);
    addCommand("enc", "enc <1-2> - Encoder-Zählerstand", cmdEncoder);

    addCommand("drive", "drive <links> <rechts> [ms] - fahren", cmdDrive);
    addCommand("turn", "turn <1|-1> [linien] [timeout_ms] - drehen bis Linie (1 = rechts)", cmdTurn);
    addCommand("calibrate", "calibrate - Liniensensor kalibrieren (dreht sich!)", cmdCalibrate);
    addCommand("follow", "follow [checkpoints] [timeout_ms] - Linie folgen (0 = endlos)", cmdFollow);
    addCommand("sensors", "sensors [ms] - Liniensensor-Werte anzeigen", cmdSensors);
    addCommand("battery", "battery - Batteriespannung", cmdBattery);

    addCommand("gpio", "gpio <0-6> <0|1> - MCP-Ausgang B0..B6", cmdGpio);
    addCommand("gpioread", "gpioread <0-6> - MCP-Eingang lesen", cmdGpioRead);
    addCommand("pin", "pin <gpio> <0|1> - ESP32-Pin setzen", cmdPin);
    addCommand("pinread", "pinread <gpio> - ESP32-Pin lesen (digital+analog)", cmdPinRead);
    addCommand("i2cscan", "i2cscan - I2C-Bus nach Geräten absuchen", cmdI2cScan);
}
