
/* Sonoff Outputs */
struct Sonoff {
  char cmdTopic[50];
  char stateTopic[50];
  int relayPin; // = 12;    // 12 = D6 - Active high
  int ledPin; //   = 13;    // 13 = D7 - Active low
  int btnPin; //   = 0;     //  0 = D3 - Active low
};

struct Sensor {
  char sensorTopic1[50];
  char sensorTopic2[50];
  char sensorType[10];
  char sensorBlink[4];
  int sensorPin1; // = 14;       // 14 = D5
  int sensorPin2;
  unsigned long sensorTimer;
};

