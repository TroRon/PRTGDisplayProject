# Architektur und Entwicklung

[Projektstand](PROJEKTSTATUS.md) unterscheidet veröffentlichte Software, Kandidaten und physische Abnahme. [WORKFLOW.md](../WORKFLOW.md) regelt die ausdrückliche Freigabe jeder Veröffentlichung.

## Komponenten und Datenfluss

| Bereich | Verantwortung |
|---|---|
| PRTG Sensoren | Vorlagen für Datenerfassung und PRTG-Ausgabe; keine Display-Zugangsdaten |
| DockerAggregator/src/prtg.mjs | Lesender Zugriff auf PRTG |
| config.mjs / health.mjs | Validierung, Zustandsmodell, Zeit-/Kanalprüfung |
| runtime.mjs | Abfragerunden und Austausch der Konfiguration ohne veraltete Rückmeldungen |
| server.mjs | Health-API, Token-/TCP-Abgrenzung und Weiterleitung an Admin/Firmware |
| admin-store.mjs / admin.mjs / admin.* | Persistente Konfiguration, Verwaltungs-API und Browseroberfläche |
| firmware.mjs | Signatur-/Board-/Hashprüfung, Firmware-Archiv und Katalog |
| admin-auth.mjs, ab 1.1.0 | Passwort-Hash, Sitzungen, Ablauf und Widerruf |
| github-firmware.mjs, ab 1.1.0 | Begrenzter HTTPS-Abruf und Prüfung des signierten GitHub-Katalogs |
| Display/firmware/hardware | ESP-IDF-Projekt mit offiziellen Waveshare-Treibern und LVGL-Oberfläche |
| Display/releases / Display/ota | Gebaute USB-/OTA-Artefakte und öffentliche Angebote |

Kein direkter Hostzugriff vom Display. PRTG-Rohantworten und Secret-Dateien sind keine öffentlichen API-Endpunkte. Fehlende oder veraltete Werte werden als unbekannt modelliert; Statusreihenfolge kritisch → unbekannt → Warnung → OK.

## Schnittstellen

