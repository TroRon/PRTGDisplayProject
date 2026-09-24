# Firmware 0.9.1 – WLAN-Auswahl, Einrichtungshotspot und WebAdmin

**Release 0.9.1.** Als USB-Paket und signiertes OTA-Angebot veröffentlicht. Lokale Tests ersetzen den Test auf dem echten Waveshare LCD-5B nicht. Die bisherigen Releases einschliesslich 0.9.0 bleiben unverändert.

## WLAN direkt am Display auswählen

1. Einstellungen → Verbindung → **WLAN suchen**.
2. Das gewünschte Netzwerk antippen. Das Passwortfeld wird aktiviert; bei einem anderen WLAN wird das alte Passwort aus dem Eingabefeld entfernt.
3. Passwort eingeben und **Speichern & verbinden** antippen.
4. IP-Adresse und Verbindungsstatus prüfen. Den Panel-Token kannst du später ergänzen. Ohne Token werden keine Live-Daten abgefragt; der Zustand bleibt unbekannt beziehungsweise die ausdrücklich gekennzeichnete Demo aktiv.

Es werden bis zu 20 sichtbare Netzwerke mit Signalstärke angezeigt. Doppelte Namen werden zusammengefasst. Unterstützt werden 2,4 GHz und WPA2/WPA3 Personal; offene, WEP- und Enterprise-Netzwerke sind nicht auswählbar. Versteckte SSIDs weiterhin manuell eingeben. Die Suche erfolgt im Netzwerkworker und blockiert nicht den LVGL-Thread. Wiederverbindungsversuche werden bei einer Suche ohne bestehende Verbindung kurz pausiert.

## Ersteinrichtung ohne USB mit Smartphone oder PC

1. Am noch nicht mit WLAN verbundenen Display: **Einstellungen → Webzugang → Hotspot starten**.
2. Mit **`SetupPRTGDisplay`** verbinden. Das zufällig erzeugte WPA2-Passwort steht auf dem Display. Es wird bei jedem Start neu erzeugt und nicht protokolliert.
3. Im Browser ausdrücklich **`http://192.168.4.1`** öffnen. Der Hotspot hat keinen Internetzugang und kein automatisches Captive Portal. Falls das Smartphone nachfragt, mit diesem WLAN verbunden bleiben.
4. Benutzer **`admin`**. Das initiale **12-stellige Zahlenpasswort** steht oben im Register Webzugang. Es ist vom WLAN-Passwort des Hotspots unabhängig. Bei einem bereits eingerichteten Gerät das bisherige WebAdmin-Passwort verwenden.
5. Zuerst gegebenenfalls WebAdmin-Passwort ändern und erneut anmelden. Panelname und Aggregator-Adresse eintragen. Dann WLAN suchen, auswählen, Passwort und optional Panel-Token eingeben und speichern.
6. Bei erfolgreicher WLAN-Verbindung schaltet sich der Hotspot automatisch ab. Er endet spätestens nach etwa zehn Minuten; auch **Hotspot stoppen** ist möglich. Eine erneute Aktivierung am Display ist möglich, solange keine normale WLAN-Verbindung besteht. Es gibt keinen automatisch startenden Dauer-Hotspot.
7. Smartphone/PC wieder mit dem normalen Netz verbinden. Am Display die neue IP-Adresse ablesen und **`http://<Display-IP>`** öffnen. Mit dem gespeicherten WebAdmin-Passwort anmelden. NTP-Zeit und Live-Verbindung kontrollieren.

Während eines WLAN-Wechsels kann die Browserverbindung abbrechen. «Speichern angefordert» bestätigt die Annahme des Auftrags, noch nicht die erfolgreiche WLAN-Verbindung; deren Ergebnis steht am Display und nach Wiederverbindung im Browser.

Der Hotspot erlaubt höchstens zwei Geräte. Er ist ein lokaler Einrichtungszugang, kein Router/NAT und keine Internetfreigabe. Das WLAN-Passwort des Hotspots wird bei jedem Start neu erzeugt. Das WebAdmin-Passwort bleibt dagegen gespeichert.

