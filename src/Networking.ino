#include <Arduino.h>

/**
 * MQTT callback to process messages
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char spayload[length];
  memcpy(spayload, payload, length);
  spayload[length] = '\0';
  char topicfilter[50] = "";

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
   }
  Serial.println();

  // Examine Configuration Message
  if (strcmp(topic,confTopic) == 0) {
    getConfiguration(spayload);
  }

  // Examine Command Message
  if (strcmp(topic,cmdTopic) == 0) {
    if ( strcmp(spayload,"ON") == 0) {
      setState("ON");
    }
    if( strcmp(spayload,"OFF") == 0 ) {
      setState("OFF");
    }
    if( strcmp(spayload,"TOGGLE") == 0 ) {
      toggleState();
    }
  }
}

/**
 * Attempt connection to MQTT broker and subscribe to command topic
 */
void mqttReconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id)) {
      Serial.println("connected..");
      snprintf(msg,50,"Startup %li - Version %s", id, version);
      ledFlash(2,100);
      client.publish("/openhab/esp8266", msg);
      if ( strlen(cmdTopic) != 0 ) {
        client.subscribe(cmdTopic);
      }
      client.subscribe(confTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

}
