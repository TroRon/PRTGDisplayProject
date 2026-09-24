# PRTG Display 0.8.1 – USB-Installation

Nur Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151, 1024×600, 16 MB Flash, 8 MB Octal-PSRAM. ESP-IDF 5.5.0. Vier getrennte Images, kein zusammengeführtes Full-Flash-Image. Physischer Test dieses Releases und OTA-/Rollback-Abnahme stehen aus.

Python 3 installieren; Paket vollständig entpacken; USB-Datenkabel anschliessen und seriellen Monitor schliessen. Im entpackten Ordner:

```text
python -m pip install esptool==4.9.0 pyserial==3.5
python flash.py --check
python flash.py
```

Unter Windows gegebenenfalls `py`, unter macOS/Linux `python3` beziehungsweise Python aus einer virtuellen Umgebung verwenden. Der Assistent zeigt Ports, verlangt `FLASH`, schreibt und verifiziert. Anschliessend kann er WLAN/Panel-Token verdeckt und den OTA-Kanal per USB abfragen. Wahl 2 im Assistenten richtet nur OTA ein und behält WLAN/Token. Wahl 1 richtet WLAN, Panel und optional OTA ein. Wahl 3 richtet nur Aggregator-Adresse und Displayname ein; WLAN/Token und OTA bleiben erhalten. Nachträglich: python provision.py --port PORT --panel-only. Firmware 0.7.2 oder neuer erforderlich. Adresse ohne API-Pfad eingeben, Displayname frei wählen. Nach etwa 20 Sekunden Live-Status prüfen. Der PRTG-Key bleibt beim Aggregator.

## Nur den OTA-Kanal nachträglich einrichten

`python provision.py --port PORT --ota-only` startet die Kanalwahl. Direkt öffentlich wählen, dann Enter für den Projekt-Standard oder eine eigene HTTPS-Manifest-URL eingeben. Kein Download oder Update wird automatisch gestartet. Benötigt Firmware 0.7.1 oder neuer. Einstellungen am Display danach erneut öffnen.

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


## OTA 0.8.1: Update, Downgrade, Neuinstallation

Von 0.7.2 genügt das bisherige «Update prüfen» im öffentlichen Direktkanal und die Installation von 0.8.1. Nach Neustart innert 120 Sekunden «Diese Version behalten» bestätigen. USB ist alternativ möglich, aber nicht für dieses Update erforderlich.

Ab 0.8.0 lädt «Versionen prüfen» die signierten Angebote. Zielversion auswählen und Aktion ausdrücklich bestätigen. Eine ältere Version ist ein Downgrade; dieselbe Version kann neu installiert werden. Vor dem zweiten Bestätigungsklick kann «Abbrechen» gewählt werden. Während Schreiben/Neustart die Versorgung nicht trennen. Vorhandene Einstellungen bleiben gespeichert; ältere Firmware kennt möglicherweise nicht alle Funktionen.

Beim Downgrade auf 0.7.x entfällt dort die Versionsauswahl. Das neueste Manifest bleibt kompatibel und ermöglicht die Rückkehr auf 0.8.1. Aggregator-Version 0.8.0 unterstützt bis zu acht hochgeladene Pakete; ein bisheriger Server liefert weiterhin ein Einzelangebot. Firmware-Versionen, Pinout und Partitionslayout werden vor Installation geprüft. Der physische OTA-/Rollback-Test dieses Releases steht aus.
