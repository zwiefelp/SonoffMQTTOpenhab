#include <Arduino.h>

char temp[50];
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

bool tempInit = false;
os_timer_t tempTimer;
float tempReading = 0.0f;

void sensorTemp() {
  if (!tempInit) {
    //void os_timer_setfn(os_timer_t *pTimer, os_timer_func_t *pFunction, void *pArg)
    os_timer_setfn(&tempTimer, sensorTempCallback, NULL);
    //void os_timer_arm(os_timer_t *pTimer, uint32_t milliseconds, bool repeat)
    os_timer_arm(&tempTimer, sensorTimer, true);
    tempInit = true;
  }
}

void sensorTempCallback(void *pArg) {
  snprintf (temp,50,"%.1f", tempReading);
  snprintf (msg, 75, "%s %.1f", sensorTopic, tempReading);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensorTopic, temp, true);
}

os_timer_t pirTimer;
bool pirDetect = false;

void sensorPIR() {
  if (digitalRead(sensorPin) == HIGH && !pirDetect) {
    os_timer_disarm(&pirTimer);
    //void os_timer_setfn(os_timer_t *pTimer, os_timer_func_t *pFunction, void *pArg)
    os_timer_setfn(&pirTimer, sensorPIRCallback, NULL);
    //void os_timer_arm(os_timer_t *pTimer, uint32_t milliseconds, bool repeat)
    os_timer_arm(&pirTimer, sensorTimer, false);
    pirDetect = true;
    snprintf (msg, 75, "%s %s", sensorTopic, "ON");
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensorTopic, "ON", true);
  }

  if (digitalRead(sensorPin) == LOW && pirDetect) {
    pirDetect = false;
  }
}

void sensorPIRCallback(void *pArg) {
  os_timer_disarm(&pirTimer);
  snprintf (msg, 75, "%s %s", sensorTopic, "OFF");
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensorTopic, "OFF", true);
}
