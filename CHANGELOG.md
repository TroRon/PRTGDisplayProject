# Änderungen

## Aggregator 1.1.0 – Systeme, Passwort und GitHub

- Reihenfolge speichern und Systeme samt Kanaldefinitionen kopieren; Kopien erhalten eigene IDs und leere PRTG-Sensor-IDs.
- Eigenes Passwort, sichere Hashspeicherung und widerrufbare Browser-Sitzungen; technisches OTA-Token bleibt gültig.
- Signierte GitHub-Angebote anzeigen und nach Bestätigung im Aggregator bereitstellen.
- Vollständiges Projekthandbuch, Betrieb, Entwicklung und aktueller Projektstatus mit synthetischen Bildern.

31 private / 16 öffentliche Tests und Desktop-/Mobilbrowser geprüft. Produktive Referenzinstallation aktualisiert; keine Änderung an Display-Firmware 1.0.0 R3 und keine neue physische OTA-Abnahme.

## Dokumentation des gesamten Projekts – 24.09.2026

Durchgängiges Projekthandbuch, Betrieb/Backup/Fehlerhilfe, Architektur/Entwicklung und Statusübersicht. R3-Einstiege und 50-Sekunden-Demo korrigiert. Aggregator 1.1.0 als noch unveröffentlichter Kandidat samt synthetischen Vorschauen dokumentiert. Keine Änderung der Software, Release-BINs oder OTA-Angebote durch dieses Dokumentationsupdate.

## Aggregator 1.0.0 – WebAdmin

Systemnamen, Kategorien, Sensoren und PRTG-Verbindung im Browser konfigurieren. Signierte Firmware-Pakete hochladen und am Display über den Aggregator auswählen. Eigenständiger Administrator-Zugang und persistente Einstellungen. [Einrichtung und Bedienung](DockerAggregator/WEBADMIN.md). Display-Firmware 1.0.0 R3 unverändert.

## 1.0.0 Revision 3 – WLAN-QR-Code und Captive Portal

Firmware-Version **1.0.0**, OTA-Sequenz **10000**; Quelltag **v1.0.0-r3**. Ausdrücklich freigegebener Ersatz von R2, keine Änderung bestehender Tags oder privater Archive.

- Grosser lokal erzeugter QR-Code verbindet das Handy mit **SetupPRTGDisplay**. Enthält nur SSID und zufälliges AP-Passwort; kein Panel-Token oder WebAdmin-Passwort.
- Captive Portal im Setup-WLAN: nach dem Verbinden die WLAN-Anmeldung öffnen. Falls kein Fenster erscheint, **http://192.168.4.1/** im Browser öffnen. Anmeldung als **admin** bleibt erforderlich.
- Automatisch angezeigte Anleitung mit manuellen Zugangsdaten, Rückweg zu Webzugang und Hotspot-Stopp. Hotspot/Portal enden nach WLAN-Verbindung oder spätestens zehn Minuten.
- DNS auf das Setup-Interface beschränkt, begrenzte Verarbeitung ohne zusätzliche Task. API-Anmeldung, Host-/Origin- und CSRF-Prüfungen bleiben bestehen. QR nutzt den bestehenden LVGL-PSRAM-Allocator.

**Schon auf 1.0.0?** Direktkanal: **Versionen prüfen → 1.0.0 → Neuinstallation**, anschliessend innert **120 Sekunden bestätigen**. Alternativ das neue USB-Paket verwenden. Einstellungen bleiben erhalten. Aggregatoren können einen anderen Hash derselben Version ablehnen; dann Direktkanal oder USB verwenden.

**Lokal geprüft:** Hardware-Build, beide PlatformIO-Profile, Modell-/LVGL-/Settings-Tests, DNS A/AAAA/EDNS und fehlerhafte Pakete, QR-Lebenszyklus und Dekodierung des tatsächlichen LVGL-Renderings, 18 Python-Tests, Paket-Hashes und Signaturen. **Physische Abnahme mit Display und iPhone/Android steht aus.** Automatisches Öffnen ist geräteabhängig.

## 1.0.0 Revision 2 – Einfachere Ersteinrichtung

Dieses ausdrücklich freigegebene Ersatzpaket behält **Firmware-Version 1.0.0 und OTA-Sequenz 10000**. Der zugehörige Quellstand trägt den separaten Git-Tag **v1.0.0-r2**; der ursprüngliche Tag wird nicht verschoben.

