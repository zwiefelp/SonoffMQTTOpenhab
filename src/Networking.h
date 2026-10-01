void MQTTdebugPrint(char* msg);
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool mqttReconnect();
void subscribeTopics();
void checkSensorState(char* stopic, char* msg);
