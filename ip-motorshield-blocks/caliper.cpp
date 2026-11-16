#include "caliper.h"

#include "commands.h"
#include "params.h"
#include "wifi-debug.h"

namespace
{
int caliperClockPin = 11;
int caliperDataPin = 12;
int caliperTimeoutMs = 150; // Pause zwischen Paketen, die ein Paket-Ende markiert

volatile uint32_t rawBits = 0; // bis zu 24 empfangene Bits (LSB zuerst reingeschoben)
volatile uint8_t bitCount = 0;
volatile uint32_t lastEdgeUs = 0;

float lastMm = NAN;

// Wird bei jeder fallenden Flanke am Takt-Pin aufgerufen (Pegelwandler invertiert
// das Messschieber-Signal, darum entspricht das einer steigenden Flanke am
// Messschieber selbst - dort ist laut Protokoll das Datenbit gueltig).
void IRAM_ATTR onClockEdge()
{
    if (bitCount < 24)
    {
        const int bit = digitalRead(caliperDataPin) == LOW ? 1 : 0; // invertiert
        rawBits |= (static_cast<uint32_t>(bit) << bitCount);
        ++bitCount;
    }
    lastEdgeUs = micros();
}

void decodePacket()
{
    if (bitCount == 24)
    {
        const uint32_t bits = rawBits;
        const uint8_t b0 = bits & 0xFF;
        const uint8_t b1 = (bits >> 8) & 0xFF;
        const uint8_t b2 = (bits >> 16) & 0xFF;
        long magnitude = (static_cast<long>(b2 & 0x1F) << 16) | (static_cast<long>(b1) << 8) | b0;
        if (b2 & 0x20)
            magnitude = -magnitude;
        lastMm = magnitude / 200.0f;
    }
    rawBits = 0;
    bitCount = 0;
}

void cmdCaliper(const Args &args)
{
    const uint32_t duration = args.getInt(0, 3000);
    const uint32_t start = millis();
    do
    {
        Debug.printf("Messschieber: %.3f mm\n", caliperMm());
    } while (millis() - start < duration && waitMs(300));
}
} // namespace

void initCaliper()
{
    pinMode(caliperClockPin, INPUT_PULLUP);
    pinMode(caliperDataPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(caliperClockPin), onClockEdge, FALLING);
}

void serviceCaliper()
{
    noInterrupts();
    const uint32_t edgeUs = lastEdgeUs;
    const uint8_t bits = bitCount;
    interrupts();
    if (bits > 0 && micros() - edgeUs > static_cast<uint32_t>(caliperTimeoutMs) * 1000UL)
    {
        noInterrupts();
        decodePacket();
        interrupts();
    }
}

float caliperMm() { return lastMm; }

void registerCaliper()
{
    addParam("caliperClockPin", &caliperClockPin, 0, 48, "Messschieber: ESP32-Pin Takt (Neustart noetig)");
    addParam("caliperDataPin", &caliperDataPin, 0, 48, "Messschieber: ESP32-Pin Daten (Neustart noetig)");
    addParam("caliperTimeoutMs", &caliperTimeoutMs, 10, 1000, "Messschieber: Paketpause in ms");
    addCommand("caliper", "caliper [ms] - Messschieber-Wert live anzeigen", cmdCaliper);
}