- **SetupPRTGDisplay startet automatisch**, wenn noch keine WLAN-Konfiguration gespeichert ist, insbesondere nach einem Werksreset. Webzugang mit den Zugangsdaten wird direkt geöffnet.
- Kein automatischer Hotspot bei bloss vorübergehendem WLAN-Ausfall, beschädigter/nicht lesbarer Konfiguration oder fehlendem WebAdmin-Zugang. Während offener OTA-Bestätigung wird gewartet. Abschaltung nach Verbindung beziehungsweise zehn Minuten bleibt erhalten.
- **WebAdmin-Passwörter ab fünf UTF-8-Bytes**, einschliesslich fünfstelliger Zahlenfolgen, werden am Display, im Browser und über USB akzeptiert. WLAN/WPA2 weiterhin mindestens acht Zeichen. Das automatisch erzeugte Initialpasswort bleibt zwölfstellig.
- Werksreset, Register Info und Demo-Wartezeit von 50 Sekunden bleiben enthalten. QR-Code und Captive Portal sind noch nicht enthalten.

**Schon auf 1.0.0?** Im öffentlichen Direktkanal **Versionen prüfen → 1.0.0 → Neuinstallation** wählen; danach innerhalb von **120 Sekunden** bestätigen. Die Firmware erkennt wegen unveränderter Versionsnummer kein höheres Update. Alternativ das neue USB-Paket vollständig entpacken und flashen. Bestehende Einstellungen bleiben erhalten. Ein Aggregator mit bereits archiviertem ursprünglichem 1.0.0 kann das Ersatzpaket wegen abweichendem Hash ablehnen; hierfür Direktkanal oder USB verwenden.

**Validierung:** Hardware-Build, beide PlatformIO-Profile, Modell-/LVGL-/Einstellungstests, automatische Einrichtungsbedingungen, WebAdmin-Validierung und 18 USB-/Flash-Tests bestanden. Browserformular mit fünfstelliger PIN und Desktop-/Mobilbedienung an synthetischen Daten geprüft. Signaturen und Paket-Hashes geprüft. **Physischer Test von R2 steht aus.** Das ursprüngliche Paket bleibt privat archiviert.

## 1.0.0 – Werksreset, Info und längere Demo-Wartezeit

- **Firmware → Werksreset** mit separater Rückfrage, deutlich benanntem Löschknopf und Abbrechen. Nur direkt am Display; während OTA oder offener Boot-Bestätigung gesperrt.
- Vollständige Löschung und Rückleseprüfung des lokalen NVS-Speichers vor Start der Dienste: WLAN, Panel-Token, Aggregator-Adresse, Displayname, NTP-/OTA-Konfiguration und WebAdmin-Zugang. Firmware bleibt installiert. Anschliessend neue Einrichtung und neues initiales Zahlenpasswort.
- Persistenter Reset-Auftrag zur Wiederaufnahme einer unterbrochenen Löschung. Fehler werden nicht als Erfolg ausgegeben. Externe Systeme und Backups bleiben unverändert.
- Das rechte Register **Copyright / Idee** heisst jetzt **Info**; Inhalt und Lizenzhinweise bleiben erhalten.
- Automatischer Demo-Fallback erst nach **50 Sekunden** vollständigem Ausfall statt 30 Sekunden. Echte Teilfehler bleiben Live; bei gültigen Daten endet die Demo sofort.
- Versionsangaben im Startlog und in den Einstellungen auf 1.0.0 vereinheitlicht.

**Update:** Firmware → Versionen prüfen → 1.0.0 installieren → nach Neustart innerhalb von **120 Sekunden «Diese Version behalten»** bestätigen. Ein Werksreset ist optional und wird durch das Update nicht ausgelöst. Einstellungen bleiben beim Update erhalten. Kein Aggregator-Update nötig.

**Validierung:** Hardware-Build mit ESP-IDF 5.5.0, beide PlatformIO-Profile, Modell-/LVGL-/Einstellungstests, Reset-Bestätigung/Abbruch/OTA-Sperre, Lösch-/Prüfreihenfolge und Fehlerwiederholung, 50-Sekunden-Grenze/Erholung/Timerüberlauf sowie 17 USB-/Flash-Tests bestanden. UI visuell geprüft. **Physische Reset-, Stromunterbruch- und OTA/Rollback-Abnahme steht aus.** Kein echtes Gerät wurde zurückgesetzt.

**Kompatibilität:** Pinout, Displaypuffer und Partitionstabelle unverändert. Für den Reset-Auftrag bleibt der bislang unzugeordnete Flash-Sektor 0xC12000–0xC12FFF reserviert. Keine Kundendaten in diesem Marker. [Einrichtung, Reset und Grenzen](Display/SETUP.md).

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
