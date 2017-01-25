/* Version */
const char* version = "1.0";

/* Sonoff Outputs */
const int relayPin = 12;  // 12 = D6 - Active high
const int ledPin   = 13; // 13 = D7 - Active low
const int btnPin = 0;     //  0 = D3 - Active low
int sensorPin = 14;       // 14 = D5

/* MQTT Settings */
char cmdTopic[50];
char stateTopic[50];
char confTopic[50];
char sensorTopic[50];
char sensorType[10];

/* Runtime Variables */
unsigned int confstage;
char msg[200];
bool configured = false;
bool bd;
char client_id[20] = "";
unsigned long timer;
unsigned long sensorTimer;
