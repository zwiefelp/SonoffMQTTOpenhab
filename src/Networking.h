void MQTTdebugPrint(char* msg);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttReconnect();
