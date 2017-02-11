#include <Arduino.h>

char temp[50];
unsigned long btnTimer;
bool btndwn = false;
char* btnstate = "OFF";
bool btnInit = false;

void sensorBTN(int nr) {
  if (!btnInit) {
    pinMode(sensors[nr].sensorPin1, INPUT);
    btnInit = true;
  }
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
    pinMode(sensors[nr].sensorPin1, INPUT);
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
bool pirInit = false;

void sensorPIR(int nr) {
  if (!pirInit) {
    pinMode(sensors[nr].sensorPin1, INPUT);
    pirInit = true;
  }

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
    pinMode(sensors[nr].sensorPin1, INPUT);
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

bool dhtInit = false;
os_timer_t dhtTimer;
unsigned long dhtTimer1 = millis();

void sensorDHT(int nr) {
  if (!dhtInit) {
    Serial.print("initialize DHT22 - Pin=");
    Serial.print(sensors[nr].sensorPin1);
    Serial.print(" Timer=");
    Serial.println(sensors[nr].sensorTimer);
    //pinMode(sensors[nr].sensorPin1,INPUT_PULLUP);

    //void os_timer_setfn(os_timer_t *pTimer, os_timer_func_t *pFunction, void *pArg)
    //os_timer_setfn(&dhtTimer, sensorDHTCallback, &nr);
    //void os_timer_arm(os_timer_t *pTimer, uint32_t milliseconds, bool repeat)
    //os_timer_arm(&dhtTimer, sensors[nr].sensorTimer, true);
    dhtInit = true;
  }
  if (millis() - dhtTimer1 >= sensors[nr].sensorTimer) {
    sensorDHTCallback(nr);
    dhtTimer1 = millis();
  }
}

//void sensorDHTCallback(void *pArg) {
void sensorDHTCallback(int nr) {
  double dhtTempReading = 0;
  double dhtHumReading = 0;
  dht DHT;
  //int nr = *((int *) pArg);

  if (strcmp(sensors[nr].sensorBlink,"1") == 0) {
    ledFlash(1,50);
  }
  int chk = DHT.read22(sensors[nr].sensorPin1);
/*
  Serial.print("DHT on Pin=");
  Serial.print(sensors[nr].sensorPin1);
  Serial.print(" read, Check=");
  Serial.println(chk);
*/

  if ( chk == DHTLIB_OK ) {
//    Serial.print("Temperature=");
//    Serial.println(DHT.temperature, 1);
    dhtTempReading = DHT.temperature;
    snprintf (temp,50,"%d.%01d", (int)dhtTempReading, abs((int)(dhtTempReading*10)%10));
    snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, temp);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic1, temp, true);

//    Serial.print("Humidity=");
//    Serial.println(DHT.humidity, 1);
    dhtHumReading=DHT.humidity;
    snprintf (temp,50,"%d.%01d", (int)dhtHumReading, abs((int)(dhtHumReading*10)%10));
    snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic2, temp);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic2, temp, true);
  }
}