## WebAdmin im normalen WLAN

Ab **0.9.1** erzeugt das Gerät beim ersten Start ohne bestehendes WebAdmin-Passwort automatisch ein zufälliges **12-stelliges Zahlenpasswort**. Das gilt bei Neuinstallation und beim Update von älteren Versionen ohne WebAdmin-Zugang. Im verbundenen WLAN startet der Webzugang automatisch. Unter **Einstellungen → Webzugang** stehen Passwort, Adresse und Benutzer `admin`.

Das Passwort bleibt über Neustarts und weitere Updates gleich. Ein bereits selbst gesetztes Passwort wird niemals durch das initiale Passwort ersetzt. Im Register Webzugang oder im Browser kann der Anwender ein eigenes Passwort mit 12–63 UTF-8-Bytes setzen; danach verschwindet die Anzeige des initialen Passworts. Alternativ per USB ändern. Kein Passwort wird im Log oder in der HTTP-Statusantwort ausgegeben.

Ab 0.9.1 bleibt ein ausdrücklich deaktivierter Webzugang auch nach Neustarts und Updates deaktiviert. Beim ersten Update von 0.9.0 kann ein dort fehlendes Passwort nicht von einer früheren Deaktivierung unterschieden werden: In beiden Fällen wird das initiale Passwort erzeugt. Bei beschädigten oder nicht lesbaren Zugangsdaten wird kein vorhandenes Passwort automatisch überschrieben; lokal neu setzen.

Im Browser sind verfügbar:

- Verbindungs-, API- und PRTG-Status sowie Firmware-Version.
- WLAN-Suche, SSID, Passwort, Panel-Token und NTP-Zeitserver.
- Aggregator-Adresse und Displayname.
- Update-Kanal, Versionsliste, Update/Downgrade/Neuinstallation, Fortschritt und Bestätigung nach Neustart.
- Änderung des WebAdmin-Passworts mit Prüfung des bisherigen Passworts; anschliessend erneut anmelden.

Gespeicherte WLAN-Passwörter und Tokens werden nicht an den Browser zurückgegeben. Leere Secretfelder behalten bestehende Werte; beim Wechsel auf eine andere SSID muss das neue WLAN-Passwort angegeben werden. Die Sitzung endet nach 15 Minuten oder Abmeldung. Passwortänderung verwirft die alte Sitzung. Eine neue Anmeldung ersetzt die bisherige Browsersitzung.

Vergessenes Passwort direkt am Display oder per USB neu setzen. **Web deaktivieren** am Display entfernt den WebAdmin-Zugang und beendet auch den Einrichtungshotspot; WLAN und Monitoring-Konfiguration bleiben erhalten. Ein neu gesetztes Passwort aktiviert den Webzugang wieder.

**HTTP ist unverschlüsselt.** Zugang ausschliesslich im vertrauenswürdigen lokalen Netz verwenden. Die Oberfläche benötigt keine Cloud-Anmeldung, kein CDN und keine externen Schriftarten. Die Firmware akzeptiert die eigene IPv4-Adresse beziehungsweise während des Hotspots `192.168.4.1`; DNS-Namen/IPv6 sind für diese Version nicht vorgesehen.

## Passwort über USB setzen

Der neue Flash-Assistent bietet **4 – nur WebAdmin**. Die vollständige USB-Einrichtung fragt ebenfalls, ob das Passwort gesetzt werden soll. Direkter Aufruf im entpackten Release-Paket:

```text
python provision.py --port PORT --web-only
```

Unter macOS/Linux gegebenenfalls `python3`, unter Windows `py`. Seriellen Monitor vorher schliessen. Das Passwort wird zweimal verdeckt abgefragt, nicht in einer Datei gespeichert und nicht ausgegeben. Erst `WEB_CONFIG_SAVED` bestätigt den NVS-Commit. Erfordert Firmware 0.9.0 oder neuer. Bei Abbruch bleiben bereits bestätigte Teilschritte gespeichert.

## OTA über den Browser

