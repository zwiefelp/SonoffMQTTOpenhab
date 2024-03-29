#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
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
  char spayload[length];
  memcpy(spayload, payload, length);
  spayload[length] = '\0';
  //char topicfilter[50] = "";

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  WebSerial.print("Message arrived [");
  WebSerial.print(topic);
  WebSerial.print("] ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    WebSerial.print((char)payload[i]);
   }
  Serial.println();
  WebSerial.println();

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

/**
 * Attempt connection to MQTT broker and subscribe to command topic
 */
void mqttReconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    WebSerial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id)) {
      Serial.println("connected..");
      WebSerial.println("connected..");
      snprintf(msg,50,"Startup %li - Version %s", espID, version);
      ledFlash(2,100);
      //client.publish("/openhab/esp8266", msg);
      MQTTdebugPrint(msg);
      if ( strlen(sonoffs[1].cmdTopic) != 0 ) {
        client.subscribe(sonoffs[1].cmdTopic);
      }
      client.subscribe(confTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      WebSerial.print("failed, rc=");
      WebSerial.print(client.state());
      WebSerial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void checkSensorState(char* stopic, char* msg) {
    for (int i=1; i<=sensorcount; i++) {
      if (strcmp(stopic,sensors[i].sensorTopic1) == 0 ) {
        strcpy(sensors[i].sensorState1,msg);
        Serial.print("Received sensorState1: ");
        Serial.print(sensors[i].sensorTopic1);
        Serial.print(" = ");
        Serial.println(sensors[i].sensorState1);

        WebSerial.print("Received sensorState1: ");
        WebSerial.print(sensors[i].sensorTopic1);
        WebSerial.print(" = ");
        WebSerial.println(sensors[i].sensorState1);
      }
      if (strcmp(stopic,sensors[i].sensorTopic2) == 0 ) {
        strcpy(sensors[i].sensorState2,msg);
        Serial.print("Received sensorState2: ");
        Serial.print(sensors[i].sensorTopic2);
        Serial.print(" = ");
        Serial.println(sensors[i].sensorState2);

        WebSerial.print("Received sensorState2: ");
        WebSerial.print(sensors[i].sensorTopic2);
        WebSerial.print(" = ");
        WebSerial.println(sensors[i].sensorState2);
      }
    }
}

