#include <Arduino.h>

char temp[50];
unsigned long btnTimer;
bool btndwn = false;
char* btnstate = "OFF";

void sensorBTN(int nr) {
  if (digitalRead(sensors[nr].sensorPin1) == LOW && btndwn == false) {
    // Short Press
    btnToggleState(nr);
    btndwn = true;
    btnTimer = millis();
    delay(200);
  }

  if (digitalRead(sensors[nr].sensorPin1) == LOW && btndwn == true && millis() - btnTimer > 500) {
    // Long Press
  }

  if (digitalRead(sensors[nr].sensorPin1) == HIGH && btndwn == true) {
    // Btn Release
    btndwn = false;
    btnTimer = millis();
  }
}

void btnToggleState(int nr) {
  if (strcmp(btnstate,"ON") == 0)  {
    btnstate = "OFF";
  } else {
    btnstate = "ON";
  }

  snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, btnstate);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensors[nr].sensorTopic1, btnstate, true);
}

bool tempInit = false;
os_timer_t tempTimer;
float tempReading = 0.0f;

void sensorTemp(int nr) {
  if (!tempInit) {
    Serial.print("initialize Temp - Pin=");
    Serial.print(sensors[nr].sensorPin1);
    Serial.print(" Timer=");
    Serial.println(sensors[nr].sensorTimer);

    //void os_timer_setfn(os_timer_t *pTimer, os_timer_func_t *pFunction, void *pArg)
    os_timer_setfn(&tempTimer, sensorTempCallback, &nr);
    //void os_timer_arm(os_timer_t *pTimer, uint32_t milliseconds, bool repeat)
    os_timer_arm(&tempTimer, sensors[nr].sensorTimer, true);
    tempInit = true;
  }
}

void sensorTempCallback(void *pArg) {
  int nr = *((int *) pArg);
  snprintf (temp,50,"%.1f", tempReading);
  snprintf (msg, 75, "%s %.1f", sensors[nr].sensorTopic1, tempReading);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensors[nr].sensorTopic1, temp, true);
}

os_timer_t pirTimer;
bool pirDetect = false;

void sensorPIR(int nr) {
  if ( digitalRead(sensors[nr].sensorPin1) == HIGH && !pirDetect ) {
    os_timer_disarm(&pirTimer);
    snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, "ON");
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic1, "ON", true);
    pirDetect = true;
  }
  if ( digitalRead(sensors[nr].sensorPin1) == LOW && pirDetect) {
    os_timer_disarm(&pirTimer);
    //void os_timer_setfn(os_timer_t *pTimer, os_timer_func_t *pFunction, void *pArg)
    os_timer_setfn(&pirTimer, sensorPIRCallback, &nr);
    //void os_timer_arm(os_timer_t *pTimer, uint32_t milliseconds, bool repeat)
    os_timer_arm(&pirTimer, sensors[nr].sensorTimer, false);
    pirDetect = false;
  }
}

void sensorPIRCallback(void *pArg) {
  os_timer_disarm(&pirTimer);
  int nr = *((int *) pArg);
  snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, "OFF");
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensors[nr].sensorTopic1, "OFF", true);
  pirDetect = false;
}

bool rfInit = false;
RCSwitch rfRec = RCSwitch();
unsigned long lastval = 0;
unsigned long newval = 0;
unsigned long lastrec = 0;
char rfmsg[20];

void sensorRF(int nr) {
  if (!rfInit) {
    Serial.print("initialize RF - Pin=");
    Serial.print(sensors[nr].sensorPin1);
    Serial.print(" Timer=");
    Serial.println(sensors[nr].sensorTimer);
    rfRec.enableReceive(sensors[nr].sensorPin1);
    rfInit = true;
  }
  if (rfRec.available()) {

    int value = rfRec.getReceivedValue();

    if (value == 0) {
      Serial.print("Unknown encoding");
    } else {
      newval = rfRec.getReceivedValue();
      if ( lastval != newval || millis() - lastrec > sensors[nr].sensorTimer) {
        snprintf (rfmsg, 20, "%li", newval);
        snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, rfmsg);
        Serial.print("Publish message: ");
        Serial.println(msg);
        client.publish(sensors[nr].sensorTopic1, rfmsg, true);
        if (strcmp(sensors[nr].sensorBlink,"1") == 0) {
          ledFlash(1,50);
        }
        lastval = newval;
        lastrec = millis();
      } else {
        if (millis() - lastrec > sensors[nr].sensorTimer) {
          lastrec = 0;
        }
      }
    }
    rfRec.resetAvailable();
  }

}
