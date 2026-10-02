# OTA in zwei Stufen

Für Boards, bei denen das OTA-Update mit `ERROR[4]: Not Enough Space` abbricht: Die laufende
alte Firmware ist so groß, dass neben ihr kein Platz für das volle Image bleibt
(Layout `1m256`: Sketch-Bereich 748 KB, eine 557-KB-Firmware lässt nur 204 KB frei).

```
tools/ota-stage/ota-stage.sh <board-ip> [image-auf-dem-pi]
PROBE_ONLY=1 tools/ota-stage/ota-stage.sh <board-ip>     # nur freien Platz messen
```

1. **Platz messen** – Update-Einladungen mit verschiedenen Größen, ohne Daten. Das Board
   prüft nur den Platz und verwirft das Update (der Rückruf auf Port 9 scheitert), es wird
   nichts geschrieben. Bei der alten Firmware blinkt die LED kurz.
2. **Zwischen-Image** (`src/main.cpp`): nur WLAN + ArduinoOTA, ohne mDNS, gzip-komprimiert
   (~207 KB). Der Bootloader entpackt es beim Neustart (ab Core 2.7). Kann die alte Firmware
   kein gzip, lehnt sie das Image beim ersten Block ab – ebenfalls ohne Schaden.
3. **Volles Image** – das Board wird über seine MAC im Board-Netz gesucht (das Zwischen-Image
   nutzt das WLAN aus `src/config.h`), dann wird das neueste
   `SonoffMQTTOpenhab-*-sonoff_s20.bin` aus `/etc/openhab2/fw` geflasht.

⚠️ Danach hängt das Board im WLAN aus `src/config.h` – bei einem Board aus einem anderen WLAN
vorher die Reichweite bedenken; ohne Verbindung bleibt nur USB. Das Relais ist nach dem
Neustart aus.

Bisher eingesetzt: Sonoff_Stecker2 (8704174), 2026-10-02.
