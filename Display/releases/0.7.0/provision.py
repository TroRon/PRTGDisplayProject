"""Local USB configuration. Never print or persist credentials."""
import argparse
import getpass
import json
import time
import sys

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    args = parser.parse_args()
    if not sys.stdin.isatty():
        raise ValueError("Bitte in einem interaktiven Terminal ausführen.")
    import serial
    ssid = input("WLAN-Name: ").strip()
    password = getpass.getpass("WLAN-Passwort (verdeckt): ")
    token = getpass.getpass("Panel-Token, kein PRTG-Key (verdeckt): ").strip()
    ntp = input("NTP-Server [pool.ntp.org]: ").strip() or "pool.ntp.org"
    if not (1 <= len(ssid.encode()) <= 32 and 8 <= len(password.encode()) <= 63
            and 16 <= len(token) <= 256 and all(33 <= ord(c) <= 126 for c in token)
            and 1 <= len(ntp) <= 127 and all(c.isascii() and (c.isalnum() or c in '.-') for c in ntp)):
        raise ValueError("Ungültige Eingabe; nichts übertragen.")
    payload = ("CONFIG " + json.dumps(dict(ssid=ssid, password=password, token=token, ntp=ntp), ensure_ascii=False) + "\n").encode()
    with serial.Serial(args.port, 115200, timeout=1) as port:
        port.reset_input_buffer()
        port.write(payload)
        port.flush()
        deadline = time.monotonic() + 45
        while time.monotonic() < deadline:
            line = port.readline()
            if b"CONFIG_SAVE_FAILED" in line or b"CONFIG_REJECTED" in line:
                raise RuntimeError("Das Panel hat die Konfiguration abgelehnt oder konnte sie nicht speichern.")
            if b"CONFIG_SAVED" in line:
                print("Gespeichert. Das Panel verbindet sich neu. WLAN/NTP/HTTP-Status auf dem Display prüfen.")
                return
    raise TimeoutError("Keine Speicherbestätigung. Display prüfen; kein Erfolg bestätigt.")

if __name__ == "__main__":
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        sys.exit("Abgebrochen.")
    except Exception:
        # Do not surface library exceptions that could include serial data.
        sys.exit("Einrichtung fehlgeschlagen. Port, Eingaben und Panel-Status prüfen; seriellen Monitor vorher schliessen.")
