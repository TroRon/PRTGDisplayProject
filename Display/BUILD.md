# Hardware-Quellen und Build

Für Anwender genügt das fertige [USB-Paket](releases/prtg-display-1.0.0.zip). Dieses Dokument richtet sich an Entwickler. [Architektur](../docs/ENTWICKLUNG.md) · [Projektstatus](../docs/PROJEKTSTATUS.md).

## Referenzstand

- Board: Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151, 1024 × 600, GT911-Touch.
- ESP-IDF 5.5.0; ESP32-S3, CPU 240 MHz, 16 MB QIO-Flash und 8 MB Octal-PSRAM mit 80 MHz.
- LVGL 8.4.0, Espressif LVGL Adapter 0.5.2, GT911-Komponente 1.1.3, ArduinoJson 6.21.5. Auflösung der weiteren Komponenten in dependencies.lock.
- Herstellerbasis im Vendor-Verzeichnis: Waveshare-Beispiel 08_lvgl_v8_demo; Pins und Timing nicht aus anderen Boards übernehmen.
- Native CMake/Ninja-Kompilierung. Die private Referenztoolchain stammt aus ESP-IDF 5.5.0 mit CMake 3.30.2; PlatformIO allein ist wegen der dokumentierten Adapter-Compilerargumente kein gleichwertig bestätigter Buildweg.

Quellen: [CMake-Projekt](firmware/hardware/CMakeLists.txt), [Komponenten](firmware/hardware/main/idf_component.yml), [Defaults](firmware/hardware/sdkconfig.defaults), [Partitionen](firmware/hardware/partitions.csv). Der historische Kommentar im Defaults-Header nennt 5.2.0; massgeblich für den Release sind 5.5.0 und die [Build-Metadaten](releases/1.0.0/BUILD-INFO.json).

## Eigenen Build vorbereiten

1. ESP-IDF 5.5.0 samt ESP32-S3-Toolchain, Python-Abhängigkeiten, CMake und Ninja installieren und dessen Umgebung aktivieren. IDF_PATH muss darauf zeigen. Einen kurzen lokalen Projektpfad verwenden.
2. Vollständigen Quellbaum behalten: hardware/main verwendet gemeinsame Dateien unter firmware/src und firmware/include. Nicht nur den hardware-Ordner kopieren.
3. Für die Release-Konfiguration die Datei releases/1.0.0/sdkconfig nach firmware/hardware/sdkconfig kopieren. Vorhandene eigene Konfiguration vorher sichern; die Defaults allein sind nicht der vollständige Release-Snapshot.
4. Natives CMake mit Ziel esp32s3 konfigurieren und bauen. Das entspricht dem Projektaufbau; eine frische Installation auf jedem unterstützten Betriebssystem ist damit noch nicht abgenommen.

Beispiel in einer aktivierten ESP-IDF-Umgebung, aus **Display/firmware/hardware**, nachdem Schritt 3 erledigt ist:

```sh
cmake -S . -B build -G Ninja -DIDF_TARGET=esp32s3
cmake --build build
```

Vor einer öffentlichen Weitergabe absolute Entwicklerpfade mit dem im CMake-Projekt vorgesehenen PUBLIC_SOURCE_ROOT-Mapping entfernen und die fertigen Artefakte darauf prüfen. Keine persönlichen sdkconfig-, Build-, managed_components- oder Python-Umgebungen committen. Die Veröffentlichung ist ein eigener freizugebender Schritt.

## Ergebnis und Prüfgrenzen

Ein eigener Build ist noch kein signiertes OTA-Release. Der private Signierschlüssel wird nicht mitgeliefert; bestehende Displays akzeptieren nur Pakete ihres Vertrauensschlüssels. Für einen eigenen Vertriebsschlüssel muss die Vertrauenskette ausdrücklich geplant werden; nicht einfach öffentliche Metadaten umbenennen.

Flash-Dateien und Offsets müssen zur verwendeten Partitionstabelle passen. Release-Pakete enthalten vier getrennte Images und die [genaue Flash-Anleitung](releases/1.0.0/FLASH.md). Kein provisioniertes Full-Flash-Backup veröffentlichen. NVS, OTA-Slots und reservierten Reset-Sektor erhalten.

Prüfen: reproduzierbare Konfiguration, Hashes/Signatur, Display/Touch/Backlight, freier interner Heap und PSRAM, WLAN/NTP/HTTPS, Einstellungen/Neustart, WebAdmin/Hotspot und OTA/Bestätigung/Rollback. Gerätetests getrennt von Build- und Browsertests dokumentieren. Simulator-Firmware nicht auf dieses Board flashen.
