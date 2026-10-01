#!/usr/bin/env bash
# Baut die Firmware und kopiert sie auf den openHAB-Host.
#   deploy/deploy-fw.sh [user@host]
#   DEST_DIR=/pfad deploy/deploy-fw.sh       anderes Zielverzeichnis
#   NO_BUILD=1 deploy/deploy-fw.sh           vorhandenes firmware.bin verwenden
#   FLASH_IP=192.168.1.109 deploy/deploy-fw.sh  danach per OTA auf dieses Board flashen
#   PIO_ENV=esp12e deploy/deploy-fw.sh       Build-Profil (sonoff_s20 = 1 MB, esp12e = 4 MB)
#
# Die Boards haengen im WLAN des Pi (192.168.1.x) und sind nur von dort erreichbar.
# Das Flashen laeuft deshalb auf dem Pi mit espota.py (wie /etc/openhab2/bin/esp_upload.sh).
#
# Auf dem Ziel landet die Datei als SonoffMQTTOpenhab-<Version>-<Profil>.bin (Version aus
# src/config.h, Leerzeichen -> "_"), z. B. SonoffMQTTOpenhab-2.2_OTA-sonoff_s20.bin.
set -euo pipefail

TARGET="${1:-pi@192.168.20.17}"
DEST_DIR="${DEST_DIR:-/etc/openhab2/fw}"
ENV_NAME="${PIO_ENV:-sonoff_s20}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ESPOTA="${ESPOTA:-/etc/openhab2/bin/espota.py}"
PIO="${PIO:-$(command -v pio || echo "$HOME/.platformio/penv/bin/pio")}"

cd "$ROOT"
if [ -n "${FLASH_IP:-}" ] && ! [[ "$FLASH_IP" =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}$ ]]; then
  echo "FLASH_IP ist keine IPv4-Adresse: $FLASH_IP" >&2; exit 1
fi
[ -f src/config.h ] || { echo "src/config.h fehlt (Vorlage: src/config.h.example)" >&2; exit 1; }

# Nur die VERSION-Zeile lesen - config.h enthaelt auch die WLAN-Zugangsdaten
# (config.h hat CRLF-Zeilenenden -> \r entfernen, sonst landet es im Dateinamen)
VERSION="$(tr -d '\r' < src/config.h | sed -n 's/^#define VERSION "\(.*\)"/\1/p')"
[ -n "$VERSION" ] || { echo "VERSION in src/config.h nicht gefunden" >&2; exit 1; }

if [ -n "$(git status --porcelain -- src lib platformio.ini)" ]; then
  echo "Hinweis: nicht committete Aenderungen in src/, lib/ oder platformio.ini" >&2
fi

if [ "${NO_BUILD:-0}" != 1 ]; then
  "$PIO" run -e "$ENV_NAME"
  # Der Build aendert die versionierte Pruefsumme - Arbeitsbaum sauber halten
  git checkout -- .pio/build/project.checksum 2>/dev/null || true
fi

BIN=".pio/build/$ENV_NAME/firmware.bin"
[ -f "$BIN" ] || { echo "$BIN fehlt - erst bauen" >&2; exit 1; }
NAME="SonoffMQTTOpenhab-${VERSION// /_}-$ENV_NAME.bin"

scp "$BIN" "$TARGET:$DEST_DIR/$NAME"
echo "Uebertragen: $NAME ($(git rev-parse --short HEAD), $(git rev-parse --abbrev-ref HEAD)) -> $TARGET:$DEST_DIR"

if [ -n "${FLASH_IP:-}" ]; then
  echo "Flashe $NAME per OTA auf $FLASH_IP (ueber $TARGET) ..."
  ssh "$TARGET" "python3 '$ESPOTA' -i '$FLASH_IP' -p 8266 -f '$DEST_DIR/$NAME'"
  echo "Erfolgreich: $FLASH_IP - nach dem Neustart meldet das Board 'Startup ... - Version $VERSION'"
fi
