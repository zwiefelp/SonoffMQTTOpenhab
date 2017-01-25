/* Version */
const char* version = "1.0";

/* WiFi Settings */
const char* ssid     = "OpenHAB";
const char* password = "$OpenHAB123";

/* Sonoff Outputs */
const int relayPin = 12;  // Active high
const int ledPin   = 13;  // Active low
const int btnPin = 0;     // Active low
int sensorPin = 14;

/* MQTT Settings */
char cmdTopic[50];
char stateTopic[50];
char confTopic[50];
char sensorTopic[50];
char sensorType[10];

unsigned int confstage;
char msg[200];
bool configured = false;
bool bd;
char client_id[20] = "";
unsigned long timer;
unsigned long sensorTimer;
