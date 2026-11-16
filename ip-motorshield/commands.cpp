#include "commands.h"
#include "params.h"
#include "shared-globals.h"
#include "wifi-debug.h"

#include <Preferences.h>
#include <nvs.h>

namespace
{
struct CommandEntry
{
    const char *name;
    const char *usage;
    CommandFunction function;
};

struct ScriptEntry
{
    const char *name;
    const char *text;
};

constexpr size_t kMaxCommands = 64;
CommandEntry commands[kMaxCommands];
size_t commandCountUsed = 0;

constexpr size_t kMaxDefaultScripts = 16;
ScriptEntry defaultScripts[kMaxDefaultScripts];
size_t defaultScriptCount = 0;

struct QueuedLine
{
    char text[160];
};
QueueHandle_t commandQueue = nullptr;

volatile bool stopFlag = false;
volatile bool running = false;
char runningLine[160] = "";
portMUX_TYPE runningLineMutex = portMUX_INITIALIZER_UNLOCKED;
int scriptDepth = 0;
void (*backgroundHook)() = nullptr;

CommandEntry *findCommand(const String &name)
{
    for (size_t i = 0; i < commandCountUsed; ++i)
    {
        if (name.equalsIgnoreCase(commands[i].name))
        {
            return &commands[i];
        }
    }
    return nullptr;
}

const char *findDefaultScript(const char *name)
{
    for (size_t i = 0; i < defaultScriptCount; ++i)
    {
        if (strcasecmp(defaultScripts[i].name, name) == 0)
        {
            return defaultScripts[i].text;
        }
    }
    return nullptr;
}

void appendJsonString(String &out, const char *text)
{
    out += '"';
    for (const char *c = text; *c; ++c)
    {
        switch (*c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += *c;
        }
    }
    out += '"';
}

void setRunningLine(const char *line)
{
    // Kein String/malloc im kritischen Abschnitt (Interrupts sind dort aus).
    portENTER_CRITICAL(&runningLineMutex);
    strlcpy(runningLine, line, sizeof(runningLine));
    portEXIT_CRITICAL(&runningLineMutex);
}
} // namespace

void addCommand(const char *name, const char *usage, CommandFunction function)
{
    if (findCommand(name) != nullptr)
    {
        Debug.printf("Befehl '%s' gibt es schon, wird ignoriert.\n", name);
        return;
    }
    if (commandCountUsed >= kMaxCommands)
    {
        Debug.println("Zu viele Befehle (kMaxCommands in commands.cpp erhöhen).");
        return;
    }
    commands[commandCountUsed++] = {name, usage, function};
}

bool runCommand(const char *lineText)
{
    String line(lineText);
    const int commentAt = line.indexOf('#');
    if (commentAt >= 0)
    {
        line.remove(commentAt);
    }
    line.trim();
    if (line.length() == 0)
    {
        return true;
    }

    int nameEnd = line.indexOf(' ');
    String name = nameEnd < 0 ? line : line.substring(0, nameEnd);
    Args args;
    args.rest = nameEnd < 0 ? String() : line.substring(nameEnd + 1);
    args.rest.trim();

    // Argumente zerlegen: Zahl, sonst Parametername, sonst nur Text.
    int pos = 0;
    while (pos < static_cast<int>(args.rest.length()) && args.count < Args::kMax)
    {
        while (pos < static_cast<int>(args.rest.length()) && args.rest[pos] == ' ')
        {
            ++pos;
        }
        int end = args.rest.indexOf(' ', pos);
        if (end < 0)
        {
            end = args.rest.length();
        }
        if (end == pos)
        {
            break;
        }
        String word = args.rest.substring(pos, end);
        char *parseEnd = nullptr;
        float value = strtof(word.c_str(), &parseEnd);
        if (parseEnd == word.c_str() || *parseEnd != '\0')
        {
            if (!getParam(word.c_str(), value))
            {
                value = NAN;
            }
        }
        args.word[args.count] = word;
        args.value[args.count] = value;
        ++args.count;
        pos = end;
    }

    CommandEntry *command = findCommand(name);
    if (command == nullptr)
    {
        Debug.printf("Unbekannter Befehl: '%s'  (help zeigt alle Befehle)\n", name.c_str());
        return false;
    }
    Debug.printf("> %s\n", line.c_str());
    command->function(args);
    return true;
}

bool runScriptText(const char *text)
{
    if (scriptDepth >= 4)
    {
        Debug.println("Skripte zu tief verschachtelt (max. 4).");
        return false;
    }
    ++scriptDepth;
    const char *lineStart = text;
    bool ok = true;
    while (*lineStart && ok)
    {
        const char *lineEnd = lineStart;
        while (*lineEnd && *lineEnd != '\n' && *lineEnd != ';')
        {
            ++lineEnd;
        }
        String line;
        line.concat(lineStart, lineEnd - lineStart);
        if (stopRequested())
        {
            ok = false;
            break;
        }
        ok = runCommand(line.c_str());
        lineStart = *lineEnd ? lineEnd + 1 : lineEnd;
    }
    --scriptDepth;
    return ok && !stopRequested();
}

