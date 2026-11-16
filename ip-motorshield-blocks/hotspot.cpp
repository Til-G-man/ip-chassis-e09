#include "hotspot.h"

#include <Preferences.h>
#include <WiFi.h>

#include "config.h"
#include "wifi-debug.h"

namespace
{
bool connectedAtBoot = false;
String configuredSsid;

bool loadCredentials(String &ssid, String &password)
{
    Preferences prefs;
    if (!prefs.begin("hotspot", true))
    {
        return false;
    }
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("pass", "");
    prefs.end();
    return ssid.length() > 0;
}
} // namespace

bool connectHotspotIfConfigured()
{
    String ssid, password;
    if (!loadCredentials(ssid, password))
    {
        return false; // nichts eingestellt -> nur eigenes WLAN wie bisher
    }
    configuredSsid = ssid;

    Debug.printf("Hotspot: verbinde mit '%s' (max. 10 s)...\n", ssid.c_str());
    WiFi.setHostname(otaHostname);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setTxPower(WIFI_POWER_11dBm); // etwas mehr Reichweite zum Handy
    WiFi.begin(ssid.c_str(), password.c_str());

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
    {
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        connectedAtBoot = true;
        WiFi.setAutoReconnect(true);
        const String ip = WiFi.localIP().toString();
        Debug.printf("Hotspot verbunden. IP: %s  (Kanal %d)\n", ip.c_str(), WiFi.channel());
        Debug.printf("  Werkbank:  http://%s:8080   oder   http://%s.local:8080\n", ip.c_str(), otaHostname);
        return true;
    }

    Debug.println("Hotspot nicht gefunden – weiter nur mit dem eigenen WLAN.");
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_AP);
    return false;
}

bool hotspotConnected()
{
    return connectedAtBoot && WiFi.status() == WL_CONNECTED;
}

void saveHotspotCredentials(const char *ssid, const char *password)
{
    Preferences prefs;
    prefs.begin("hotspot", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    prefs.end();
}

void clearHotspotCredentials()
{
    Preferences prefs;
    prefs.begin("hotspot", false);
    prefs.clear();
    prefs.end();
}

void printWifiStatus(Print &out)
{
    String ssid, password;
    const bool configured = loadCredentials(ssid, password);
    out.printf("Eigenes WLAN: %s  IP %s  (%d verbunden)\n", WiFi.softAPSSID().c_str(),
               WiFi.softAPIP().toString().c_str(), WiFi.softAPgetStationNum());
    if (!configured)
    {
        out.println("Hotspot: nicht eingestellt  (wifi <name> <passwort>, dann reboot)");
        return;
    }
    if (hotspotConnected())
    {
        out.printf("Hotspot: '%s' verbunden, IP %s, Signal %d dBm\n", ssid.c_str(),
                   WiFi.localIP().toString().c_str(), WiFi.RSSI());
        out.printf("  http://%s:8080   oder   http://%s.local:8080\n", WiFi.localIP().toString().c_str(), otaHostname);
    }
    else
    {
        out.printf("Hotspot: '%s' eingestellt, aber nicht verbunden (beim Start nicht gefunden -> reboot zum erneuten Versuch)\n",
                   ssid.c_str());
    }
}
