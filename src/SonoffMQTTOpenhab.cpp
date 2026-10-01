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
#include <SPI.h>
//#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include "DHTStable.h"
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

#define WIFI_SETUP_MS 20000    // so lange wartet setup() hoechstens aufs WLAN
#define WIFI_RETRY_MS 300000   // neues WiFi.begin() hoechstens alle 5 min (nur wenn das SDK aufgab)
#define CONFIG_RETRY_MS 30000  // ohne (vollstaendige) Konfiguration erneut anfragen
#define BTN_RESTART_MS 5000    // Taster so lange halten → Neustart

char confTopic[50];
char debugTopic[50];
unsigned int confstage;
unsigned long confRequestTs;
int lastWifiReason = 0;                  // Grund der letzten WLAN-Trennung (SDK-Code), 0 = keine
int lastMqttState = MQTT_STATE_NONE;     // client.state() beim letzten MQTT-Abbruch
WiFiEventHandler wifiDisconnectHandler;
int sensorcount;
int sonoffcount;
int usedisplay;
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

#ifdef DISPLAY
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  #define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif 

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
  usedisplay = 0;

  #ifdef SERIAL_DEBUG
    Serial.begin(115200);
    snprintf(msg,20,"Booting V%s", version);
    Serial.println();
    Serial.println(msg);
    snprintf(msg,20,"ESP ID %li", espID);
    Serial.println(msg);
  #endif

  /* Set up the outputs. LED is active-low */
  pinMode(sonoffs[1].ledPin, OUTPUT);
  pinMode(sonoffs[1].relayPin, OUTPUT);
  digitalWrite(sonoffs[1].ledPin, HIGH);
  digitalWrite(sonoffs[1].relayPin, LOW);
  pinMode(sonoffs[1].btnPin, INPUT_PULLUP);
  //pinMode(sensors[1].sensorPin, INPUT);

  ledFlash(4,100);

  configured = false;
  confstage = 0;
  sonoffs[1].cmdTopic[0]=0;
  snprintf(client_id,20,"client-%li", espID);
  snprintf(confTopic,50,"/openhab/configuration/%li",espID);
  snprintf(debugTopic,50,"/openhab/debug/%li", espID);
  //config[confTopic] = confTopic;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& e) {
    lastWifiReason = e.reason;
  });
  WiFi.begin(ssid, password);
  Serial.println("WiFi begun");
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println("...");

  // Nicht endlos warten: ohne WLAN laeuft loop() trotzdem (Taster) und verbindet spaeter
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_SETUP_MS) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    ledFlash(2,100);
  } else {
    Serial.println("Connection Failed! Continuing without WiFi");
  }

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

  /* Prepare MQTT client */
  client.setServer(broker, 1883);
  client.setCallback(mqttCallback);
}

/**
 * WLAN-Verbindung pruefen, ohne zu blockieren. Das SDK verbindet selbst neu
 * (setAutoReconnect). Ein erneutes WiFi.begin() bricht einen laufenden Verbindungsaufbau
 * ab – bei schlechtem Empfang kaeme das Board so nie durch. Deshalb nur, wenn das SDK
 * aufgegeben hat (WL_CONNECT_FAILED / WL_NO_SSID_AVAIL), und hoechstens alle WIFI_RETRY_MS.
 */
void wifiLoop() {
  static bool wasConnected = WiFi.status() == WL_CONNECTED;
  static unsigned long lastAttempt = millis();
  static wl_status_t lastStatus = WL_IDLE_STATUS;

  wl_status_t status = WiFi.status();
  if (status != lastStatus) {
    Serial.printf("WiFi status %d -> %d\n", lastStatus, status);
    lastStatus = status;
  }

  if (status == WL_CONNECTED) {
    if (!wasConnected) {
      Serial.println("WiFi connected");
      ledFlash(2,100);
    }
    wasConnected = true;
    return;
  }

  wasConnected = false;
  bool gaveUp = status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL;
  if (gaveUp && millis() - lastAttempt >= WIFI_RETRY_MS) {
    lastAttempt = millis();
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, password);
  }
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

  if (digitalRead(sonoffs[1].btnPin) == LOW && bd == true && millis() - timer > BTN_RESTART_MS) {
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
    if(strcmp(sensors[i].sensorType,"LED") == 0) {
      sensorLED(i);
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
    /*
    if(strcmp(sensors[i].sensorType,"BME") == 0) {
      sensorBME(i);
    }
    */
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
  wifiLoop();

  // Grund eines MQTT-Abbruchs merken, er geht mit der naechsten Startmeldung raus
  static bool mqttWasConnected = false;
  if (mqttWasConnected && !client.connected()) {
    lastMqttState = client.state();
  }
  mqttWasConnected = client.connected();

  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    // Konfiguration nur neu anfordern, solange keine vorliegt. Frueher wurde confstage nach
    // jeder Neuverbindung auf 0 gesetzt und erreichte bei konfigurierten Boards nie wieder 4 –
    // dann lief checkSensorState() nicht mehr.
    if (mqttReconnect() && !configured) {
      confstage = 0;
    }
  }

  if (!configured) {
    if ( confstage == 0 && client.connected() ) {
      getConfiguration((char *)"initialize");
    }
    // Keine oder unvollstaendige Antwort (openHAB nicht erreichbar, EndConfig verloren)
    if ( confstage == 1 && millis() - confRequestTs > CONFIG_RETRY_MS ) {
      MQTTdebugPrint((char *)"No complete configuration received, requesting again");
      confstage = 0;
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
