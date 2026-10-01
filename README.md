# SonoffMQTTOpenhab

Firmware für ESP8266-basierte Geräte (z. B. Sonoff S20) zur Anbindung an
[openHAB](https://www.openhab.org/) über MQTT.

Die Besonderheit: Das Gerät bezieht seine **gesamte Konfiguration zur Laufzeit
über MQTT** vom Server. So lässt sich ein einziges Firmware-Image für viele
unterschiedliche Geräterollen (Schalter, Sensoren, Taster …) verwenden, ohne
neu kompilieren zu müssen.

## Funktionsumfang

- Relais-Schalten (ON/OFF/TOGGLE) per MQTT und per physischem Taster
- Konfiguration wird vom openHAB-Server über MQTT gepusht
- Over-the-Air-Updates (ArduinoOTA)
- Optionaler Deep-Sleep-Betrieb (für batteriebetriebene Sensoren)
- Unterstützte Sensortypen:
  - `BTN` – Taster (publiziert ON/OFF)
  - `LED` – LED-Ausgang (folgt einem MQTT-State)
  - `TOGGLE` – Schalteingang
  - `PIR` – Bewegungsmelder (mit Nachlaufzeit)
  - `RF` – 433-MHz-Empfänger (RCSwitch)
  - `DHT` – DHT22 Temperatur/Luftfeuchte
  - `MOISTURE` – analoger Bodenfeuchte-Sensor (mit Kalibrierung)
  - `BAT` – Batteriespannungs-Messung (mit Kalibrierung)
  - `TEMP` – Platzhalter (derzeit ohne Messwertquelle)
  - `BME` – BME280 (im Code auskommentiert)

## Hardware

- Board: `sonoff_s20` (ESP8266, 80 MHz)
- Standard-GPIOs (per MQTT überschreibbar):
  - Relais: GPIO 12 (D6, active high)
  - LED: GPIO 13 (D7, active low)
  - Taster: GPIO 0 (D3, active low)
  - Sensor-Pin: GPIO 14 (D5)

## Build & Flash

Das Projekt nutzt [PlatformIO](https://platformio.org/).

```bash
# Abhängigkeiten installieren und kompilieren
pio run

# Per USB flashen
pio run --target upload

# Per OTA flashen (Gerät muss im Netz erreichbar sein)
pio run --target upload --upload-port <geraete-ip>

# Serielle Ausgabe mitlesen
pio device monitor
```

### Deploy auf den openHAB-Pi

Die Boards hängen im WLAN des Pi (`192.168.1.x`) und sind nur von dort
erreichbar. `deploy/deploy-fw.sh` baut die Firmware, kopiert sie als
`SonoffMQTTOpenhab-<Version>.bin` nach `pi@192.168.20.17:/etc/openhab2/fw` und
flasht sie auf Wunsch per OTA vom Pi aus (`espota.py`):

```bash
deploy/deploy-fw.sh                          # bauen und kopieren
FLASH_IP=192.168.1.109 deploy/deploy-fw.sh   # zusätzlich auf dieses Board flashen
PIO_ENV=esp12e deploy/deploy-fw.sh           # Profil für Module mit 4 MB Flash
```

Build-Profile: `sonoff_s20` (1 MB Flash, Sonoff-Geräte) und `esp12e` (4 MB,
z. B. Wemos D1 mini/NodeMCU). Die Flash-Größe eines Moduls zeigt
`esptool.py --port /dev/ttyUSB0 flash_id`.

Die IP eines Boards liefert `getIP` (siehe unten). Die Version kommt aus
`VERSION` in `src/config.h`.

### Abhängigkeiten

In `platformio.ini` als `lib_deps` hinterlegt:

- `robtillaart/DHTStable`
- `knolleary/PubSubClient`

## Konfiguration

### Compile-Zeit

Vor dem ersten Build muss eine `src/config.h` angelegt werden. Als Vorlage dient
`src/config.h.example`:

```bash
cp src/config.h.example src/config.h
# anschließend SSID und WLAN-Passwort eintragen
```

> **Hinweis:** `src/config.h` ist per `.gitignore` von der Versionierung
> ausgenommen und darf keine echten Zugangsdaten ins Repository bringen.

Einstellbare Defines:

| Define          | Bedeutung                                  |
| --------------- | ------------------------------------------ |
| `SSID`          | WLAN-SSID                                  |
| `WIFIPASSWORD`  | WLAN-Passwort                              |
| `VERSION`       | Versionsstring                             |
| `DHTTYPE`       | DHT-Sensortyp (z. B. `DHT22`)              |
| `SERIAL_DEBUG`  | aktiviert serielle Debug-Ausgabe           |
| `DISPLAY`       | aktiviert optionales SSD1306-Display       |

Der MQTT-Broker ist derzeit fest auf `192.168.1.1:1883` eingestellt
(`SonoffMQTTOpenhab.cpp`).

### Laufzeit (MQTT)

Nach dem Verbinden meldet sich das Gerät beim Broker und fordert seine
Konfiguration an:

1. Gerät publiziert `getconfig:<ESP-ChipID>` auf `/openhab/configuration`
2. Gerät abonniert `/openhab/configuration/<ESP-ChipID>` (Konfig-Topic)
3. Der Server pusht die Konfiguration **zeilenweise** als `schlüssel:wert`
4. Mit `EndConfig` schließt der Server die Konfiguration ab — das Gerät ist
   anschließend betriebsbereit
5. Kommt binnen 30 s keine vollständige Konfiguration (z. B. openHAB nicht
   erreichbar), fordert das Gerät sie erneut an
6. Bei einer späteren Neuverbindung zum Broker abonniert ein bereits
   konfiguriertes Gerät seine Topics neu, ohne die Konfiguration erneut
   anzufordern

**Topics (`<id>` = ESP-ChipID):**

| Topic                          | Richtung        | Zweck                       |
| ------------------------------ | --------------- | --------------------------- |
| `/openhab/configuration`       | Gerät → Server  | Konfigurations-Anforderung  |
| `/openhab/configuration/<id>`  | Server → Gerät  | Konfigurationsdaten         |
| `/openhab/debug/<id>`          | Gerät → Server  | Debug-Ausgaben, u. a. die Startmeldung bei jeder MQTT-Verbindung: `Startup <id> - Version 2.3 OTA: RSSI=-71 MQTTrc=-3 WiFiReason=201` (MQTTrc/WiFiReason nur nach einem Abbruch) |

**Konfigurationsschlüssel** (Auswahl, siehe `mqttconfig.cpp`):

| Schlüssel                        | Bedeutung                                  |
| -------------------------------- | ------------------------------------------ |
| `cmdTopic:<topic>`               | Kommando-Topic für das Relais              |
| `stateTopic:<topic>`             | Status-Topic des Relais                     |
| `sonoff:<n>` / `sensor:<n>`      | aktiven Sonoff- bzw. Sensor-Index wählen    |
| `sensorType:<typ>`               | Sensortyp (BTN, LED, PIR, DHT, …)           |
| `sensorTopic1:<topic>`           | primäres Topic des Sensors                  |
| `sensorTopic2:<topic>`           | sekundäres Topic (z. B. Luftfeuchte)        |
| `sensorPin1:<gpio>`              | GPIO des Sensors                            |
| `sensorPin2:<gpio>`              | zweiter GPIO (z. B. I²C)                     |
| `sensorTimer:<ms>`               | Mess-/Nachlaufintervall in Millisekunden    |
| `sensorBlink:<0\|1>`             | LED-Blinken bei Messung/Empfang             |
| `sensorInitState:<state>`        | Initialzustand                              |
| `calibMin` / `calibMax`          | Kalibrierwerte (MOISTURE, BAT)              |
| `relayPin` / `ledPin` / `btnPin` | GPIO-Belegung des Sonoff                    |
| `sleeptime:<s>`                  | Deep-Sleep-Dauer in Sekunden (0 = aus)      |
| `EndConfig`                      | Konfiguration abschließen                   |

**Laufzeit-Kommandos** (auf dem Konfig-Topic):

| Kommando      | Wirkung                             |
| ------------- | ----------------------------------- |
| `getVersion`  | publiziert die Firmware-Version     |
| `getTopic`    | publiziert das State-Topic          |
| `getIP`       | publiziert die IP-Adresse           |
| `restart`     | startet das Gerät neu               |
| `reconfigure` | fordert die Konfiguration erneut an |

## Bedienung am Gerät

- **Kurzer Tastendruck:** Relais umschalten (ON/OFF)
- **Taster 5 s gedrückt halten:** Neustart des Geräts
- Ohne WLAN oder Broker läuft das Gerät weiter, der Taster schaltet das Relais
  (sobald es einmal konfiguriert war). WLAN und Broker werden im Hintergrund neu
  verbunden.

## 433-MHz-Codes

`mqttcodes.txt` enthält die Zuordnung empfangener 433-MHz-Funkcodes zu
openHAB-Items (für Geräte mit `RF`-Sensor).

> Mehrere `RF`-Empfänger pro Gerät sind nicht möglich: Die `RCSwitch`-Bibliothek
> nutzt einen statischen Interrupt-Handler und unterstützt nur einen Empfänger.

## Projektstruktur

| Datei                   | Inhalt                                                 |
| ----------------------- | ------------------------------------------------------ |
| `SonoffMQTTOpenhab.cpp` | `setup()`/`loop()`, WLAN, OTA, Relais, Sensor-Dispatch |
| `mqttconfig.cpp`        | Parsen der über MQTT empfangenen Konfiguration         |
| `Networking.cpp`        | MQTT-Callback, Reconnect, Sensor-State-Empfang         |
| `sensors.cpp`           | Treiber für die einzelnen Sensortypen                  |
| `types.h`               | Datenstrukturen `Sonoff` und `Sensor`                  |
| `config.h`              | Compile-Zeit-Konfiguration (nicht versioniert)         |
| `RCSwitch.*`            | 433-MHz-Bibliothek                                     |

## Status

In Entwicklung / produktiv im Einsatz.
