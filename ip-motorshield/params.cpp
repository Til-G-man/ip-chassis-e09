#include "params.h"
#include "wifi-debug.h"

#include <Preferences.h>

namespace
{
enum class ParamType : uint8_t
{
    Float,
    Int,
    Bool
};

struct ParamEntry
{
    const char *name;
    const char *help;
    ParamType type;
    void *ptr;
    float minValue;
    float maxValue;
    float defaultValue;
};

constexpr size_t kMaxParams = 64;
ParamEntry params[kMaxParams];
size_t paramCountUsed = 0;

float readValue(const ParamEntry &p)
{
    switch (p.type)
    {
    case ParamType::Float:
        return *static_cast<float *>(p.ptr);
    case ParamType::Int:
        return static_cast<float>(*static_cast<int *>(p.ptr));
    default:
        return *static_cast<bool *>(p.ptr) ? 1.0f : 0.0f;
    }
}

void writeValue(ParamEntry &p, float value)
{
    value = constrain(value, p.minValue, p.maxValue);
    switch (p.type)
    {
    case ParamType::Float:
        *static_cast<float *>(p.ptr) = value;
        break;
    case ParamType::Int:
        *static_cast<int *>(p.ptr) = static_cast<int>(lroundf(value));
        break;
    default:
        *static_cast<bool *>(p.ptr) = value >= 0.5f;
        break;
    }
}

ParamEntry *findParam(const char *name)
{
    for (size_t i = 0; i < paramCountUsed; ++i)
    {
        if (strcasecmp(params[i].name, name) == 0)
        {
            return &params[i];
        }
    }
    return nullptr;
}

void addEntry(const char *name, ParamType type, void *ptr, float minValue, float maxValue, const char *help)
{
    if (strlen(name) > 15)
    {
        Debug.printf("Param '%s': Name zu lang (max. 15 Zeichen), wird ignoriert.\n", name);
        return;
    }
    if (findParam(name) != nullptr)
    {
        Debug.printf("Param '%s' gibt es schon, wird ignoriert.\n", name);
        return;
    }
    if (paramCountUsed >= kMaxParams)
    {
        Debug.println("Zu viele Parameter (kMaxParams in params.cpp erhöhen).");
        return;
    }
    ParamEntry &p = params[paramCountUsed++];
    p = {name, help, type, ptr, minValue, maxValue, 0.0f};
    p.defaultValue = readValue(p);
}

void formatValue(const ParamEntry &p, char *buffer, size_t size)
{
    if (p.type == ParamType::Float)
    {
        snprintf(buffer, size, "%g", readValue(p));
    }
    else
    {
        snprintf(buffer, size, "%d", static_cast<int>(readValue(p)));
    }
}
} // namespace

void addParam(const char *name, float *value, float minValue, float maxValue, const char *help)
{
    addEntry(name, ParamType::Float, value, minValue, maxValue, help);
}

void addParam(const char *name, int *value, int minValue, int maxValue, const char *help)
{
    addEntry(name, ParamType::Int, value, minValue, maxValue, help);
}

void addParam(const char *name, bool *value, const char *help)
{
    addEntry(name, ParamType::Bool, value, 0, 1, help);
}

bool setParam(const char *name, float value)
{
    ParamEntry *p = findParam(name);
    if (p == nullptr)
    {
        return false;
    }
    writeValue(*p, value);
    return true;
}

bool getParam(const char *name, float &value)
{
    ParamEntry *p = findParam(name);
    if (p == nullptr)
    {
        return false;
    }
    value = readValue(*p);
    return true;
}

void loadParams()
{
    Preferences prefs;
    if (!prefs.begin("params", true))
    {
        return; // noch nie gespeichert
    }
    size_t loaded = 0;
    for (size_t i = 0; i < paramCountUsed; ++i)
    {
        if (prefs.isKey(params[i].name))
        {
            writeValue(params[i], prefs.getFloat(params[i].name, readValue(params[i])));
            ++loaded;
        }
    }
    prefs.end();
    Debug.printf("%u gespeicherte Parameter geladen.\n", static_cast<unsigned>(loaded));
}

void saveParams()
{
    Preferences prefs;
    prefs.begin("params", false);
    for (size_t i = 0; i < paramCountUsed; ++i)
    {
        const float value = readValue(params[i]);
        if (value == params[i].defaultValue)
        {
            prefs.remove(params[i].name); // Standardwert muss nicht gespeichert werden
        }
        else
        {
            prefs.putFloat(params[i].name, value);
        }
    }
    prefs.end();
    Debug.println("Parameter gespeichert.");
}

void resetParams()
{
    Preferences prefs;
    prefs.begin("params", false);
    prefs.clear();
    prefs.end();
    for (size_t i = 0; i < paramCountUsed; ++i)
    {
        writeValue(params[i], params[i].defaultValue);
    }
    Debug.println("Parameter auf Standardwerte zurückgesetzt.");
}

void printParams(Print &out)
{
    char value[24];
    for (size_t i = 0; i < paramCountUsed; ++i)
    {
        formatValue(params[i], value, sizeof(value));
        out.printf("  %-15s = %-10s %s\n", params[i].name, value, params[i].help);
    }
}

static void appendJsonString(String &out, const char *text)
{
    out += '"';
    for (const char *c = text; *c; ++c)
    {
        if (*c == '"' || *c == '\\')
        {
            out += '\\';
        }
        if (*c == '\n')
        {
            out += "\\n";
            continue;
        }
        out += *c;
    }
    out += '"';
}

void paramsToJson(String &out)
{
    char value[24];
    out += '[';
    for (size_t i = 0; i < paramCountUsed; ++i)
    {
        const ParamEntry &p = params[i];
        if (i > 0)
        {
            out += ',';
        }
        formatValue(p, value, sizeof(value));
        out += "{\"name\":";
        appendJsonString(out, p.name);
        out += ",\"help\":";
        appendJsonString(out, p.help);
        out += ",\"type\":\"";
        out += p.type == ParamType::Float ? "float" : (p.type == ParamType::Int ? "int" : "bool");
        out += "\",\"value\":";
        out += value;
        out += ",\"def\":";
        snprintf(value, sizeof(value), "%g", p.defaultValue);
        out += value;
        snprintf(value, sizeof(value), "%g", p.minValue);
        out += ",\"min\":";
        out += value;
        snprintf(value, sizeof(value), "%g", p.maxValue);
        out += ",\"max\":";
        out += value;
        out += '}';
    }
    out += ']';
}
