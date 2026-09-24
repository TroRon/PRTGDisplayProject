# PRTG Display 0.7.0 – USB-Installation

Nur Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151, 1024×600, 16 MB Flash, 8 MB Octal-PSRAM. ESP-IDF 5.5.0. Vier getrennte Images, kein zusammengeführtes Full-Flash-Image. Physischer Test dieses Releases und OTA-/Rollback-Abnahme stehen aus.

Python 3 installieren; Paket vollständig entpacken; USB-Datenkabel anschliessen und seriellen Monitor schliessen. Im entpackten Ordner:

```text
python -m pip install esptool==4.9.0 pyserial==3.5
python flash.py --check
python flash.py
```

Unter Windows gegebenenfalls `py`, unter macOS/Linux `python3` beziehungsweise Python aus einer virtuellen Umgebung verwenden. Der Assistent zeigt Ports, verlangt `FLASH`, schreibt und verifiziert. Anschliessend kann er WLAN/Panel-Token verdeckt per USB abfragen. Aggregator-Adresse und Panelname im Register Panel eintragen. Der PRTG-Key bleibt beim Aggregator.

## Manuell (PORT ersetzen)

```text
python -m esptool --chip esp32s3 --port PORT --baud 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 eaglenet-lcd5b.bin 0xc10000 ota-reset.bin
python -m esptool --chip esp32s3 --port PORT --baud 460800 verify_flash 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 eaglenet-lcd5b.bin 0xc10000 ota-reset.bin
```

Das DIO-Argument entspricht dem generierten Bootloader-Flashheader; die Anwendung konfiguriert Quad/QIO. Nicht auf Verdacht ändern. Bei USB-Fehlern 115200 statt 460800 versuchen.

| Offset | Datei |
|---|---|
| 0x0 | bootloader.bin |
| 0x8000 | partition-table.bin |
| 0x10000 | eaglenet-lcd5b.bin |
| 0xc10000 | ota-reset.bin |

NVS bei 0x9000 bleibt im bisherigen Projektlayout erhalten. Kein `erase_flash`. Die OTA-Auswahl wird zurückgesetzt. Vor 0.6 ist diese vollständige USB-Installation wegen des neuen Layouts nötig. Factory-App und zwei OTA-Slots haben je 4 MiB. Das OTA-Paket enthält nur die Anwendung und signierte Metadaten; niemals Bootloader oder USB-ZIP über OTA installieren.

WLAN/IP, Zeit und API/PRTG-Zustand kontrollieren. Demo-Daten sind ausdrücklich fiktiv. Für eine spätere OTA-Aktualisierung nach Neustart innert 120 Sekunden bestätigen; sonst ist Rollback vorgesehen.

Die vollständige Einrichtungsanleitung steht im Repo unter `Display/README.md`. Technische Konfiguration: `BUILD-INFO.json`, `sdkconfig`, `partitions.csv`. Prüfsummen: `SHA256.txt`.
