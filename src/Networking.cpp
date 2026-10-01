#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "SonoffMQTTOpenhab.h"
#include "Networking.h"
#include "mqttconfig.h"

/**
 * MQTT callback to process messages
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char spayload[length + 1];
  memcpy(spayload, payload, length);
  spayload[length] = '\0';
  //char topicfilter[50] = "";

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
   }
  Serial.println();

  // Examine Configuration Message
  if (strcmp(topic,confTopic) == 0) {
    getConfiguration(spayload);
  }

  // Examine Sensor State Messages
  if (confstage == 4) {
    checkSensorState(topic, spayload);
  }


  // Examine Command Message
  if (strcmp(topic,sonoffs[1].cmdTopic) == 0) {
    if ( strcmp(spayload,"ON") == 0) {
      setState((char *)"ON");
    }
    if( strcmp(spayload,"OFF") == 0 ) {
      setState((char *)"OFF");
    }
    if( strcmp(spayload,"TOGGLE") == 0 ) {
      toggleState();
    }
  }
}

/**
 * Print Debug Output to Serial and/or MQTT
**/
void MQTTdebugPrint(char* msg) {
  if (client.connected()) {
    client.publish(debugTopic, msg);
  }
}

#define MQTT_RETRY_MS 5000
static unsigned long lastMqttAttempt = 0;

/**
 * Alle Topics des Boards abonnieren. Nach jeder neuen Verbindung noetig: der Broker
 * vergisst die Abos, sonst kaemen danach keine Sensorzustaende mehr an.
 */
void subscribeTopics() {
  client.subscribe(confTopic);
  if ( strlen(sonoffs[1].cmdTopic) != 0 ) {
    client.subscribe(sonoffs[1].cmdTopic);
  }
  for (int i = 1; i <= sensorcount; i++) {
    if ( strlen(sensors[i].sensorTopic1) != 0 ) {
      client.subscribe(sensors[i].sensorTopic1);
    }
    if ( strlen(sensors[i].sensorTopic2) != 0 ) {
      client.subscribe(sensors[i].sensorTopic2);
    }
  }
}

/**
 * Ein Verbindungsversuch zum MQTT-Broker, hoechstens alle MQTT_RETRY_MS.
 * Blockiert nicht, damit Taster und OTA auch ohne Broker funktionieren.
 * Liefert true, wenn die Verbindung mit diesem Aufruf hergestellt wurde.
 */
bool mqttReconnect() {
  if (lastMqttAttempt != 0 && millis() - lastMqttAttempt < MQTT_RETRY_MS) {
    return false;
  }
  lastMqttAttempt = millis();
  Serial.print("Attempting MQTT connection...");
  if (!client.connect(client_id)) {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    return false;
  }
  Serial.println("connected..");
  // Startmeldung mit Empfang und Grund des letzten Abbruchs (Schluessel=Wert nach ":",
  // der Debug-Tab zeigt sie als Felder): MQTTrc = client.state() beim Abbruch
  // (-4 Timeout, -3 Verbindung verloren …), WiFiReason = SDK-Trennungsgrund (z. B. 201 kein AP)
  int n = snprintf(msg, sizeof(msg), "Startup %li - Version %s: RSSI=%d", espID, version, WiFi.RSSI());
  if (lastMqttState != MQTT_STATE_NONE && n < (int)sizeof(msg)) {
    n += snprintf(msg + n, sizeof(msg) - n, " MQTTrc=%d", lastMqttState);
  }
  if (lastWifiReason != 0 && n < (int)sizeof(msg)) {
    snprintf(msg + n, sizeof(msg) - n, " WiFiReason=%d", lastWifiReason);
  }
  ledFlash(2,100);
  MQTTdebugPrint(msg);
  subscribeTopics();
  return true;
}

void checkSensorState(char* stopic, char* msg) {
    for (int i=1; i<=sensorcount; i++) {
      if (strcmp(stopic,sensors[i].sensorTopic1) == 0 ) {
        strlcpy(sensors[i].sensorState1, msg, sizeof(sensors[i].sensorState1));
        Serial.print("Received sensorState1: ");
        Serial.print(sensors[i].sensorTopic1);
        Serial.print(" = ");
        Serial.println(sensors[i].sensorState1);
      }
      if (strcmp(stopic,sensors[i].sensorTopic2) == 0 ) {
        strlcpy(sensors[i].sensorState2, msg, sizeof(sensors[i].sensorState2));
        Serial.print("Received sensorState2: ");
        Serial.print(sensors[i].sensorTopic2);
        Serial.print(" = ");
        Serial.println(sensors[i].sensorState2);
      }
    }
}

