//#include <Adafruit_SSD1306.h>
/* Runtime Variables and Configuration*/
extern char confTopic[50];
extern char debugTopic[50];
extern unsigned int confstage;
extern int sensorcount;
extern int sonoffcount;
extern char msg[200];
extern bool configured;
extern bool bd;
extern char client_id[20];
extern unsigned long timer;
extern bool sleep;
extern unsigned long sleeptime;
extern long espID;
extern const char* version;
extern int usedisplay;

extern struct Sonoff sonoffs[10];
extern struct Sensor sensors[10]; 

extern WiFiClient wificlient;
extern PubSubClient client;
//extern AsyncWebServer server;
//extern Adafruit_SSD1306 display;

void ledFlash(long rep, long del);
void toggleState();
void setState(char* state);
