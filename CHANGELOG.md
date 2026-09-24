# Änderungen

## 0.8.0 – Versionsauswahl für OTA

- Öffentlicher Direktkanal und Aggregator liefern bis zu acht verfügbare Versionen.
- Update, gezieltes Downgrade und Neuinstallation derselben Version am Display auswählen.
- Installierte Version, Zielversion, Paketgrösse und Fortschritt sind sichtbar.
- Deutlicher Downgrade-Hinweis und ausdrückliche zweite Bestätigung. Versions-/Kanalwechsel hebt eine vorgemerkte Installation auf. «Abbrechen» ist vor dem Installationsstart möglich.
- Konkretere HTTP-/TLS-/Verbindungsfehler statt einer pauschalen Download-Meldung. Keine Tokens oder URLs im OTA-Log.
- Signatur, Board, Partitionslayout, Versionsmetadaten und Hash bleiben verpflichtend. Download in den inaktiven Slot; Boot-Bestätigung weiterhin innerhalb von 120 Sekunden.
- Bestehende Firmware liest weiterhin `manifest.json` als neuestes Angebot. Die neue Firmware lädt zusätzlich `catalog.json`. Alte Aggregatoren ohne Katalog bleiben als Einzelangebot nutzbar.
- Aggregator archiviert signierte Uploads. Ältere Pakete verändern das neueste Angebot nicht; dieselbe Versionsnummer mit anderem Inhalt wird abgelehnt.

**Installation von 0.7.2:** «Update prüfen» → «Installieren» → Neustart → innerhalb von 120 Sekunden «Diese Version behalten». Danach «Versionen prüfen» für die Versionsauswahl. Kein neuer USB-Flash nötig.

**Downgrade:** Alte Firmware kann neuere Funktionen nicht anbieten. 0.7.x besitzt keine Versionsliste, kann aber über das neueste Manifest wieder auf 0.8.0 aktualisieren. Einstellungen werden nicht gelöscht; zukünftige inkompatible Layouts dürfen nicht in denselben Katalog aufgenommen werden.

**Validierung:** ESP-IDF-Hardware-Build und lokale Firmware-/LVGL-/USB-Tests, Aggregator-Tests, Signaturen und Paket-Hashes. Der reale OTA-Installations-/Rollback-Test von 0.8.0 steht aus. 0.7.2 und der öffentliche Manifest-Abruf wurden zuvor am Gerät bestätigt. Der produktive Aggregator wird durch die Veröffentlichung nicht aktualisiert.
