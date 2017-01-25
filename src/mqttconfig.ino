#include <Arduino.h>

void getConfiguration(char* cmd) {
  char delimiter[] = ":";
  char *ptr;
  char temp[50];

  if ( strcmp(cmd,"initialize") == 0 ) {
    snprintf(msg,50,"getconfig:%i", id);
    client.publish("/openhab/configuration",msg, true);
    confstage = 1;
    goto finish;
  }

  if ( strcmp(cmd,"getVersion") == 0 ) {
    ledFlash(2,100);
    snprintf(temp,50,"Version %s", version);
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
        strcpy(cmdTopic,ptr);
        Serial.print("Received cmdTopic: ");
        Serial.println(cmdTopic);
        client.subscribe(cmdTopic);
        goto finish;
      }

      if (strcmp(ptr,"stateTopic") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(stateTopic,ptr);
        Serial.print("Received stateTopic: ");
        Serial.println(stateTopic);
        goto finish;
      }

      if (strcmp(ptr,"sensorTopic") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(sensorTopic,ptr);
        Serial.print("Received sensorTopic: ");
        Serial.println(sensorTopic);
        goto finish;
      }

      if (strcmp(ptr,"sensorType") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(sensorType,ptr);
        Serial.print("Received sensorType: ");
        Serial.println(sensorType);
        goto finish;
      }

      if (strcmp(ptr,"sensorTimer") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sensorTimer = atol(temp);
        Serial.print("Received sensorTimer: ");
        Serial.println(sensorTimer);
        goto finish;
      }

      if (strcmp(ptr,"sensorPin") == 0) {
        ptr = strtok(NULL, delimiter);
        strcpy(temp,ptr);
        sensorPin=atoi(temp);
        Serial.print("Received sensorPin: ");
        Serial.println(sensorPin);
        pinMode(sensorPin, INPUT);
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
