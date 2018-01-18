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
#include <RCSwitch.h>
#include "dht.h"
//#include <map>
// #include <user_config.h>
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

extern "C" {
  #include "user_interface.h"
}

#define DHTTYPE DHT22

IPAddress broker(192,168,1,1);          // Address of the MQTT broker
WiFiClient wificlient;
PubSubClient client(wificlient);
long id = ESP.getChipId();

struct Sensor sensors[10];
struct Sonoff sonoffs[10];
//std::map <string, char> config;

/**
 * Setup
 */
void setup() {
  Serial.begin(115200);
  Serial.println("Booting V1.0");
  configured = false;
  confstage = 0;
  sonoffs[1].cmdTopic[0]=0;
  snprintf(client_id,20,"client-%li", id);
  snprintf(confTopic,50,"/openhab/configuration/%li",id);
  //config[confTopic] = confTopic;

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.println("WiFi begun");
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println("...");

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
  pinMode(sonoffs[1].ledPin, OUTPUT);
  pinMode(sonoffs[1].relayPin, OUTPUT);
  digitalWrite(sonoffs[1].ledPin, HIGH);
  digitalWrite(sonoffs[1].relayPin, LOW);
  pinMode(sonoffs[1].btnPin, INPUT_PULLUP);
  //pinMode(sensors[1].sensorPin, INPUT);

  /* Prepare MQTT client */
  client.setServer(broker, 1883);
  client.setCallback(mqttCallback);
}

void toggleState() {
    if ( digitalRead(sonoffs[1].relayPin) == LOW ) {
      setState("ON");
    } else {
      setState("OFF");
    }
}

void setState(char* state) {
  if ( strcmp(state,"ON") == 0 ) {
    digitalWrite(sonoffs[1].ledPin, LOW);      // LED is active-low, so this turns it on
    digitalWrite(sonoffs[1].relayPin, HIGH);
  }
  if ( strcmp(state,"OFF") == 0) {
    digitalWrite(sonoffs[1].ledPin, HIGH);     // LED is active-low, so this turns it off
    digitalWrite(sonoffs[1].relayPin, LOW);
  }
  snprintf (msg, 75, "%s %s", sonoffs[1].stateTopic, state);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sonoffs[1].stateTopic, state, true);
}

void ledFlash(long rep, long del) {
  int ledstate = digitalRead(sonoffs[1].ledPin);
  digitalWrite(sonoffs[1].ledPin, HIGH);
  for (int i = 0; i < rep; i++) {
    delay(del);
    digitalWrite(sonoffs[1].ledPin, LOW);
    delay(del);
    digitalWrite(sonoffs[1].ledPin, HIGH);
  }
  digitalWrite(sonoffs[1].ledPin, ledstate);
}

void btnLoop() {
  if (digitalRead(sonoffs[1].btnPin) == LOW && bd == false) {
    if (configured) {
      toggleState();
    }
    bd = true;
    timer = millis();
    delay(200);
  }

  if (digitalRead(sonoffs[1].btnPin) == LOW && bd == true && millis() - timer > 500) {
    ledFlash(4,100);
    ESP.restart();
  }

  if (digitalRead(sonoffs[1].btnPin) == HIGH && bd == true) {
    bd = false;
    timer = millis();
  }
}

void sensorLoop() {
  for (int i=1; i<=sensorcount; i++) {

    if(strcmp(sensors[i].sensorType,"BTN") == 0) {
      sensorBTN(i);
    }
    if(strcmp(sensors[i].sensorType,"TOGGLE") == 0) {
      sensorTOGGLE(i);
    }
    if(strcmp(sensors[i].sensorType,"TEMP") == 0) {
      sensorTemp(i);
    }
    if(strcmp(sensors[i].sensorType,"PIR") == 0) {
      sensorPIR(i);
    }
    if(strcmp(sensors[i].sensorType,"RF") == 0) {
      sensorRF(i);
    }
    if(strcmp(sensors[i].sensorType,"DHT") == 0) {
      sensorDHT(i);
    }

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
