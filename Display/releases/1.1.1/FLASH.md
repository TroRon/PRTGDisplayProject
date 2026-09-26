# Firmware 1.1.1 – Hardware-Release

Nur Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151. 1024×600, 16 MB Flash, 8 MB Octal-PSRAM. ESP-IDF 5.5.0. Noch nicht am realen Gerät abgenommen ; Veröffentlichung freigegeben. Vier getrennte USB-Images, kein zusammengeführter Full-Flash-Dump.

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

Kein erase_flash. Die NVS-Einstellungen bei 0x9000 bleiben erhalten; der USB-Flash setzt nur die OTA-Auswahl zurück. Das signierte `ota/eaglenet-1.1.1.eagleota` enthält nur die Anwendung für einen bestehenden OTA-Slot und darf im vorhandenen Aggregator hochgeladen werden. Auch im öffentlichen OTA-Katalog verfügbar. Nach OTA-Neustart innerhalb von 120 Sekunden bestätigen.

Verwendete Konfiguration: BUILD-INFO.json, sdkconfig, partitions.csv. Prüfsummen: SHA256.txt. Lizenzhinweise: LICENSES/. Keine alten Release-Dateien ersetzen. Erst Hardware prüfen, dann über Veröffentlichung entscheiden.
