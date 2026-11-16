#include "devices.h"
#include "program.h"

#include <vector>

#include "commands.h"
#include "params.h"
#include "pinout.h"
#include "robot.h"
#include "shared-globals.h"
#include "sorter.h"
#include "wifi-debug.h"

namespace
{
// ---------------------------------------------------------------------
//  Variablen
// ---------------------------------------------------------------------
struct Variable
{
    char name[16];
    float value;
};
constexpr int kMaxVariables = 48;
Variable variables[kMaxVariables];
int variableCount = 0;

// ---------------------------------------------------------------------
//  Kontext für "time", "checkpoints" und Liniensensor
// ---------------------------------------------------------------------
uint32_t programStartMs = 0;
uint32_t contextStartMs = 0;
int contextCheckpoints = 0;
bool insideUntil = false;
uint16_t lastLinePosition = 2000;
int callDepth = 0;

void refreshLineIfNeeded()
{
    // In until-Schleifen wird der Sensor sowieso jeden Schritt gelesen.
    if (!insideUntil && lineCalibrated)
    {
        lastLinePosition = readLine();
    }
}

bool isIdentifierStart(char c) { return isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool isIdentifierChar(char c) { return isalnum(static_cast<unsigned char>(c)) || c == '_'; }

// ---------------------------------------------------------------------
//  Ausdrücke:  or > and > not > Vergleich > + - > * / % > Vorzeichen
// ---------------------------------------------------------------------
class ExpressionParser
{
public:
    explicit ExpressionParser(const char *text) : p(text) {}

    float parse()
    {
        const float value = parseOr();
        skipSpaces();
        if (ok && *p != '\0')
        {
            fail(String("unerwartet: '") + p + "'");
        }
        return value;
    }

    bool ok = true;
    String error;

private:
    const char *p;

    void skipSpaces()
    {
        while (*p == ' ' || *p == '\t')
        {
            ++p;
        }
    }

    void fail(const String &message)
    {
        if (ok)
        {
            ok = false;
            error = message;
        }
    }

    bool matchWord(const char *word)
    {
        skipSpaces();
        const size_t n = strlen(word);
        if (strncasecmp(p, word, n) == 0 && !isIdentifierChar(p[n]))
        {
            p += n;
            return true;
        }
        return false;
    }

    bool matchOperator(const char *op)
    {
        skipSpaces();
        const size_t n = strlen(op);
        if (strncmp(p, op, n) == 0)
        {
            p += n;
            return true;
        }
        return false;
    }

    float parseOr()
    {
        float value = parseAnd();
        while (ok && (matchOperator("||") || matchWord("or")))
        {
            const float right = parseAnd();
            value = (value != 0 || right != 0) ? 1 : 0;
        }
        return value;
    }

    float parseAnd()
    {
        float value = parseNot();
        while (ok && (matchOperator("&&") || matchWord("and")))
        {
            const float right = parseNot();
            value = (value != 0 && right != 0) ? 1 : 0;
        }
        return value;
    }

    float parseNot()
    {
        if (matchWord("not"))
        {
            return parseNot() == 0 ? 1 : 0;
        }
        skipSpaces();
        if (*p == '!' && p[1] != '=')
        {
            ++p;
            return parseNot() == 0 ? 1 : 0;
        }
        return parseComparison();
    }

    float parseComparison()
    {
        const float left = parseSum();
        if (matchOperator("<="))
            return left <= parseSum() ? 1 : 0;
        if (matchOperator(">="))
            return left >= parseSum() ? 1 : 0;
        if (matchOperator("=="))
            return left == parseSum() ? 1 : 0;
        if (matchOperator("!="))
            return left != parseSum() ? 1 : 0;
        if (matchOperator("<"))
            return left < parseSum() ? 1 : 0;
        if (matchOperator(">"))
            return left > parseSum() ? 1 : 0;
        if (matchOperator("="))
            return left == parseSum() ? 1 : 0; // "=" wie "==" erlauben
        return left;
    }

    float parseSum()
    {
        float value = parseProduct();
        while (ok)
        {
            if (matchOperator("+"))
                value += parseProduct();
            else if (matchOperator("-"))
                value -= parseProduct();
            else
                break;
        }
        return value;
    }

    float parseProduct()
    {
        float value = parseUnary();
        while (ok)
        {
            if (matchOperator("*"))
            {
                value *= parseUnary();
            }
            else if (matchOperator("/"))
            {
                const float d = parseUnary();
                value = d == 0 ? 0 : value / d;
            }
            else if (matchOperator("%"))
            {
                const float d = parseUnary();
                value = d == 0 ? 0 : fmodf(value, d);
            }
            else
            {
                break;
            }
        }
        return value;
    }

    float parseUnary()
    {
        if (matchOperator("-"))
            return -parseUnary();
        if (matchOperator("+"))
            return parseUnary();
        return parsePrimary();
    }

    float parsePrimary()
    {
        skipSpaces();
        if (*p == '(')
        {
            ++p;
            const float value = parseOr();
            skipSpaces();
            if (*p == ')')
                ++p;
            else
                fail("')' fehlt");
            return value;
        }
        if (isdigit(static_cast<unsigned char>(*p)) || (*p == '.' && isdigit(static_cast<unsigned char>(p[1]))))
        {
            char *end = nullptr;
            const float value = strtof(p, &end);
            p = end;
            return value;
        }
        if (isIdentifierStart(*p))
        {
            const char *start = p;
            while (isIdentifierChar(*p))
            {
                ++p;
            }
            String name;
            name.concat(start, p - start);
            skipSpaces();
            if (*p == '(')
            {
                ++p;
                float args[4] = {0, 0, 0, 0};
                int count = 0;
                skipSpaces();
                if (*p != ')')
                {
                    while (ok)
                    {
                        const float value = parseOr();
                        if (count < 4)
                            args[count++] = value;
                        skipSpaces();
                        if (*p == ',')
                        {
                            ++p;
                            continue;
                        }
                        break;
                    }
                }
                skipSpaces();
                if (*p == ')')
                    ++p;
                else
                    fail("')' fehlt nach " + name);
                return ok ? callFunction(name, args, count) : 0;
            }
            return lookupName(name);
        }
        fail(*p ? String("unerwartet: '") + p + "'" : String("Ausdruck unvollständig"));
        return 0;
    }

    float callFunction(const String &name, const float *a, int n)
    {
        const int i = n > 0 ? static_cast<int>(lroundf(a[0])) : 0;
        if (name.equalsIgnoreCase("qtr"))
        {
            if (i < 0 || i >= static_cast<int>(QTR_SENSOR_COUNT))
            {
                fail("qtr(0..4)");
                return 0;
            }
            refreshLineIfNeeded();
            return qtrSensorValues[i];
        }
        if (name.equalsIgnoreCase("gpio"))
        {
            if (i < 0 || i > 6)
            {
                fail("gpio(0..6)");
                return 0;
            }
            const auto pin = static_cast<Mcp23017::Pin>(i);
            shield.gpio().pinMode(pin, INPUT_PULLUP);
            return shield.gpio().digitalRead(pin);
        }
        if (name.equalsIgnoreCase("pin"))
        {
            pinMode(i, INPUT_PULLUP);
            return digitalRead(i);
        }
        if (name.equalsIgnoreCase("analog"))
            return analogRead(i);
        if (name.equalsIgnoreCase("enc"))
            return (i >= 1 && i <= 6) ? shield.motor(i).position() : 0;
        if (name.equalsIgnoreCase("current"))
            return (i >= 1 && i <= 6) ? shield.motor(i).readCurrentAmps() : 0;
        if (name.equalsIgnoreCase("abs"))
            return fabsf(n > 0 ? a[0] : 0);
        if (name.equalsIgnoreCase("min") && n >= 2)
            return min(a[0], a[1]);
        if (name.equalsIgnoreCase("max") && n >= 2)
            return max(a[0], a[1]);
        if (name.equalsIgnoreCase("random") && n >= 2)
            return static_cast<float>(random(static_cast<long>(a[0]), static_cast<long>(a[1]) + 1));
        fail("unbekannte Funktion " + name + "()");
        return 0;
    }

    float lookupName(const String &name)
    {
        if (name.equalsIgnoreCase("true"))
            return 1;
        if (name.equalsIgnoreCase("false"))
            return 0;
        if (name.equalsIgnoreCase("time"))
            return static_cast<float>(millis() - contextStartMs);
        if (name.equalsIgnoreCase("millis"))
            return static_cast<float>(millis());
        if (name.equalsIgnoreCase("checkpoints"))
            return static_cast<float>(contextCheckpoints);
        if (name.equalsIgnoreCase("lineCenter"))
        {
            refreshLineIfNeeded();
            return centerOnLine() ? 1 : 0;
        }
        if (name.equalsIgnoreCase("linePos"))
        {
            refreshLineIfNeeded();
            return lastLinePosition;
        }
        if (name.equalsIgnoreCase("battery"))
            return shield.batteryVoltage();
        if (name.equalsIgnoreCase("cube"))
            return cubeAtExit() ? 1 : 0;
        if (name.equalsIgnoreCase("wheelSensor"))
            return readWheelSensor();
        if (name.equalsIgnoreCase("exitSensor"))
            return readExitSensor();

        float value = 0;
        if (getVariable(name.c_str(), value) || getParam(name.c_str(), value) || lookupDevice(name.c_str(), value))
        {
            return value;
        }
        fail("unbekannt: " + name);
        return 0;
    }
};

// ---------------------------------------------------------------------
//  Programmzeilen
// ---------------------------------------------------------------------
enum class Kind
{
    Empty,
    Command,
    Repeat,
    While,
    If,
    Else,
    End,
    Let,
    Print,
    Until
};

struct Line
{
    String text;  // ohne Kommentar, getrimmt
    String rest;  // Text nach dem Schlüsselwort
    Kind kind = Kind::Empty;
    int number = 0;  // Zeilennummer für Fehlermeldungen
    int match = -1;  // Index des passenden "end"
    int elseAt = -1; // Index des passenden "else"
};

String firstWord(const String &text)
{
    const int space = text.indexOf(' ');
    String word = space < 0 ? text : text.substring(0, space);
    word.toLowerCase();
    return word;
}

int findUntil(const String &text)
{
    String lower = text;
    lower.toLowerCase();
    return lower.indexOf(" until ");
}

void reportError(const Line &line, const String &message)
{
    Debug.printf("FEHLER Zeile %d: %s   [%s]\n", line.number, message.c_str(), line.text.c_str());
}

bool buildLines(const char *text, std::vector<Line> &lines)
{
    int number = 1;
    const char *start = text;
    while (true)
    {
        const char *end = start;
        while (*end && *end != '\n' && *end != ';')
        {
            ++end;
        }
        Line line;
        line.number = number;
        line.text.concat(start, end - start);
        const int comment = line.text.indexOf('#');
        if (comment >= 0)
        {
            line.text.remove(comment);
        }
        line.text.trim();

        if (line.text.length() > 0)
        {
            const String word = firstWord(line.text);
            const int space = line.text.indexOf(' ');
            line.rest = space < 0 ? String() : line.text.substring(space + 1);
            line.rest.trim();
            if (word == "repeat")
                line.kind = Kind::Repeat;
            else if (word == "while")
                line.kind = Kind::While;
            else if (word == "if")
                line.kind = Kind::If;
            else if (word == "else")
                line.kind = Kind::Else;
            else if (word == "end")
                line.kind = Kind::End;
            else if (word == "let")
                line.kind = Kind::Let;
            else if (word == "print")
                line.kind = Kind::Print;
            else if (findUntil(line.text) > 0)
                line.kind = Kind::Until;
            else
                line.kind = Kind::Command;
            lines.push_back(line);
        }

        if (*end == '\n')
        {
            ++number;
        }
        if (*end == '\0')
        {
            break;
        }
        start = end + 1;
    }

    // Blöcke zuordnen: repeat/while/if ... [else] ... end
    std::vector<int> open;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i)
    {
        Line &line = lines[i];
        if (line.kind == Kind::Repeat || line.kind == Kind::While || line.kind == Kind::If)
        {
            if (line.rest.length() == 0)
            {
                reportError(line, "Bedingung/Anzahl fehlt");
                return false;
            }
            open.push_back(i);
        }
        else if (line.kind == Kind::Else)
        {
            if (open.empty() || lines[open.back()].kind != Kind::If || lines[open.back()].elseAt >= 0)
            {
                reportError(line, "'else' ohne passendes 'if'");
                return false;
            }
            lines[open.back()].elseAt = i;
        }
        else if (line.kind == Kind::End)
        {
            if (open.empty())
            {
                reportError(line, "'end' ohne passenden Block");
                return false;
            }
            lines[open.back()].match = i;
            open.pop_back();
        }
    }
    if (!open.empty())
    {
        reportError(lines[open.back()], "'end' fehlt");
        return false;
    }
    return true;
}

bool evalForLine(const Line &line, const String &expression, float &value)
{
    bool ok = true;
    String error;
    value = evalExpression(expression.c_str(), ok, &error);
    if (!ok)
    {
        reportError(line, error);
    }
    return ok;
}

// Zerlegt "1000 (speed + 200)" in einzelne Ausdrücke (Klammern zusammen).
std::vector<String> splitArguments(const String &text)
{
    std::vector<String> parts;
    String current;
    int depth = 0;
    for (size_t i = 0; i < text.length(); ++i)
    {
        const char c = text[i];
        if (c == '(')
            ++depth;
        if (c == ')')
            --depth;
        if ((c == ' ' || c == '\t') && depth <= 0)
        {
            if (current.length() > 0)
            {
                parts.push_back(current);
                current = "";
            }
            continue;
        }
        current += c;
    }
    if (current.length() > 0)
    {
        parts.push_back(current);
    }
    return parts;
}

// follow / drive / turn / motor / wait ... until <bedingung>
bool runUntil(const Line &line)
{
    const int at = findUntil(line.text);
    String action = line.text.substring(0, at);
    String condition = line.text.substring(at + 7);
    action.trim();
    condition.trim();
    if (condition.length() == 0)
    {
        reportError(line, "Bedingung nach 'until' fehlt");
        return false;
    }

    std::vector<String> words = splitArguments(action);
    String verb = words[0];
    verb.toLowerCase();
    float a[2] = {0, 0};
    for (size_t i = 1; i < words.size() && i <= 2; ++i)
    {
        if (!evalForLine(line, words[i], a[i - 1]))
        {
            return false;
        }
    }

    enum
    {
        Follow,
        Drive,
        Turn,
        MotorRun,
        Wait
    } mode;
    if (verb == "follow")
        mode = Follow;
    else if (verb == "drive")
        mode = Drive;
    else if (verb == "turn")
        mode = Turn;
    else if (verb == "motor")
        mode = MotorRun;
    else if (verb == "wait")
        mode = Wait;
    else
    {
        reportError(line, "'until' geht nur mit follow, drive, turn, motor, wait");
        return false;
    }
    if (mode == Follow && !lineCalibrated)
    {
        reportError(line, "erst 'calibrate' ausführen");
        return false;
    }
    const int motorNumber = static_cast<int>(a[0]);
    if (mode == MotorRun && (motorNumber < 1 || motorNumber > 6))
    {
        reportError(line, "motor <1-6> <speed> until ...");
        return false;
    }

    // Kontext sichern (time / checkpoints gelten für diesen Block)
    const uint32_t savedStart = contextStartMs;
    const int savedCheckpoints = contextCheckpoints;
    contextStartMs = millis();
    contextCheckpoints = 0;
    insideUntil = true;

    LineStatus previous = lineStatus;
    if (lineCalibrated)
    {
        lastLinePosition = readLine();
        previous = lineStatus;
    }

    switch (mode)
    {
    case Follow:
        resetFollow();
        break;
    case Drive:
        drive(static_cast<int>(a[0]), static_cast<int>(a[1]));
        break;
    case Turn:
    {
        const int dir = a[0] >= 0 ? 1 : -1;
        drive(dir * turnSpeed, -dir * turnSpeed);
        break;
    }
    case MotorRun:
        shield.motor(motorNumber).setSpeed(static_cast<int>(a[1]));
        break;
    case Wait:
        break;
    }

    const uint32_t periodUs = 1000000UL / max(driveLoopHzTarget, 1);
    uint32_t lastStepUs = micros() - periodUs;
    bool ok = true;
    while (true)
    {
        serviceBackground();
        if (stopRequested())
        {
            ok = false;
            break;
        }
        if (micros() - lastStepUs < periodUs)
        {
            delayMicroseconds(200);
            continue;
        }
        lastStepUs = micros();

        if (lineCalibrated)
        {
            lastLinePosition = readLine();
            if (lineStatus == STOP && previous != STOP)
            {
                ++contextCheckpoints;
                ++checkpointCounter;
                lastCheckpointTimeMs = millis();
            }
            previous = lineStatus;
            if (mode == Follow)
            {
                followLineStep(lastLinePosition);
            }
        }

        float done = 0;
        if (!evalForLine(line, condition, done))
        {
            ok = false;
            break;
        }
        if (done != 0)
        {
            break;
        }
    }

    if (mode == MotorRun)
        shield.motor(motorNumber).stop();
    else if (mode != Wait)
        stopDrive();

    insideUntil = false;
    contextStartMs = savedStart;
    contextCheckpoints = savedCheckpoints;
    return ok;
}

bool runLet(const Line &line)
{
    const int eq = line.rest.indexOf('=');
    if (eq <= 0)
    {
        reportError(line, "let <name> = <wert>");
        return false;
    }
    String name = line.rest.substring(0, eq);
    name.trim();
    bool validName = name.length() > 0 && name.length() < 16 && isIdentifierStart(name[0]);
    for (size_t i = 0; i < name.length(); ++i)
    {
        validName = validName && isIdentifierChar(name[i]);
    }
    if (!validName)
    {
        reportError(line, "ungültiger Variablenname '" + name + "'");
        return false;
    }
    float value = 0;
    if (!evalForLine(line, line.rest.substring(eq + 1), value))
    {
        return false;
    }
    setVariable(name.c_str(), value);
    return true;
}

bool runPrint(const Line &line)
{
    // "print Runde {runde} fertig" -> Ausdrücke in {} werden ersetzt
    String out;
    const String &text = line.rest;
    int pos = 0;
    while (pos < static_cast<int>(text.length()))
    {
        const int open = text.indexOf('{', pos);
        if (open < 0)
        {
            out += text.substring(pos);
            break;
        }
        const int close = text.indexOf('}', open);
        if (close < 0)
        {
            out += text.substring(pos);
            break;
        }
        out += text.substring(pos, open);
        float value = 0;
        if (!evalForLine(line, text.substring(open + 1, close), value))
        {
            return false;
        }
        char buffer[24];
        snprintf(buffer, sizeof(buffer), "%g", value);
        out += buffer;
        pos = close + 1;
    }
    Debug.println(out);
    return true;
}

bool executeRange(std::vector<Line> &lines, int from, int to)
{
    int i = from;
    while (i < to)
    {
        serviceBackground();
        if (stopRequested())
        {
            return false;
        }
        Line &line = lines[i];
        switch (line.kind)
        {
        case Kind::Repeat:
        {
            float count = 0;
            if (!evalForLine(line, line.rest, count))
                return false;
            for (int k = 0; k < static_cast<int>(count); ++k)
            {
                if (!executeRange(lines, i + 1, line.match))
                    return false;
            }
            i = line.match + 1;
            break;
        }
        case Kind::While:
        {
            while (true)
            {
                float condition = 0;
                if (!evalForLine(line, line.rest, condition))
                    return false;
                if (condition == 0)
                    break;
                if (!executeRange(lines, i + 1, line.match))
                    return false;
                delay(1); // Schleife ohne Wartezeit darf den Rest nicht blockieren
            }
            i = line.match + 1;
            break;
        }
        case Kind::If:
        {
            float condition = 0;
            if (!evalForLine(line, line.rest, condition))
                return false;
            const int bodyEnd = line.elseAt >= 0 ? line.elseAt : line.match;
            if (condition != 0)
            {
                if (!executeRange(lines, i + 1, bodyEnd))
                    return false;
            }
            else if (line.elseAt >= 0)
            {
                if (!executeRange(lines, line.elseAt + 1, line.match))
                    return false;
            }
            i = line.match + 1;
            break;
        }
        case Kind::Let:
            if (!runLet(line))
                return false;
            ++i;
            break;
        case Kind::Print:
            if (!runPrint(line))
                return false;
            ++i;
            break;
        case Kind::Until:
            if (!runUntil(line))
                return false;
            ++i;
            break;
        case Kind::Command:
            if (!runCommand(line.text.c_str()))
            {
                if (!stopRequested())
                {
                    reportError(line, "Befehl fehlgeschlagen");
                }
                return false;
            }
            if (stopRequested())
                return false;
            ++i;
            break;
        default:
            ++i;
            break;
        }
    }
    return true;
}
} // namespace

