#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "dht.h"
#include <stdlib.h>
#include <os_type.h>
#include "config.h"
#include "types.h"
#include "SonoffMQTTOpenhab.h"
#include "sensors.h"
#include <Wire.h>                                                       // required by BME280 library
#include <BME280_t.h>                                                   // import BME280 template library

char temp[50];
unsigned long btnTimer;
bool btndwn = false;
char* btnstate = (char *)"OFF";
bool btnInit = false;
//ADC_MODE(ADC_VCC);

void drawText(char* text, int line) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10*line);
  display.println(text);
  display.display(); 
}

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
    btnstate = (char *)"OFF";
  } else {
    btnstate = (char *)"ON";
  }

  snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, btnstate);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensors[nr].sensorTopic1, btnstate, true);
}

char* togglestate = (char *)"OFF";
bool toggleon = false;
bool toggleInit = false;

void sensorTOGGLE(int nr) {
  if (!toggleInit) {
    pinMode(sensors[nr].sensorPin1, INPUT);
    toggleInit = true;
  }

  if (digitalRead(sensors[nr].sensorPin1) == LOW && toggleon == false) {
    toggleon = true;
    togglestate = (char *)"ON";
    snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, btnstate);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic1, btnstate, true);
  }

  if (digitalRead(sensors[nr].sensorPin1) == HIGH && toggleon == false) {
    toggleon = false;
    togglestate = (char *)"OFF";
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic1, btnstate, true);
  }
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
    sensorTempCallback(&nr);
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
      if ( lastval != newval ) {
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

      } else if (millis() - lastrec > sensors[nr].sensorTimer) {
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
      }
    }
    rfRec.resetAvailable();
  }
}

// DHT22 Temperature and Humidity Sensor

bool dhtInit = false;
unsigned long dhtTimer1 = 0;

void sensorDHT(int nr) {
  if (!dhtInit) {
    Serial.print("initialize DHT22 - Pin=");
    Serial.print(sensors[nr].sensorPin1);
    Serial.print(" Timer=");
    Serial.println(sensors[nr].sensorTimer);
    //pinMode(sensors[nr].sensorPin1,INPUT_PULLUP);

    dhtInit = true;
    sensorDHTCallback(nr);
  }

  if (millis() - dhtTimer1 >= sensors[nr].sensorTimer) {
    sensorDHTCallback(nr);
    dhtTimer1 = millis();
  }
}

void sensorDHTCallback(int nr) {
  double dhtTempReading = 0;
  double dhtHumReading = 0;
  dht DHT;
  int chk;

  if (strcmp(sensors[nr].sensorBlink,"1") == 0) {
    ledFlash(1,50);
  }
  Serial.println("Read DHT Sensor");
  chk = DHT.read22(sensors[nr].sensorPin1);

  if ( chk != DHTLIB_OK) {
    Serial.println("DHT not OK! Try again read DHT Sensor");
    chk = DHT.read22(sensors[nr].sensorPin1);
  }

  if ( chk == DHTLIB_OK ) {
    dhtTempReading = DHT.temperature;
    snprintf (temp,50,"%d.%01d", (int)dhtTempReading, abs((int)(dhtTempReading*10)%10));
    snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, temp);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic1, temp, true);

    dhtHumReading=DHT.humidity;
    if ( dhtHumReading <= 100 && dhtHumReading >= 0 ) {
      snprintf (temp,50,"%d.%01d", (int)dhtHumReading, abs((int)(dhtHumReading*10)%10));
      snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic2, temp);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish(sensors[nr].sensorTopic2, temp, true);
    }
  } else {
    Serial.println("Failed reading DHT Sensor");
  }
}

// BME280 Temperature, Humidity and Pressure Sensor

bool BMEInit = false;
bool BMEError = false;
unsigned long BMETimer1 = 0;
BME280<> BMESensor; 
int sdaPin = 2;
int sclPin = 1;

void sensorBME(int nr) {
  if (!BMEInit && !BMEError) {
    Serial.print("initialize BME - Pin1=");
    Serial.print(sensors[nr].sensorPin1);
    Serial.print(" Pin2=");
    Serial.print(sensors[nr].sensorPin2);

    Serial.print(" Timer=");
    Serial.println(sensors[nr].sensorTimer);
    //pinMode(sensors[nr].sensorPin1,INPUT_PULLUP);
    
    // initialize I2C that connects to sensor
    sdaPin = sensors[nr].sensorPin1;
    sclPin = sensors[nr].sensorPin2;
    Wire.begin(sdaPin, sclPin);
    //Wire.begin(sensors[nr].sensorPin1, sensors[nr].sensorPin2);
    //Wire.begin(2,1);
    scani2c();

    if (! BMESensor.begin()) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      BMEError = true;
    } else {
      BMEInit = true;
      sensorBMECallback(nr);
    }
  }

  if (millis() - BMETimer1 >= sensors[nr].sensorTimer && !BMEError) {
    sensorBMECallback(nr);
    BMETimer1 = millis();
  }
}

