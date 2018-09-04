#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "SonoffMQTTOpenhab.h"
#include "sensors.h"
#include "mqttconfig.h"
#include "Networking.h"

void getConfiguration(char* cmd) {
  char delimiter[] = ":";
  char *ptr;
  char temp[50];

  if ( strcmp(cmd,"initialize") == 0 ) {
    snprintf(msg,50,"getconfig:%li", espID);
    client.publish("/openhab/configuration",msg, true);
    confstage = 1;
    goto finish;
  }

  if ( strcmp(cmd,"getVersion") == 0 ) {
    ledFlash(2,100);
    snprintf(temp,50,"Version %s", version);
    client.publish(confTopic ,temp, true);
  }

  if ( strcmp(cmd,"getTopic") == 0 ) {
    ledFlash(2,100);
    snprintf(temp,50,"Topic: %s", sonoffs[1].stateTopic);
    client.publish(confTopic ,temp, true);
  }

  if ( strcmp(cmd,"getIP") == 0 ) {
    ledFlash(2,100);
    IPAddress ip = WiFi.localIP();
    snprintf(temp,50,"IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    client.publish(confTopic ,temp, true);
  }

  if ( strcmp(cmd,"restart") == 0 ) {
    ledFlash(4,100);
    ESP.restart();
  }

  if ( strcmp(cmd,"reconfigure") == 0 ) {
    ledFlash(3,100);
    configured = false;
    confstage = 0;
    goto finish;
  }

  if ( confstage == 1 ) {
    strcpy(msg,cmd);
    ptr = strtok(msg, delimiter);
    if (ptr != NULL) {

      if (strcmp(ptr,"cmdTopic") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(sonoffs[sonoffcount].cmdTopic,ptr);
        Serial.print("Received cmdTopic: ");
        Serial.println(sonoffs[sonoffcount].cmdTopic);
        client.subscribe(sonoffs[sonoffcount].cmdTopic);
        goto finish;
      }

      if (strcmp(ptr,"stateTopic") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(sonoffs[sonoffcount].stateTopic,ptr);
        Serial.print("Received stateTopic: ");
        Serial.println(sonoffs[sonoffcount].stateTopic);
        goto finish;
      }

      if (strcmp(ptr,"sensorTopic") == 0 || strcmp(ptr,"sensorTopic1") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(sensors[sensorcount].sensorTopic1,ptr);
        Serial.print("Received sensorTopic1: ");
        Serial.println(sensors[sensorcount].sensorTopic1);
        goto finish;
      }

      if (strcmp(ptr,"sensorTopic2") == 0 ) {
        ptr = strtok(NULL, delimiter);
        strcpy(sensors[sensorcount].sensorTopic2,ptr);
        Serial.print("Received sensorTopic2: ");
        Serial.println(sensors[sensorcount].sensorTopic2);
        goto finish;
      }


      if (strcmp(ptr,"sensorType") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(sensors[sensorcount].sensorType,ptr);
        Serial.print("Received sensorType: ");
        Serial.println(sensors[sensorcount].sensorType);
        goto finish;
      }

      if (strcmp(ptr,"sensorBlink") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(sensors[sensorcount].sensorBlink,ptr);
        Serial.print("Received sensorBlink: ");
        Serial.println(sensors[sensorcount].sensorBlink);
        goto finish;
      }

      if (strcmp(ptr,"sensorTimer") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sensors[sensorcount].sensorTimer = strtol(temp,&ptr,10);
        Serial.print("Received sensorTimer: ");
        Serial.println(sensors[sensorcount].sensorTimer );
        goto finish;
      }

      if (strcmp(ptr,"sensorPin") == 0 || strcmp(ptr,"sensorPin1") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sensors[sensorcount].sensorPin1 = strtol(temp, &ptr, 10);
        Serial.print("Received sensorPin1: ");
        Serial.println(sensors[sensorcount].sensorPin1);
        //pinMode(sensors[sensorcount].sensorPin1, INPUT);
        goto finish;
      }

      if (strcmp(ptr,"sensorPin2") == 0 ) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sensors[sensorcount].sensorPin2 = strtol(temp, &ptr, 10);
        Serial.print("Received sensorPin2: ");
        Serial.println(sensors[sensorcount].sensorPin2);
        //pinMode(sensors[sensorcount].sensorPin2, INPUT);
        goto finish;
      }

      if (strcmp(ptr,"calibMin") == 0 ) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sensors[sensorcount].calibMin = strtod(temp, &ptr);
        Serial.print("Received calibMin: ");
        Serial.println(sensors[sensorcount].calibMin);
        goto finish;
      }

       if (strcmp(ptr,"calibMax") == 0 ) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sensors[sensorcount].calibMax = strtod(temp, &ptr);
        Serial.print("Received calibMax: ");
        Serial.println(sensors[sensorcount].calibMax);
        goto finish;
      }

      if (strcmp(ptr,"ledPin") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sonoffs[sonoffcount].ledPin = strtol(temp, &ptr, 10);
        Serial.print("Received ledPin: ");
        Serial.println(sonoffs[sonoffcount].ledPin);
        pinMode(sonoffs[sonoffcount].ledPin, OUTPUT);
        goto finish;
      }

      if (strcmp(ptr,"sensor") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sensorcount= strtol(temp, &ptr, 10);
        Serial.print("Received sensor: ");
        Serial.println(sensorcount);
        goto finish;
      }

      if (strcmp(ptr,"sonoff") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sonoffcount= strtol(temp, &ptr, 10);
        Serial.print("Received sonoff: ");
        Serial.println(sonoffcount);
        goto finish;
      }

      if (strcmp(ptr,"sleeptime") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sleeptime = strtol(temp, &ptr, 10);
        Serial.print("Received sleeptime: ");
        Serial.println(sleeptime);
        goto finish;
      }

      if (strcmp(ptr,"EndConfig") == 0) {
        Serial.println("EndConfig");
        confstage = 4;
        goto finish;
      }
    }
  }

  finish:;
}
