/* Version */
const char* version = "1.2 OTA";

/* WiFi Settings */
const char* ssid     = "OpenHAB";
const char* password = "$OpenHAB123";

/* Sonoff Outputs */
struct Sonoff {
  char cmdTopic[50];
  char stateTopic[50];
  int relayPin = 12;    // 12 = D6 - Active high
  int ledPin   = 13;    // 13 = D7 - Active low
  int btnPin   = 0;     //  0 = D3 - Active low
};

struct Sensor {
  char sensorTopic1[50];
  char sensorTopic2[50];
  char sensorType[10];
  char sensorBlink[4];
  int sensorPin1 = 14;       // 14 = D5
  int sensorPin2;
  unsigned long sensorTimer;
};

/* Runtime Variables and Configuration*/
char confTopic[50];
unsigned int confstage;
int sensorcount = 1;
int sonoffcount = 1;
char msg[200];
bool configured = false;
bool bd;
char client_id[20] = "";
unsigned long timer;
