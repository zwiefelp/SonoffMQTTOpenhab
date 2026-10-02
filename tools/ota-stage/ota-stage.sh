#!/usr/bin/env bash
# OTA in zwei Stufen fuer Boards, bei denen espota mit "ERROR[4]: Not Enough Space" abbricht
# (alte, grosse Firmware laesst neben sich keinen Platz fuer das volle Image).
#   tools/ota-stage/ota-stage.sh <board-ip> [image-auf-dem-pi]
#   PROBE_ONLY=1 tools/ota-stage/ota-stage.sh <board-ip>   nur freien Platz messen
#
# 1. Freien Platz messen (Update-Einladung ohne Daten - das Board schreibt nichts)
# 2. Zwischen-Image (nur WLAN + OTA, gzip) flashen - danach haengt das Board im WLAN aus
#    src/config.h, also ggf. unter neuer IP im Board-Netz
# 3. Board ueber seine MAC im Board-Netz suchen und das volle Image flashen
# Alles laeuft ueber den Pi, nur von dort sind die Boards erreichbar.
set -euo pipefail

IP="${1:?Board-IP angeben}"
PI="${PI:-pi@192.168.20.17}"
NET="${NET:-192.168.1}"
ESPOTA="${ESPOTA:-/etc/openhab2/bin/espota.py}"
FULL="${2:-}"
HERE="$(cd "$(dirname "$0")" && pwd)"
PIO="${PIO:-$(command -v pio || echo "$HOME/.platformio/penv/bin/pio")}"
[[ "$IP" =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}$ ]] || { echo "keine IPv4-Adresse: $IP" >&2; exit 1; }

echo "== Freien Platz auf $IP messen"
FREE="$(ssh "$PI" python3 - "$IP" <<'PY'
import socket, sys, time
esp = (sys.argv[1], 8266)
def probe(size):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.settimeout(5)
    # Port 9 ist zu: nach "OK" scheitert der Rueckruf des Boards, es verwirft das Update
    s.sendto(("0 9 %d %s\n" % (size, "0" * 32)).encode(), esp)
    try: r = s.recv(64).decode(errors="replace")
    except socket.timeout: sys.exit("keine Antwort von %s:8266" % esp[0])
    finally: s.close()
    time.sleep(3)
    if r.startswith("OK"): return True
    if "Not Enough Space" in r: return False
    sys.exit("unerwartete Antwort: " + r.strip())
lo, hi = 0, 256          # in 4-KB-Sektoren, 1 MB ist sicher zu gross
while hi - lo > 1:
    mid = (lo + hi) // 2
    if probe(mid * 4096): lo = mid
    else: hi = mid
print(lo * 4096)
PY
)"
echo "frei: $FREE Bytes"
[ "${PROBE_ONLY:-0}" = 1 ] && exit 0

echo "== Zwischen-Image bauen"
(cd "$HERE" && "$PIO" run -s)
gzip -9 -c "$HERE/.pio/build/sonoff_s20/firmware.bin" > "$HERE/.pio/stage.bin.gz"
SIZE=$(stat -c %s "$HERE/.pio/stage.bin.gz")
echo "Zwischen-Image: $SIZE Bytes (gzip)"
[ $(( (SIZE + 4095) / 4096 * 4096 )) -le "$FREE" ] || { echo "passt nicht in $FREE Bytes" >&2; exit 1; }

MAC="$(ssh "$PI" "ping -c1 -W2 $IP >/dev/null; ip neigh show $IP" | awk '{for(i=1;i<NF;i++) if($i=="lladdr") print $(i+1)}')"
[ -n "$MAC" ] || { echo "MAC von $IP nicht ermittelbar" >&2; exit 1; }
echo "MAC: $MAC"

if [ -z "$FULL" ]; then
  FULL="$(ssh "$PI" "ls -t /etc/openhab2/fw/SonoffMQTTOpenhab-*-sonoff_s20.bin | head -1")"
fi
echo "volles Image: $FULL"

echo "== Stufe 1: Zwischen-Image"
scp -q "$HERE/.pio/stage.bin.gz" "$PI:/tmp/ota-stage.bin.gz"
ssh "$PI" "python3 '$ESPOTA' -i '$IP' -p 8266 -f /tmp/ota-stage.bin.gz"

echo "== Board im Netz $NET.x suchen (bis 3 min)"
sleep 20   # Neustart abwarten, sonst passt evtl. noch der alte Eintrag
NEW="$(ssh "$PI" "end=\$((\$(date +%s) + 180)); until [ \$(date +%s) -gt \$end ]; do
  for i in \$(seq 2 253); do ping -c1 -W1 $NET.\$i >/dev/null 2>&1 & done; wait
  ip=\$(ip neigh show to $NET.0/24 | grep -i ' $MAC ' | grep -v FAILED | awk '{print \$1}' | head -1)
  [ -n \"\$ip\" ] && { echo \$ip; break; }
done")"
[ -n "$NEW" ] || { echo "Board nicht gefunden - laeuft das Zwischen-Image (LED blinkt)? WLAN-Reichweite?" >&2; exit 1; }
echo "gefunden: $NEW"

echo "== Stufe 2: $FULL"
ssh "$PI" "python3 '$ESPOTA' -i '$NEW' -p 8266 -f '$FULL'; rm -f /tmp/ota-stage.bin.gz"
echo "Fertig - das Board meldet sich mit 'Startup ... - Version ...' unter $NEW"