bool runScript(const char *name)
{
    String text;
    if (!loadScript(name, text))
    {
        Debug.printf("Skript '%s' nicht gefunden.\n", name);
        return false;
    }
    Debug.printf("--- Skript '%s' startet ---\n", name);
    const bool ok = runScriptText(text.c_str());
    Debug.printf("--- Skript '%s' %s ---\n", name, ok ? "fertig" : "abgebrochen");
    return ok;
}

void addDefaultScript(const char *name, const char *text)
{
    if (defaultScriptCount < kMaxDefaultScripts)
    {
        defaultScripts[defaultScriptCount++] = {name, text};
    }
}

bool loadScript(const char *name, String &text)
{
    Preferences prefs;
    if (prefs.begin("scripts", true))
    {
        if (prefs.isKey(name))
        {
            text = prefs.getString(name);
            prefs.end();
            return true;
        }
        prefs.end();
    }
    const char *fallback = findDefaultScript(name);
    if (fallback == nullptr)
    {
        return false;
    }
    text = fallback;
    return true;
}

bool storeScript(const char *name, const char *text)
{
    if (strlen(name) == 0 || strlen(name) > 15 || strchr(name, ' ') != nullptr)
    {
        Debug.println("Skriptname: 1-15 Zeichen, keine Leerzeichen.");
        return false;
    }
    Preferences prefs;
    prefs.begin("scripts", false);
    if (strlen(text) == 0)
    {
        prefs.remove(name);
        Debug.printf("Skript '%s' gelöscht (Standardversion aus dem Code gilt wieder, falls vorhanden).\n", name);
    }
    else
    {
        prefs.putString(name, text);
        Debug.printf("Skript '%s' gespeichert.\n", name);
    }
    prefs.end();
    return true;
}

void queueCommand(const char *line)
{
    if (commandQueue == nullptr)
    {
        commandQueue = xQueueCreate(8, sizeof(QueuedLine));
    }
    QueuedLine item;
    strlcpy(item.text, line, sizeof(item.text));
    if (xQueueSend(commandQueue, &item, 0) != pdTRUE)
    {
        Debug.println("Befehls-Warteschlange voll.");
    }
}

void processCommandQueue()
{
    if (commandQueue == nullptr || running)
    {
        return;
    }
    QueuedLine item;
    while (xQueueReceive(commandQueue, &item, 0) == pdTRUE)
    {
        stopFlag = false;
        running = true;
        robotMotionEnabled = true; // Status für die alte Telemetrie-Seite
        setRunningLine(item.text);
        runCommand(item.text);
        setRunningLine("");
        robotMotionEnabled = false;
        running = false;
    }
}

void requestStop()
{
    stopFlag = true;
    if (commandQueue != nullptr)
    {
        xQueueReset(commandQueue); // wartende Befehle verwerfen
    }
}

bool stopRequested()
{
    return stopFlag || wifiDebugMotionLocked();
}

bool commandRunning()
{
    return running;
}

String currentCommandLine()
{
    char copy[sizeof(runningLine)];
    portENTER_CRITICAL(&runningLineMutex);
    memcpy(copy, runningLine, sizeof(copy));
    portEXIT_CRITICAL(&runningLineMutex);
    return String(copy);
}

bool waitMs(uint32_t ms)
{
    const uint32_t start = millis();
    while (millis() - start < ms)
    {
        serviceBackground();
        if (stopRequested())
        {
            return false;
        }
        delay(min<uint32_t>(5, ms - (millis() - start)));
    }
    serviceBackground();
    return !stopRequested();
}

void setBackgroundHook(void (*hook)())
{
    backgroundHook = hook;
}

void serviceBackground()
{
    if (backgroundHook != nullptr)
    {
        backgroundHook();
    }
}

void printCommands(Print &out)
{
    for (size_t i = 0; i < commandCountUsed; ++i)
    {
        out.printf("  %s\n", commands[i].usage);
    }
}

void commandsToJson(String &out)
{
    out += '[';
    for (size_t i = 0; i < commandCountUsed; ++i)
    {
        if (i > 0)
        {
            out += ',';
        }
        out += "{\"name\":";
        appendJsonString(out, commands[i].name);
        out += ",\"usage\":";
        appendJsonString(out, commands[i].usage);
        out += '}';
    }
    out += ']';
}

void scriptsToJson(String &out)
{
    // Namen sammeln: Standard-Skripte aus dem Code + im Flash gespeicherte.
    String names[32];
    size_t count = 0;
    auto addName = [&](const char *name)
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (names[i].equalsIgnoreCase(name))
            {
                return;
            }
        }
        if (count < 32)
        {
            names[count++] = name;
        }
    };
    for (size_t i = 0; i < defaultScriptCount; ++i)
    {
        addName(defaultScripts[i].name);
    }
    nvs_iterator_t it = nullptr;
    esp_err_t res = nvs_entry_find(NVS_DEFAULT_PART_NAME, "scripts", NVS_TYPE_STR, &it);
    while (res == ESP_OK)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        addName(info.key);
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);

    out += '[';
    for (size_t i = 0; i < count; ++i)
    {
        String text;
        loadScript(names[i].c_str(), text);
        if (i > 0)
        {
            out += ',';
        }
        out += "{\"name\":";
        appendJsonString(out, names[i].c_str());
        out += ",\"text\":";
        appendJsonString(out, text.c_str());
        out += '}';
    }
    out += ']';
}
