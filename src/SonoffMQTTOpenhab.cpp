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
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "SonoffMQTTOpenhab.h"
#include "sensors.h"
#include "mqttconfig.h"
#include "Networking.h"

extern "C" {
  #include "user_interface.h"
}

char confTopic[50];
char debugTopic[50];
unsigned int confstage;
int sensorcount;
int sonoffcount;
char msg[200];
bool configured;
bool bd;
char client_id[20];
unsigned long timer;
bool sleep;
unsigned long sleeptime;
long espID;
struct Sonoff sonoffs[10];
struct Sensor sensors[10];

/* Version */
const char* version = VERSION;

/* WiFi Settings */
const char* ssid     = SSID;
const char* password = WIFIPASSWORD;

IPAddress broker(192,168,1,1);          // Address of the MQTT broker
WiFiClient wificlient;
PubSubClient client(wificlient);

/**
 * Setup
 */
void setup() {
  espID = ESP.getChipId();
  sensorcount = 1;
  sonoffcount = 1;
  configured = false;
  sleep = false;
  sleeptime = 0;

  #ifdef SERIAL_DEBUG
    Serial.begin(115200);
    snprintf(msg,20,"Booting V%s", version);
    Serial.println(msg);
    snprintf(msg,20,"ESP ID %li", espID);
    Serial.println(msg);
  #endif

  configured = false;
  confstage = 0;
  sonoffs[1].cmdTopic[0]=0;
  snprintf(client_id,20,"client-%li", espID);
  snprintf(confTopic,50,"/openhab/configuration/%li",espID);
  snprintf(debugTopic,50,"/openhab/debug/%li", espID);
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
      setState((char *)"ON");
    } else {
      setState((char *)"OFF");
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
    if(strcmp(sensors[i].sensorType,"BME") == 0) {
      sensorBME(i);
    }
    if(strcmp(sensors[i].sensorType,"MOISTURE") == 0) {
      sensorMoist(i);
    }
    if(strcmp(sensors[i].sensorType,"BAT") == 0) {
      sensorBat(i);
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
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, password);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
    ledFlash(2,100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      mqttReconnect();
    }
  }

  if (!configured) {
    if ( confstage == 0 ) {
      getConfiguration((char *)"initialize");
    }
    if ( confstage == 4 ) {
      configured = true;
      setState((char *)"OFF");
    }
  }

  if (client.connected()) {
    client.loop();
  }

  if (sleep) {
    // Going to sleep (GPIO16 (D0) must be connected to RST)
    snprintf(msg, 50, "Deep sleep for %li secs", sleeptime);
    MQTTdebugPrint(msg);

    Serial.print("Going into deep sleep for ");
    Serial.print(sleeptime * 1e6);
    Serial.println(" microseconds");
    delay(100);
    ESP.deepSleep(sleeptime * 1e6); // 20e6 is 20e6 microseconds
  }

  btnLoop();

  if (configured) {
    sensorLoop();
    if (sleeptime > 0 ) {
      sleep = true;
    }
  }

}
