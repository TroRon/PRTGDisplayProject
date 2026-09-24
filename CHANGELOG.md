# Änderungen

## 0.9.1 – Initiales Zahlenpasswort für WebAdmin

- Bei Neuinstallation und Update ohne bisherigen WebAdmin-Zugang erzeugt das Gerät automatisch ein zufälliges zwölfstelliges Zahlenpasswort. Benutzer: `admin`.
- Passwort unter **Einstellungen → Webzugang** ablesen; bei verbundenem WLAN startet der Webzugang automatisch unter `http://<Display-IP>`.
- Initialpasswort bleibt über Neustarts und Updates erhalten. Bereits selbst gesetzte Passwörter werden nicht überschrieben.
- Passwort im Browser, am Display oder über USB ändern. Danach wird der Initialwert aus der Anzeige und seinem zusätzlichen NVS-Eintrag entfernt; das eigene Passwort bleibt nur als Hash gespeichert.
- Ab 0.9.1 bleibt eine ausdrückliche Deaktivierung auch nach Neustarts erhalten. Bei einem Update von 0.9.0 wird jeder fehlende Zugang eingerichtet, auch wenn WebAdmin dort früher deaktiviert wurde.

**Update:** Firmware → Versionen prüfen → 0.9.1 installieren → nach Neustart innerhalb von **120 Sekunden «Diese Version behalten»** bestätigen. Danach das Passwort unter Webzugang ablesen und im Browser anmelden. WLAN, Panel-Token, Partitionslayout und Pinout bleiben unverändert. Kein Aggregator-Update nötig.

**Validierung:** Hardware-Build (ESP-IDF 5.5.0), beide PlatformIO-Profile, Modell-/LVGL-/Einstellungstests und 17 USB-/Flash-Tests bestanden. Signaturen, Hashes und Paketinhalt geprüft. Physischer Test von Initialpasswort, Persistenz, Web-Anmeldung und OTA/Rollback steht aus. Bestehende Releases bleiben unverändert.

[Einrichtung und technische Grenzen](Display/SETUP.md). HTTP nur im vertrauenswürdigen lokalen Netz verwenden. Der Initialwert liegt bis zur Passwortänderung auch lesbar im lokalen unverschlüsselten NVS, damit er nach einem Neustart am Display ablesbar bleibt. Keine Zugangsdaten sind in den Release-BINs enthalten.

## 0.9.0 – WLAN, Einrichtungshotspot und WebAdmin

- WLAN-Netzwerke suchen und antippen; anschliessend das verdeckte Passwort eingeben. WLAN kann auch vor dem Panel-Token eingerichtet werden.
- Manueller Einrichtungshotspot **SetupPRTGDisplay** mit zufälligem WPA2-Passwort, maximal zehn Minuten und automatischer Abschaltung nach erfolgreicher WLAN-Verbindung.
- Optionaler HTTP-WebAdmin: Einstellungen, Status, WLAN-Suche und signierte OTA-Updates inklusive Versionswahl und Downgrade im Browser.
- Benutzer `admin`, individuelles Passwort am Display oder über USB; kein gemeinsames Standardpasswort. WebAdmin ist bei bestehenden Geräten zunächst deaktiviert.
- Flash-Assistent mit «4 – nur WebAdmin» und `provision.py --port PORT --web-only`. WLAN, Panel und OTA bleiben dabei erhalten.

**Update von 0.8.1:** Einstellungen → Firmware → Versionen prüfen → 0.9.0 → Installieren. Nach Neustart innerhalb von **120 Sekunden «Diese Version behalten»** bestätigen. Danach unter **Webzugang** ein Passwort setzen und `http://<Display-IP>` öffnen. USB ist für dieses Update nicht erforderlich. Partitionslayout, NVS und Display-/Touch-Pins bleiben unverändert; kein Aggregator-Update nötig.

**Ersteinrichtung:** Ohne WLAN unter Webzugang den Hotspot starten, mit SetupPRTGDisplay verbinden und `http://192.168.4.1` öffnen. Passwort und Hinweise stehen am Display. [Schritt-für-Schritt-Anleitung](Display/SETUP.md). HTTP nur im vertrauenswürdigen lokalen Netz verwenden.

**Validierung:** Hardware-Build mit ESP-IDF 5.5.0, beide PlatformIO-Profile, Modell-/LVGL-/Einstellungstests, 17 Python-USB-/Flash-Tests und Browserbedienung an synthetischen Testdaten bestanden. Signaturen und Paket-Hashes geprüft. **Die physische Abnahme von 0.9.0 (WLAN, Hotspot, WebAdmin, Speicherreserve und OTA/Rollback) steht noch aus.** 0.8.1 wurde zuvor am Gerät bestätigt; alte Releases und Simulator 0.3 bleiben unverändert.

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
