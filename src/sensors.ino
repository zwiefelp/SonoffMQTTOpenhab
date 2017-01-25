#include <Arduino.h>

unsigned long btnTimer;
bool btndwn = false;
char* btnstate = "OFF";

void sensorBTN() {
  if (digitalRead(sensorPin) == LOW && btndwn == false) {
    // Short Press
    btnToggleState();
    btndwn = true;
    btnTimer = millis();
    delay(200);
  }

  if (digitalRead(sensorPin) == LOW && btndwn == true && millis() - btnTimer > 500) {
    // Long Press
  }

  if (digitalRead(sensorPin) == HIGH && btndwn == true) {
    // Btn Release
    btndwn = false;
    btnTimer = millis();
  }
}

void btnToggleState() {
  if (strcmp(btnstate,"ON") == 0)  {
    btnstate = "OFF";
  } else {
    btnstate = "ON";
  }

  snprintf (msg, 75, "%s %s", sensorTopic, btnstate);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensorTopic, btnstate, true);
}