float evalExpression(const char *text, bool &ok, String *error)
{
    ExpressionParser parser(text);
    const float value = parser.parse();
    ok = parser.ok;
    if (!ok && error != nullptr)
    {
        *error = parser.error;
    }
    return value;
}

bool runProgram(const char *text)
{
    std::vector<Line> lines;
    if (!buildLines(text, lines))
    {
        return false;
    }
    return executeRange(lines, 0, static_cast<int>(lines.size()));
}

bool callScript(const char *name, const float *args, int argCount)
{
    String text;
    if (!loadScript(name, text))
    {
        Debug.printf("Skript/Funktion '%s' nicht gefunden.\n", name);
        return false;
    }
    if (callDepth >= 8)
    {
        Debug.println("Funktionen zu tief verschachtelt (max. 8) – ruft sich etwas selbst auf?");
        return false;
    }

    // arg1..arg8 setzen und danach wiederherstellen
    float saved[8];
    char argName[8];
    for (int i = 0; i < 8; ++i)
    {
        snprintf(argName, sizeof(argName), "arg%d", i + 1);
        saved[i] = 0;
        getVariable(argName, saved[i]);
        setVariable(argName, i < argCount && !isnan(args[i]) ? args[i] : 0);
    }
    ++callDepth;
    const bool ok = runProgram(text.c_str());
    --callDepth;
    for (int i = 0; i < 8; ++i)
    {
        snprintf(argName, sizeof(argName), "arg%d", i + 1);
        setVariable(argName, saved[i]);
    }
    return ok;
}