| Pfad | Zweck / Zugang |
|---|---|
| GET /healthz | Prozessprobe; kein PRTG-Gesundheitsnachweis |
| GET /api/v1/health | Snapshot schema_version 1, Panel-Bearer-Token |
| GET /api/v1/firmware/catalog.json | Signierter Versionskatalog, Panel-Token |
| GET /api/v1/firmware/manifest.json | Höchstes Angebot für ältere Clients, Panel-Token |
| GET /api/v1/firmware/<sha256>.bin | Passendes Image, Panel-Token |
| PUT /api/v1/firmware/upload | Signiertes Paket, OTA-Administrator-Token |
| /admin und /api/admin/* | Browserverwaltung mit Host-/Origin-/Zugangsprüfung |
| /updates | Weiterhin technischer Firmware-Upload |

Health enthält Modus, Gesamtstatus, Datenzeitpunkte, Vollständigkeit, Kategorien, Systeme/Sensoren und Alarme. Neue API-Felder und Grössen gegen den Display-Parser prüfen. Keine Nullmesswerte für fehlende Kanäle erfinden. Deaktivierte Systeme werden nicht abgefragt; leere Bereiche erscheinen in der API deaktiviert und werden am Display ausgeblendet.

Ab 1.1.0: POST /api/admin/login tauscht die Anmeldedaten gegen ein zufälliges Sitzungstoken; POST logout widerruft es; PUT password ändert das Passwort. GET github liest signierte Angebote, POST github/import übernimmt die per SHA256 ausgewählte Version. Schreibzugriffe benötigen passenden Origin. Diese Routen stehen ab Aggregator 1.1.0 zur Verfügung.

## Konfiguration und Persistenz

Ursprüngliche JSON-Konfiguration und Secrets bleiben read-only gemountet. Die erste Web-Speicherung erzeugt admin/settings.json mit Revision, Konfiguration und optionalem Ersatz-PRTG-Token. Atomare Dateiersetzung, Revisionsprüfung und synchroner Runtime-Wechsel verhindern stilles Überschreiben beziehungsweise Rückkehr alter Poll-Ergebnisse. Eine laufende Abfragerunde darf enden; ihre alten Ergebnisse werden nicht in die neue Konfiguration übernommen.

Ab 1.1.0 separates password.json mit Salt und scrypt-Hash. Sessions nur im Speicher, nach Restart ungültig. Originales OTA-Token bleibt technischer Vollzugang. Keine Sitzungs-/Passwort-/PRTG-Daten in Browser-Logs, URL, Screenshots oder öffentliche Artefakte schreiben.

## Hardware-Build und Release

Referenz: ESP-IDF **5.5.0**, ESP32-S3, 16 MB Quad-Flash und 8 MB Octal-PSRAM. Hersteller-Pinout und GT911-/RGB-Ansteuerung erhalten. Zwei RGB-Framebuffer in PSRAM; Netzwerk-Allokationsfehler dürfen keinen permanenten Reboot auslösen. Aktuelle Buildkonfiguration und Komponenten sind in der [Hardware-Buildanleitung](../Display/BUILD.md) und den [Build-Metadaten](../Display/releases/1.0.0/BUILD-INFO.json) dokumentiert.

Keine Pins aus einem ähnlich benannten Board übernehmen. Partitionen, NVS, OTA-Slots und reservierten Reset-Auftragssektor 0xC12000–0xC12FFF berücksichtigen. Keine Änderungen an eFuses/Flashverschlüsselung als beiläufiger Build-Schritt. Factory-Backups und private Signierschlüssel bleiben ausserhalb des Repos.

USB-Pakete enthalten einzelne Images, SHA256, Flash-Werkzeug und genaue Offsets. OTA-Pakete binden Metadaten/Board/Layout/Version und Hash per RSA-Signatur; der Vertrauensschlüssel ist öffentlich. Binärprüfung allein ist keine neue Release-Freigabe. R1/R2/R3 derselben Firmwareversion nicht mit aufsteigenden Versionsnummern verwechseln.

## Lokal prüfen

```sh
cd DockerAggregator
node --test
cd ..
python tools/test_sensor.py
python tools/check-public.py
```

Node benötigt für den Aggregator keine zusätzlichen npm-Pakete. Auf Windows bei Pfad-/Sandboxproblemen gegebenenfalls `node --preserve-symlinks --preserve-symlinks-main --test` verwenden. Docker-Build und `docker compose config --quiet` separat auf dem Zielsystem prüfen. Keine echten PRTG-Tokens für lokale Demo-/UI-Tests benötigen.

Für Firmware: Hersteller-/Hardware-Build, Modell-/LVGL-/Einstellungsprüfungen, Flash-/USB-Werkzeugtests und Paket-/Signaturprüfung. Zusätzlich am realen Board Display, Touch, WLAN, Setup-Web, Speicher, Persistenz, OTA und Rollback prüfen. Ein Screenshot eines Testservers ist kein RF-/ESP-IDF-Hardwaretest.

## Änderungen veröffentlichen

Entwicklung privat, neutralen Kandidaten ausserhalb der öffentlichen Arbeitskopie erstellen. Vor neuer öffentlicher Übernahme konkreten Umfang, Tests, Commit/Push und gegebenenfalls Deployment freigeben lassen. Die Freigabe für Dokumentation allein veröffentlicht keine neue Software und installiert keinen Container.

Nur beabsichtigte Dateien übernehmen, externe Lizenzen erhalten, Quell-/Paketdaten auf Secrets und interne Infrastruktur prüfen. Nach Commit/Push Remote-Commit und Arbeitsbaum prüfen. Fertige Release-Artefakte nicht still überschreiben; Revisionen ausdrücklich kennzeichnen. Geplante Funktionen nicht als bereits ausgeliefert darstellen.