Versionen prüfen → Zielversion auswählen → Installation vorbereiten → Version kontrollieren → Installieren & neu starten. Es wird derselbe signaturgeprüfte OTA-Weg wie am Display verwendet; keine beliebigen unsignierten BINs. Die Auswahl wird an Katalogrevision und Zielversion gebunden, damit ein paralleler Wechsel am Display keine andere Version installiert.

Nach Neustart innerhalb von **120 Sekunden** am Display oder nach erneuter Web-Anmeldung **Diese Version behalten** bestätigen. Ohne Bestätigung ist der bisherige Rollback vorgesehen. Bei einem Downgrade auf 0.8.1 ist dort kein Webserver vorhanden; die WebAdmin-Daten bleiben im separaten NVS-Namespace, werden aber erst von 0.9.0 wieder verwendet. Bestehende WLAN-/Panel-/OTA-Konfiguration und Partitionslayout bleiben kompatibel.

## Technische Grenzen und Abnahme

ESP-IDF 5.5.0, unveränderte Waveshare-Pins, GT911 und zwei RGB-Framebuffer in PSRAM. WLAN-Scan und AP-Umschaltung werden im vorhandenen Netzwerkworker ausgeführt. JSON- und HTTP-Nutzpuffer liegen in PSRAM; der HTTP-Server nutzt einen auf 8 KiB begrenzten internen Taskstack und höchstens drei Client-Sockets. Ein fehlgeschlagener Webstart beendet die Display-UI nicht und wird erst nach 30 Sekunden erneut versucht. Heapdiagnosen `before_web_start`/`after_web_start` ergänzen die bisherigen Logs.

Damit das initiale Zahlenpasswort nach einem Neustart weiterhin am Display ablesbar ist, liegt es bis zur Änderung zusätzlich im lokalen NVS. Beim Passwortwechsel oder Deaktivieren wird dieser Eintrag gelöscht; eigenes Passwort weiterhin nur als Hash. NVS ist unverschlüsselt und Flash-Backups bleiben vertraulich.

WebAdmin nutzt gesalzene PBKDF2-SHA256-Hashes (20000 Durchläufe), zufällige Sitzungstokens, HttpOnly-/SameSite-Strict-Cookie, CSRF-Token, Host-/Origin-Prüfung und Verzögerung nach falschen Anmeldungen. NVS-Flashverschlüsselung wird nicht nachträglich aktiviert. Physischer Zugriff auf Display/USB erlaubt wie bisher eine lokale Neueinrichtung.

**Lokal geprüft:** Hardware-Build; beide PlatformIO-Profile; Modell/LVGL; Einstellungen mit WLAN-Auswahl und Hotspot-Anzeige; Passwort-, Cookie-, CSRF-, Origin-, Ablauf-/Rate-Limit- und USB-Validierung; Python-Flash-/USB-Tests. Die Browseroberfläche wurde an einem ausdrücklich synthetischen HTTP-Testserver auf Desktop und bei 390 Pixel Breite bedient. Dieser Testserver emuliert weder ESP-IDF noch Funkverbindung oder Firmware-Authentifizierung.

**Am Gerät noch zu prüfen:** Initialpasswort bei frischem NVS und Update ohne Passwort; bestehendes Passwort erhalten; Login, Passwortwechsel und Neustart; deaktivierten Zugang nach Neustart beibehalten; Scan ohne Konfiguration und im verbundenen Betrieb; WPA2/WPA3-Verbindung; Hotspot samt Passwort, Abbruch, zehnminütiger Abschaltung und Abschaltung nach WLAN-Verbindung; Web-Anmeldung nach Neustart; NVS-Persistenz, falsches Passwort und Passwortwechsel; parallele Display-/Browserbedienung; interne Heapreserve während Scan/Web/HTTPS und OTA; gezielter Rollback.

Herstellergrundlagen: [Espressif Wi-Fi-Treiber 5.5](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-guides/wifi.html) und die lokal mit ESP-IDF 5.5.0 ausgelieferten `esp_http_server`-/`esp_wifi`-APIs. Beim Scan werden die vom Treiber angelegten Ergebnislisten explizit freigegeben.