void setVariable(const char *name, float value)
{
    for (int i = 0; i < variableCount; ++i)
    {
        if (strcasecmp(variables[i].name, name) == 0)
        {
            variables[i].value = value;
            return;
        }
    }
    if (variableCount >= kMaxVariables)
    {
        Debug.println("Zu viele Variablen (max. 48).");
        return;
    }
    strlcpy(variables[variableCount].name, name, sizeof(variables[variableCount].name));
    variables[variableCount].value = value;
    ++variableCount;
}

bool getVariable(const char *name, float &value)
{
    for (int i = 0; i < variableCount; ++i)
    {
        if (strcasecmp(variables[i].name, name) == 0)
        {
            value = variables[i].value;
            return true;
        }
    }
    return false;
}

void clearVariables()
{
    variableCount = 0;
}

void variablesToJson(String &out)
{
    out += '{';
    bool first = true;
    char buffer[24];
    for (int i = 0; i < variableCount; ++i)
    {
        if (strncmp(variables[i].name, "arg", 3) == 0)
        {
            continue;
        }
        if (!first)
        {
            out += ',';
        }
        first = false;
        out += '"';
        out += variables[i].name;
        out += "\":";
        snprintf(buffer, sizeof(buffer), "%g", variables[i].value);
        out += isfinite(variables[i].value) ? buffer : "null";
    }
    out += '}';
}

void resetProgramClock()
{
    programStartMs = millis();
    contextStartMs = programStartMs;
    contextCheckpoints = 0;
}

int programDepth()
{
    return callDepth;
}
