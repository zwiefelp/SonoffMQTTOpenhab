// Zwischenstufe: verbindet sich mit dem WLAN aus config.h (wie die Firmware) und wartet auf OTA.
// Ohne mDNS (begin(false), NO_GLOBAL_MDNS) – spart Platz. Relais (GPIO12) bleibt aus,
// die LED (GPIO13) blinkt als Lebenszeichen.
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"

void setup() {
  pinMode(12, OUTPUT); digitalWrite(12, LOW);
  pinMode(13, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("sonoff-ota-stage");
  WiFi.begin(SSID, WIFIPASSWORD);
  ArduinoOTA.setHostname("sonoff-ota-stage");
  ArduinoOTA.begin(false);
}

void loop() {
  ArduinoOTA.handle();
  // Ohne Verbindung nach 10 min neu starten (neuer Versuch)
  if (WiFi.status() != WL_CONNECTED && millis() > 600000UL) ESP.restart();
  digitalWrite(13, (millis() / 500) % 2);
}
