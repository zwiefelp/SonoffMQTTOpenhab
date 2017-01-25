/*
 * Sonoff Firmware for Openhab with MQTT
 * Peter Zwiefelhofer 2017-01-21
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <config.h>

IPAddress broker(192,168,1,1);          // Address of the MQTT broker
WiFiClient wificlient;
PubSubClient client(wificlient);
long id = ESP.getChipId();

/**
 * Setup
 */
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  configured = false;
  confstage = 0;
  cmdTopic[0]=0;
  snprintf(client_id,20,"client-%li", id);
  snprintf(confTopic,50,"/openhab/configuration/%li",id);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("WiFi begun");

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ledFlash(2,100);
  Serial.println("Proceeding");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    ledFlash(2,100);
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    ledFlash(3,100);
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR   ) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR  ) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR    ) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /* Set up the outputs. LED is active-low */
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, LOW);
  pinMode(btnPin, INPUT_PULLUP);
  pinMode(sensorPin, INPUT);

  /* Prepare MQTT client */
  client.setServer(broker, 1883);
  client.setCallback(mqttCallback);
}

void toggleState() {
    if ( digitalRead(relayPin) == LOW ) {
      setState("ON");
    } else {
      setState("OFF");
    }
}

void setState(char* state) {
  if ( strcmp(state,"ON") == 0 ) {
    digitalWrite(ledPin, LOW);      // LED is active-low, so this turns it on
    digitalWrite(relayPin, HIGH);
  }
  if ( strcmp(state,"OFF") == 0) {
    digitalWrite(ledPin, HIGH);     // LED is active-low, so this turns it off
    digitalWrite(relayPin, LOW);
  }
  snprintf (msg, 75, "%s %s", stateTopic, state);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(stateTopic, state, true);
}

void ledFlash(long rep, long del) {

  int ledstate = digitalRead(ledPin);
  digitalWrite(ledPin, HIGH);
  for (int i = 0; i < rep; i++) {
    delay(del);
    digitalWrite(ledPin, LOW);
    delay(del);
    digitalWrite(ledPin, HIGH);
  }
  digitalWrite(ledPin, ledstate);

}

void btnLoop() {
  if (digitalRead(btnPin) == LOW && bd == false) {
    if (configured) {
      toggleState();
    }
    bd = true;
    timer = millis();
    delay(200);
  }

  if (digitalRead(btnPin) == LOW && bd == true && millis() - timer > 500) {
    ledFlash(4,100);
    ESP.restart();
  }

  if (digitalRead(btnPin) == HIGH && bd == true) {
    bd = false;
    timer = millis();
  }
}

void sensorLoop() {
  if(strcmp(sensorType,"BTN") == 0) {

  }
}

/**
 * Main
 */
void loop() {
  ArduinoOTA.handle();
  if (WiFi.status() != WL_CONNECTED)
  {
    ledFlash(2,100);
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, password);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      mqttReconnect();
    }
  }

  if (!configured) {
    if ( confstage == 0 ) {
      getConfiguration("initialize");
    }
    if ( confstage == 4 ) {
      configured = true;
      setState("OFF");
    }
  }

  if (client.connected()) {
    client.loop();
  }

  btnLoop();

  if (configured) {
    sensorLoop();
  }

}