void sensorBMECallback(int nr) {
  double BMETempReading = 0;
  double BMEHumReading = 0;
  double BMEPressReading = 0;

  if (strcmp(sensors[nr].sensorBlink,"1") == 0) {
    ledFlash(1,50);
  }
  Serial.println("Read BME Sensor");
  BMESensor.refresh();

  Serial.print("Read Temperature: ");
  Serial.println(BMESensor.temperature);
  BMETempReading = BMESensor.temperature;
  snprintf (temp,50,"%d.%01d", (int)BMETempReading, abs((int)(BMETempReading*10)%10));
  snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, temp);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensors[nr].sensorTopic1, temp, true);

  Serial.print("Read Humidity: ");
  Serial.println(BMESensor.humidity);
  BMEHumReading=BMESensor.humidity;
  if ( BMEHumReading <= 100 && BMEHumReading >= 0 ) {
    snprintf (temp,50,"%d.%01d", (int)BMEHumReading, abs((int)(BMEHumReading*10)%10));
    snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic2, temp);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic2, temp, true);
  }

  Serial.print("Read Pressure: ");
  Serial.println(BMESensor.pressure);
  BMEPressReading=BMESensor.pressure;
}

// Scan i2C

void scani2c()
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++)
  {
    // The i2c scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println(" !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("Done.\n");
  }

}


// Moisture Sensor
bool moistInit = false;
unsigned long moistTimer1 = 0;

void sensorMoist(int nr) {
  if (!moistInit) {
    Serial.print("initialize Moisture Sensor - Pin=");
    Serial.print(sensors[nr].sensorPin1);
    Serial.print(" Timer=");
    Serial.println(sensors[nr].sensorTimer);
    //pinMode(sensors[nr].sensorPin1,INPUT_PULLUP);
    moistInit = true;
    sensorMoistCallback(nr);
  }

  if (millis() - moistTimer1 >= sensors[nr].sensorTimer) {
    sensorMoistCallback(nr);
    moistTimer1 = millis();
  }
}

void sensorMoistCallback(int nr) {
  long int moistValue = 0;
  #define VALUEWET 400
  #define VALUEDRY 800
  int percValue = 0 ;
  double valueDry = VALUEDRY;
  double valueWet = VALUEWET;
  
  if (strcmp(sensors[nr].sensorBlink,"1") == 0) {
    ledFlash(1,50);
  }

  if (sensors[nr].calibMin) {
    valueDry = sensors[nr].calibMin;
  }

  if (sensors[nr].calibMax) {
    valueWet = sensors[nr].calibMax;
  }

  moistValue = analogRead(sensors[nr].sensorPin1);
  snprintf (temp,50,"%li", moistValue);
  snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, temp);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensors[nr].sensorTopic1, temp, true);

  if (sensors[nr].sensorTopic2) {
    percValue = int((moistValue - valueDry) / (valueWet - valueDry) * 100);
    if (percValue > 100) { percValue = 100; }
    if (percValue < 0 ) { percValue = 0; }
    snprintf (temp,50,"%i", percValue);
    snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic2, temp);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic2, temp, true);
    
    #ifdef DISPLAY
    if (usedisplay == 1 ) {
      display.clearDisplay();
      drawText("Moisture Sensor",0);
      snprintf (temp,50,"Raw: %li", moistValue);
      drawText(temp,1);
      snprintf (temp,50,"Perc: %i %%", percValue);
      drawText(temp,2);
    }
    #endif
  }
}

// Battery Voltage Sensor
bool batInit = false;
unsigned long batTimer1 = 0;

void sensorBat(int nr) {
  if (!batInit) {
    //ADC_MODE(ADC_VCC);
    Serial.print("initialize Battery Sensor - Pin=");
    Serial.print(sensors[nr].sensorPin1);
    Serial.print(" Timer=");
    Serial.println(sensors[nr].sensorTimer);
    pinMode(sensors[nr].sensorPin1,INPUT);
    batInit = true;
    sensorBatCallback(nr);
  }

  if (millis() - batTimer1 >= sensors[nr].sensorTimer) {
    sensorBatCallback(nr);
    batTimer1 = millis();
  }
}

void sensorBatCallback(int nr) {
  #define VCC_ADJ 1.096
  #define MIN_VOLTS 3.0
  #define MAX_VOLTS 4.2
  int batValue = 0;
  float volt = 0.0;
  int raw = 0;
  double minVolts = MIN_VOLTS;
  double maxVolts = MAX_VOLTS;

  if (sensors[nr].calibMin) {
    minVolts = sensors[nr].calibMin;
  }

  if (sensors[nr].calibMax) {
    maxVolts = sensors[nr].calibMax;
  }

  if (strcmp(sensors[nr].sensorBlink,"1") == 0) {
    ledFlash(1,50);
  }
  Serial.println("Read Battery Sensor");

  //volt = (float)ESP.getVcc()* VCC_ADJ
  raw = analogRead(sensors[nr].sensorPin1);
  volt = (raw / 1023.0) * MAX_VOLTS;
  batValue = int((volt - minVolts) / (maxVolts - minVolts) * 100);
  if (batValue < 0) { batValue = 0;}
  if (batValue > 100) { batValue = 100; }
  if (volt < 0 ) { volt = 0.0; }

  snprintf (temp,50,"%i", batValue);
  snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic1, temp);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(sensors[nr].sensorTopic1, temp, true);

  if (sensors[nr].sensorTopic2) {
    snprintf (temp,50,"%.4f", volt);
    snprintf (msg, 75, "%s %s", sensors[nr].sensorTopic2, temp);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(sensors[nr].sensorTopic2, temp, true);

    //snprintf (temp,50,"%li", raw);
    //client.publish("/openhab/in/bett_raw/state", temp, true);
  }
}
