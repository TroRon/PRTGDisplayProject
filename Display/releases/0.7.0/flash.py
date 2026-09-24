"""Guided flashing for the bundled LCD-5B release. Does not erase NVS."""
import argparse
import hashlib
import importlib.metadata
from pathlib import Path
import subprocess
import sys

IMAGES = ("0x0", "bootloader.bin", "0x8000", "partition-table.bin", "0x10000", "eaglenet-lcd5b.bin", "0xc10000", "ota-reset.bin")

def check_release(folder):
    expected = {}
    for line in (folder / "SHA256.txt").read_text(encoding="utf-8").splitlines():
        digest, name = line.split("  ", 1)
        path = (folder / name).resolve()
        if not path.is_relative_to(folder.resolve()) or len(digest) != 64:
            raise ValueError("Ungültige Prüfsummenliste.")
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest.lower():
            raise ValueError("Prüfsumme stimmt nicht: " + name)
        expected[name] = digest
    if any(name not in expected for name in IMAGES[1::2]):
        raise ValueError("Firmware-Datei fehlt in der Prüfsummenliste.")

def flash_commands(port, baud):
    base = [sys.executable, "-m", "esptool", "--chip", "esp32s3", "--port", port,
            "--baud", str(baud), "--before", "default_reset", "--after", "hard_reset"]
    return [base + ["write_flash", "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "16MB", *IMAGES],
            base + ["verify_flash", *IMAGES]]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="z.B. /dev/cu.usbmodem1101 oder COM4")
    parser.add_argument("--baud", type=int, choices=(115200, 460800), default=460800)
    parser.add_argument("--check", action="store_true", help="Nur Release-Prüfsummen prüfen; kein Gerätezugriff")
    args = parser.parse_args()
    folder = Path(__file__).resolve().parent
    check_release(folder)
    print("Release-Prüfsummen: OK")
    if args.check:
        return
    if not sys.stdin.isatty():
        raise ValueError("Bitte in einem interaktiven Terminal starten.")
    if importlib.metadata.version("esptool") != "4.9.0":
        raise ValueError("Bitte esptool 4.9.0 gemäss Anleitung installieren.")
    from serial.tools import list_ports
    ports = list(list_ports.comports())
    print("Erkannte serielle Anschlüsse:")
    for port in ports:
        print("  " + port.device + "  " + port.description)
    port = args.port or input("Port des angeschlossenen Waveshare-Panels eingeben: ").strip()
    if not port or port not in [entry.device for entry in ports]:
        raise ValueError("Port nicht gefunden. Kabel prüfen und erneut starten.")
    print("Ziel: Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151, 1024x600, 16 MB Flash.")
    print("Port: " + port + ". Vier Images werden geschrieben und danach verifiziert.")
    print("Vorhandene NVS-Anmeldedaten bleiben erhalten. Seriellen Monitor schliessen.")
    if input("Nur für dieses Board: FLASH eingeben zum Starten: ").strip() != "FLASH":
        print("Abgebrochen. Nichts geschrieben.")
        return
    for command in flash_commands(port, args.baud):
        subprocess.run(command, cwd=folder, check=True)
    print("Firmware geschrieben und verifiziert. Das Panel startet neu.")
    if input("WLAN/Panel-Token jetzt per USB einrichten? [j/N]: ").strip().lower() in ("j", "ja"):
        input("Warten, bis die Oberfläche sichtbar ist; dann Enter drücken.")
        subprocess.run([sys.executable, str(folder / "provision.py"), "--port", port], cwd=folder, check=True)

if __name__ == "__main__":
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        sys.exit("Abgebrochen.")
    except subprocess.CalledProcessError:
        sys.exit("Schreiben/Prüfen/Einrichten fehlgeschlagen. Kein vollständiger Erfolg bestätigt; Anleitung prüfen.")
    except (ValueError, OSError, importlib.metadata.PackageNotFoundError) as exc:
        sys.exit(str(exc))
