#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoOSCWiFi.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <FastLED.h>
#include <WiFiManager.h>

namespace times {
    uint32_t prev = 0;
    uint32_t current = 0;
    uint32_t delta = 0;

    void update() {
        prev = current;
        current = millis();
        delta = current - prev;
    }
}

constexpr uint8_t ledCount = 24;
constexpr uint8_t dataPin = 4;

bool lightTest = false;
bool allConnection = false;

CRGB ledArray[ledCount]{};

int accu = 1000;

auto blink = [] { ledArray[random(0, ledCount - 1)].setHue(random8()); };
auto resetAndBlink = [] {
    accu = 0;
    blink();
};

auto runLightTest = [] {
    accu += static_cast<int>(times::delta);
    if (accu >= 250) resetAndBlink();
};

void setup() {
    // write your initialization code here
    // Serial.begin(114514);
    Serial.begin(115200);
    WiFiManager wifiManager;
    const auto res = wifiManager.autoConnect("ESP32-Prismatic");
    if (!res) ESP.restart();
    MDNS.begin("prismatic");
    MDNS.addService("osc", "udp", 1337);
    CFastLED::addLeds<WS2812B, dataPin, GRB>(ledArray, ledCount);
    OscWiFi.subscribe(1337, "/blocked", [&](const bool&b) { if (allConnection || b) blink(); });
    OscWiFi.subscribe(1337, "/all", [&](const float&f) { allConnection = f > 0; });
    OscWiFi.subscribe(1337, "/test", [&](const float&f) {
        lightTest = f > 0;
        accu = 1000;
    });
}

void loop() {
    // write your code here
    times::update();
    OscWiFi.update();
    if (lightTest) runLightTest();
    FastLED.show();
    fadeToBlackBy(ledArray, ledCount, 1);
}
