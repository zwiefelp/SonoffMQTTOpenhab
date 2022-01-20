void MQTTdebugPrint(char* msg);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttReconnect();
void checkSensorState(char* stopic, char* msg);
