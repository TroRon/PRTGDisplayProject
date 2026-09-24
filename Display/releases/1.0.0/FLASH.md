# Firmware 1.0.0 – Hardware-Release, Revision 3

Nur Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151. 1024×600, 16 MB Flash, 8 MB Octal-PSRAM. ESP-IDF 5.5.0. Lokal gebaut und geprüft; die physische Abnahme der Revision 3 steht aus. Vier getrennte USB-Images, kein zusammengeführter Full-Flash-Dump.

Paket vollständig entpacken, USB-Datenkabel anschliessen, seriellen Monitor schliessen. Terminal in diesem Ordner:

```text
python -m pip install esptool==4.9.0 pyserial==3.5
python flash.py --check
python flash.py
```

Unter macOS/Linux gegebenenfalls python3 verwenden, unter Windows py. Der Assistent prüft Hashes, zeigt Ports, verlangt FLASH und verifiziert anschliessend. Ersteinrichtung: WLAN direkt auswählen, SetupPRTGDisplay am Display aktivieren oder per USB einrichten. Details: SETUP.md. Nur WebAdmin setzen: `python provision.py --port PORT --web-only`. WLAN/Panel/OTA bleiben dabei erhalten.

Manuell (PORT ersetzen):

```text
python -m esptool --chip esp32s3 --port PORT --baud 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 eaglenet-lcd5b.bin 0xc10000 ota-reset.bin
python -m esptool --chip esp32s3 --port PORT --baud 460800 verify_flash 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 eaglenet-lcd5b.bin 0xc10000 ota-reset.bin
```

Kein erase_flash. Die NVS-Einstellungen bei 0x9000 bleiben erhalten; der USB-Flash setzt nur die OTA-Auswahl zurück. Das signierte `ota/eaglenet-1.0.0.eagleota` enthält nur die Anwendung für einen bestehenden OTA-Slot und darf im vorhandenen Aggregator hochgeladen werden. Der öffentliche Direktkanal bietet dieses korrigierte Paket an. Nach OTA-Neustart innerhalb von 120 Sekunden bestätigen.

Verwendete Konfiguration: BUILD-INFO.json, sdkconfig, partitions.csv. Prüfsummen: SHA256.txt. Lizenzhinweise: LICENSES/. Keine alten Release-Dateien ersetzen. Nach Installation Display, Touch, WLAN und Ersteinrichtung am Gerät prüfen.

**Bereits 1.0.0 installiert?** Im Direktkanal «Versionen prüfen → 1.0.0 → Neuinstallation» verwenden; «kein neueres Update» ist wegen identischer Versionsnummer möglich. Alternativ dieses vollständige USB-Paket verwenden. Nach OTA innerhalb von 120 Sekunden bestätigen. Ein Aggregator mit bereits gespeichertem ursprünglichem 1.0.0-Paket kann den abweichenden Hash derselben Versionsnummer ablehnen; in diesem Fall Direktkanal oder USB nutzen.

Bei fehlender gespeicherter WLAN-Konfiguration startet SetupPRTGDisplay automatisch und öffnet Webzugang. WebAdmin erlaubt fünfstellige Zahlenpasswörter. WLAN-Passwörter benötigen mindestens acht Zeichen. Weitere Details: SETUP.md. Der Sektor 0xC12000–0xC12FFF bleibt für Reset-Aufträge reserviert.

Neu in R3: WLAN-QR-Code am Display und Captive Portal. QR-Code scannen, mit SetupPRTGDisplay verbinden und als admin anmelden. Falls kein Fenster erscheint: http://192.168.4.1/ öffnen. Anleitung: SETUP.md. Physischer Handy-/AP-Test steht aus.
