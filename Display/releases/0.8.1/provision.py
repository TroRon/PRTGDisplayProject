"""Local USB configuration. Never print or persist credentials."""
import argparse
import getpass
import json
import time
import sys
import re

DEFAULT_MANIFEST = 'https://raw.githubusercontent.com/TroRon/PRTGDisplayProject/main/Display/ota/manifest.json'

def panel_settings():
    origin = input('Aggregator-Adresse (https://hostname, ohne API-Pfad): ').strip().removesuffix('/')
    name = input('Displayname [PRTG Display]: ').strip() or 'PRTG Display'
    if not (origin.startswith('https://') and origin[8:] and len(origin.encode()) < 256
            and all(33 <= ord(c) <= 126 and c not in '/@?#\\' for c in origin[8:])
            and len(name.encode()) <= 48 and all(ord(c) >= 32 and ord(c) != 127 for c in name)):
        raise ValueError('Ungültige Panel-Eingabe; nichts übertragen.')
    return dict(origin=origin, name=name)

def send_panel(port, panel):
    send_config(port, 'PANEL_CONFIG', panel, b'PANEL_CONFIG_SAVED', (b'PANEL_CONFIG_REJECTED', b'CONFIG_REJECTED'))
    print('Gespeichert: Aggregator-Adresse und Displayname. Offene Einstellungen am Display erneut öffnen.')

def ota_settings():
    choice = input('OTA-Kanal: [0] unverändert, [1] Aggregator, [2] direkt öffentlich [0]: ').strip() or '0'
    if choice == '0': return None
    if choice not in ('1', '2'): raise ValueError('Ungültige Kanalwahl; nichts übertragen.')
    url = DEFAULT_MANIFEST
    if choice == '2':
        url = input('Manifest-URL [Enter = PRTGDisplayProject]: ').strip() or DEFAULT_MANIFEST
    host, separator, path = url[8:].partition('/')
    if not (url.startswith('https://') and host and separator and path and len(url.encode()) < 384
            and all(33 <= ord(c) <= 126 and c not in '@#?\\' for c in url)):
        raise ValueError('Ungültige HTTPS-Manifest-Adresse; nichts übertragen.')
    return dict(direct=choice == '2', url=url)

def send_config(port, command, value, success, failures):
    port.reset_input_buffer()
    port.write((command + ' ' + json.dumps(value, ensure_ascii=False) + '\n').encode())
    port.flush()
    deadline = time.monotonic() + 45
    while time.monotonic() < deadline:
        words = re.sub(rb'\x1b\[[0-9;]*m', b'', port.readline()).split()
        if words and words[-1] in failures:
            raise RuntimeError('Konfiguration abgelehnt; Firmware-Version und Panel-Status prüfen.')
        if words and words[-1] == success: return
    raise TimeoutError('Keine Speicherbestätigung; kein Erfolg bestätigt.')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--ota-only", action="store_true", help="Nur OTA-Kanal/URL ändern; WLAN und Token behalten")
    mode.add_argument("--panel-only", action="store_true", help="Nur Aggregator-Adresse/Displayname ändern; WLAN und OTA behalten")
    args = parser.parse_args()
    if not sys.stdin.isatty():
        raise ValueError("Bitte in einem interaktiven Terminal ausführen.")
    import serial
    if args.panel_only:
        panel = panel_settings()
        with serial.Serial(args.port, 115200, timeout=1) as port:
            send_panel(port, panel)
        print('WLAN und OTA unverändert. Live-Daten sollten bei erreichbarem Aggregator innerhalb von etwa 20 Sekunden erscheinen.')
        return
    if args.ota_only:
        ota = ota_settings()
        if ota is None:
            print('Unverändert. Nichts übertragen.'); return
        with serial.Serial(args.port, 115200, timeout=1) as port:
            send_config(port, 'OTA_CONFIG', ota, b'OTA_CONFIG_SAVED', (b'OTA_CONFIG_REJECTED', b'CONFIG_REJECTED'))
        print('OTA-Kanal gespeichert. WLAN und Token bleiben unverändert. Einstellungen am Display erneut öffnen.')
        return
    ssid = input("WLAN-Name: ").strip()
    password = getpass.getpass("WLAN-Passwort (verdeckt): ")
    token = getpass.getpass("Panel-Token, kein PRTG-Key (verdeckt): ").strip()
    ntp = input("NTP-Server [pool.ntp.org]: ").strip() or "pool.ntp.org"
    if not (1 <= len(ssid.encode()) <= 32 and 8 <= len(password.encode()) <= 63
            and 16 <= len(token) <= 256 and all(33 <= ord(c) <= 126 for c in token)
            and 1 <= len(ntp) <= 127 and all(c.isascii() and (c.isalnum() or c in '.-') for c in ntp)):
        raise ValueError("Ungültige Eingabe; nichts übertragen.")
    panel = panel_settings()
    ota = ota_settings()
    with serial.Serial(args.port, 115200, timeout=1) as port:
        # Check new protocol support before changing Wi-Fi on an older device.
        send_panel(port, panel)
        send_config(port, 'CONFIG', dict(ssid=ssid, password=password, token=token, ntp=ntp), b'CONFIG_SAVED', (b'CONFIG_SAVE_FAILED', b'CONFIG_REJECTED'))
        print('Gespeichert: WLAN/Panel-Token. Verbindungsstatus auf dem Display prüfen.')
        if ota is not None:
            send_config(port, 'OTA_CONFIG', ota, b'OTA_CONFIG_SAVED', (b'OTA_CONFIG_REJECTED', b'CONFIG_REJECTED'))
            print('OTA-Kanal gespeichert. Einstellungen am Display erneut öffnen. Kein Update automatisch gestartet.')

if __name__ == "__main__":
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        sys.exit("Abgebrochen.")
    except Exception:
        # Do not surface library exceptions that could include serial data.
        sys.exit("Einrichtung fehlgeschlagen. Port, Eingaben und Panel-Status prüfen; seriellen Monitor vorher schliessen.")
