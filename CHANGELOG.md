# Änderungen

## 0.8.1 – Nur aktive Rubriken anzeigen

- Nicht konfigurierte Rubriken (`enabled: false`) verschwinden nach dem ersten empfangenen Status automatisch. Die übrigen Kacheln füllen die Zeile ohne Lücken.
- Aktive Rubriken mit unbekanntem Zustand, Warnung oder Fehler bleiben sichtbar. Verbindungsfehler deaktivieren keine Rubrik.
- Wird die geöffnete Rubrik deaktiviert, erscheint wieder die Übersicht. Reaktivierte Rubriken werden automatisch eingeblendet. Hinweise bleiben erreichbar.
- Vor der ersten Antwort ist die Konfiguration unbekannt. Der klar gekennzeichnete Demo-Modus zeigt weiterhin seine fiktiven Beispielrubriken.

**Installation:** Im Firmware-Register Versionen/Update prüfen, 0.8.1 installieren und nach Neustart innerhalb von 120 Sekunden bestätigen. WLAN, Panel und OTA-Einstellungen bleiben erhalten; kein Aggregator-Update nötig.

**Validierung:** Hardware-Build, beide PlatformIO-Profile, Modell- und native LVGL-Tests bestanden, einschliesslich Ausblenden, Navigation, Fehlern und Reaktivierung. Physischer Test von 0.8.1 steht aus. Simulator und bestehende Releases bleiben unverändert.

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
