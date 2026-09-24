# Firmware 1.0.0 – WLAN-Auswahl, Einrichtungshotspot und WebAdmin

[Gesamtes Projekt einrichten](../docs/HANDBUCH.md) · [Projektstand / Abnahme](../docs/PROJEKTSTATUS.md). Die folgenden Schritte betreffen den **Display-WebAdmin**, nicht den separaten Aggregator-WebAdmin.

**Aktuell 1.0.0, Revision 3 (R3).** Freigegebenes Ersatzpaket. Firmware-Version 1.0.0 und OTA-Sequenz 10000 bleiben unverändert; Git-Quellstand und GitHub-Release verwenden zur eindeutigen Zuordnung den Tag `v1.0.0-r3`. Lokale Tests ersetzen den Test auf dem echten Waveshare LCD-5B nicht. Die bisherigen Releases einschliesslich 0.9.1 bleiben unverändert.

## WLAN direkt am Display auswählen

1. Einstellungen → Verbindung → **WLAN suchen**.
2. Das gewünschte Netzwerk antippen. Das Passwortfeld wird aktiviert; bei einem anderen WLAN wird das alte Passwort aus dem Eingabefeld entfernt.
3. Passwort eingeben und **Speichern & verbinden** antippen.
4. IP-Adresse und Verbindungsstatus prüfen. Den Panel-Token kannst du später ergänzen. Ohne Token werden keine Live-Daten abgefragt; der Zustand bleibt unbekannt beziehungsweise die ausdrücklich gekennzeichnete Demo aktiv.

Es werden bis zu 20 sichtbare Netzwerke mit Signalstärke angezeigt. Doppelte Namen werden zusammengefasst. Unterstützt werden 2,4 GHz und WPA2/WPA3 Personal; offene, WEP- und Enterprise-Netzwerke sind nicht auswählbar. Versteckte SSIDs weiterhin manuell eingeben. Die Suche erfolgt im Netzwerkworker und blockiert nicht den LVGL-Thread. Wiederverbindungsversuche werden bei einer Suche ohne bestehende Verbindung kurz pausiert.

## Ersteinrichtung ohne USB mit Smartphone oder PC

1. Bei Erstinstallation ohne gespeicherte WLAN-Konfiguration sowie nach einem Werksreset startet **SetupPRTGDisplay automatisch**. Das Display öffnet direkt Webzugang. Später ist **Einstellungen → Webzugang → Hotspot starten** weiterhin möglich.
2. Mit **`SetupPRTGDisplay`** verbinden. Das zufällig erzeugte WPA2-Passwort steht auf dem Display. Es wird bei jedem Start neu erzeugt und nicht protokolliert.
3. Im Browser ausdrücklich **`http://192.168.4.1`** öffnen. Der Hotspot hat keinen Internetzugang. Ab R3 können QR-Code und Captive Portal die Verbindung/Anmeldeseite öffnen; das automatische Öffnen hängt vom Handy ab. Die manuelle Browseradresse bleibt der Rückweg. Falls das Smartphone nachfragt, mit diesem WLAN verbunden bleiben.
4. Benutzer **`admin`**. Das initiale **12-stellige Zahlenpasswort** steht oben im Register Webzugang. Es ist vom WLAN-Passwort des Hotspots unabhängig. Bei einem bereits eingerichteten Gerät das bisherige WebAdmin-Passwort verwenden.
5. Zuerst gegebenenfalls WebAdmin-Passwort ändern und erneut anmelden. Panelname und Aggregator-Adresse eintragen. Dann WLAN suchen, auswählen, Passwort und optional Panel-Token eingeben und speichern.
6. Bei erfolgreicher WLAN-Verbindung schaltet sich der Hotspot automatisch ab. Er endet spätestens nach etwa zehn Minuten; auch **Hotspot stoppen** ist möglich. Eine erneute Aktivierung am Display ist möglich, solange keine normale WLAN-Verbindung besteht. Der automatische Start erfolgt einmal pro Start eines unkonfigurierten Geräts. Nach Ablauf oder manuellem Stoppen wird er in diesem Startvorgang nicht automatisch erneut gestartet.
7. Smartphone/PC wieder mit dem normalen Netz verbinden. Am Display die neue IP-Adresse ablesen und **`http://<Display-IP>`** öffnen. Mit dem gespeicherten WebAdmin-Passwort anmelden. NTP-Zeit und Live-Verbindung kontrollieren.

Während eines WLAN-Wechsels kann die Browserverbindung abbrechen. «Speichern angefordert» bestätigt die Annahme des Auftrags, noch nicht die erfolgreiche WLAN-Verbindung; deren Ergebnis steht am Display und nach Wiederverbindung im Browser.

Der Hotspot erlaubt höchstens zwei Geräte. Er ist ein lokaler Einrichtungszugang, kein Router/NAT und keine Internetfreigabe. Das WLAN-Passwort des Hotspots wird bei jedem Start neu erzeugt. Das WebAdmin-Passwort bleibt dagegen gespeichert.

## WebAdmin im normalen WLAN

Ab **0.9.1** erzeugt das Gerät beim ersten Start ohne bestehendes WebAdmin-Passwort automatisch ein zufälliges **12-stelliges Zahlenpasswort**. Das gilt bei Neuinstallation und beim Update von älteren Versionen ohne WebAdmin-Zugang. Im verbundenen WLAN startet der Webzugang automatisch. Unter **Einstellungen → Webzugang** stehen Passwort, Adresse und Benutzer `admin`.

Das Passwort bleibt über Neustarts und weitere Updates gleich. Ein bereits selbst gesetztes Passwort wird niemals durch das initiale Passwort ersetzt. Im Register Webzugang oder im Browser kann der Anwender ein eigenes Passwort mit 5–63 UTF-8-Bytes setzen; danach verschwindet die Anzeige des initialen Passworts. Alternativ per USB ändern. Kein Passwort wird im Log oder in der HTTP-Statusantwort ausgegeben.

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

## Werksreset und Weitergabe ab 1.0.0

1. Ein frisch installiertes OTA-Update zuerst mit **Diese Version behalten** bestätigen.
2. Am Display **Einstellungen → Firmware → Werksreset** öffnen.
3. Rückfrage lesen. **Abbrechen** lässt sämtliche Daten unverändert. Erst **Alle Daten löschen & neu starten** beauftragt die Löschung.
4. Stromversorgung angeschlossen lassen. Das Display startet neu, löscht den vollständigen lokalen NVS-Bereich und prüft alle gelöschten Bytes, bevor es Einstellungen oder Netzwerkdienste lädt.
5. Danach sind WLAN, WLAN-Passwort, Panel-Token, NTP, Aggregator-Adresse, Displayname, OTA-Kanal/URL, WebAdmin-Hash und initiales Passwort entfernt. Die installierte Firmware bleibt erhalten. Das Gerät ist neu einzurichten und erzeugt ein neues zwölfstelliges Initialpasswort unter **Webzugang**. Ohne Verbindung zeigt es die gekennzeichnete Demo.

Der Reset ist ausschliesslich direkt am Display verfügbar. Während einer OTA-Aktion oder offenen Boot-Bestätigung ist er gesperrt. Ein Registerwechsel oder Schliessen verwirft eine offene Rückfrage. USB-Flash und normale OTA-Updates löschen die Einstellungen weiterhin nicht automatisch.

Der Auftrag betrifft Kundendaten **auf diesem Display**. PRTG, Aggregator, externe Flash-Backups, Browserdaten und Zugangsdaten auf anderen Geräten bleiben bestehen. Für die Weitergabe auch dort gespeicherte Panel-Tokens bei Bedarf ersetzen.

Technisch: vollständige sektorweise Flash-Löschung des NVS-Bereichs 0x9000–0xEFFF, anschliessend Rücklesen auf 0xFF. Dadurch werden auch ältere gelöschte NVS-Einträge innerhalb dieser Partition entfernt. Nach dem Neustart gibt es keine alten Laufzeit-Sitzungen mehr. Die aktuelle Firmware speichert keine Kundendaten in App-Slots und verwendet kein Flash-Coredump; WLAN-Treiberkonfiguration liegt im RAM. Es handelt sich nicht um eine Garantie für fremde frühere Firmware, externe Backups oder forensische Verfahren.

Für den bestätigten Reset-Auftrag ist der bisher unzugeordnete Sektor **0xC12000–0xC12FFF** nach `otadata` reserviert. Er enthält nur eine feste Auftragskennung, keine Kundendaten. Das bestehende Partitionslayout bleibt OTA-kompatibel; jeder Build muss diesen Bereich freihalten. Firmware prüft Flashgrösse, NVS-/OTA-Position und Überlappungen vor Zugriff. Ein vollständig gespeicherter Auftrag bleibt bis zur erfolgreichen NVS-Löschprüfung erhalten. Bei Stromunterbruch während der Löschung wird sie beim nächsten Start vor allen Diensten wiederholt. Ein Unterbruch vor gespeicherter Auftragskennung kann den Reset verhindern; in diesem Fall erneut ausführen.

Bei Flash-/Löschfehlern wird kein erfolgreicher Reset gemeldet und es werden beim betroffenen Reset-Start keine Kundendaten geladen. USB-Log prüfen und neu starten. Falls ein USB-Recovery nötig ist, nur bei bestätigtem LCD-5B und diesem Partitionslayout nach ausdrücklicher Löschentscheidung den NVS-Bereich löschen, zum Beispiel `python -m esptool --chip esp32s3 --port PORT erase_region 0x9000 0x6000`. Danach wieder starten; eine vorhandene Reset-Kennung wird erst nach erfolgreicher Prüfung entfernt. Ein normaler USB-Firmware-Flash allein ist kein Werksreset.

**Lokal geprüft für 1.0.0:** Rückfrage, Abbrechen, Registerwechsel, OTA-Sperre, Fehlermeldungen, Lösch-/Prüfreihenfolge und Wiederholung nach Fehlern sowie Überlappungsgrenzen; Hardware-/Simulator-Builds und bisherige Regressionstests. **Am Gerät ausstehend:** Reset mit synthetischer Konfiguration, verifizierte Löschung, neues Passwort, Neueinrichtung sowie gezielter Stromunterbruch und OTA/Rollback. Kein echtes Kunden-Gerät wurde automatisch zurückgesetzt.

Das bisherige Register Copyright / Idee heisst jetzt **Info** und bleibt ganz rechts. Inhalt und Lizenzhinweise bleiben erhalten.

## Demo-Wartezeit ab 1.0.0

Der automatische Demo-Modus beginnt erst nach **50 Sekunden** durchgehend fehlender Verbindung beziehungsweise vollständigem Quellenausfall. WLAN und Zeitsynchronisation erhalten dadurch mehr Zeit beim Start. Bis dahin bleibt der Zustand unbekannt; echte Daten beenden die Demo sofort. Teilfehler und echte Alarme bleiben Live. Die Wartezeit gilt auch bei späteren vollständigen Ausfällen und wird bei Erholung zurückgesetzt.

## Korrekturstand R2 unter derselben Versionsnummer

- WebAdmin erlaubt jetzt Passwörter ab **5 UTF-8-Bytes**, auch reine Zahlenfolgen wie fünf Ziffern. Gilt für Display, Browser und USB; weniger als fünf, mehr als 63 Byte oder Steuerzeichen werden abgelehnt. Bestehende Passwörter bleiben gültig. Das automatisch erzeugte Initialpasswort bleibt zwölfstellig.
- WLAN und Setup-Hotspot behalten die WPA2-Mindestlänge von acht Zeichen; diese Passwörter sind nicht das WebAdmin-Passwort.
- Automatischer Hotspot nur bei tatsächlich fehlender gespeicherter WLAN-Konfiguration, nicht bei einem Verbindungsfehler oder beschädigten/nicht lesbaren Daten. Bei fehlendem Netzwerk/WebAdmin-Start kein automatischer Hotspot. Eine offene OTA-Bestätigung wird zuerst abgeschlossen.
- Der Hotspot läuft höchstens zehn Minuten und endet nach erfolgreicher WLAN-Verbindung. Für die angezeigten Zugangsdaten wird automatisch Webzugang geöffnet. Nach Werksreset ist das Gerät wieder unkonfiguriert.
- R2 bleibt Firmware **1.0.0**, OTA-Sequenz 10000. Daher ist es kein nummerisch neueres Update. Mit dem korrigierten Direktkanal muss ein Gerät mit 1.0.0 **Versionen prüfen → 1.0.0 → Neuinstallation** verwenden oder per USB geflasht werden. Prüfsummen unterscheiden die Pakete.
- Aggregatoren können bereits vorhandene Pakete gleicher Versionsnummer mit anderem Hash ablehnen. Für diese Korrektur Direktkanal/USB verwenden; keine Schutzprüfung oder bestehende Uploads automatisch umgehen. Das ursprüngliche 1.0.0-Paket bleibt privat archiviert.
- Lokal geprüft: automatische Einrichtungsbedingungen, fehlende Konfiguration vs. WLAN-Ausfall, OTA-Sperre, fünfstellige USB-/Web-Passwörter, bisherige UI-/Reset-/Demo-Tests und Builds. Physischer Test von AP-Start/Passwortwechsel/Reset weiterhin ausstehend.

## Einrichtung per QR-Code und Captive Portal (R3)

Veröffentlichter Stand **1.0.0 R3**; App-Version und OTA-Sequenz bleiben unverändert. Dieses ausdrücklich freigegebene Paket ersetzt R2.

1. Nach Erstinstallation oder Werksreset startet der Setup-Hotspot wie unter R2 automatisch. Bei einem bereits eingerichteten Gerät ohne WLAN lässt er sich unter **Webzugang → Hotspot starten** aktivieren.
2. Das Display zeigt einen grossen WLAN-QR-Code. Mit der Handy-Kamera scannen und den Beitritt zu **SetupPRTGDisplay** bestätigen. Alternativ SSID und WLAN-Passwort vom Display abtippen.
3. Die WLAN-Anmeldeseite sollte automatisch erscheinen. Falls das Handy «Kein Internet» meldet, verbunden bleiben und **Anmelden** wählen. Wenn kein Fenster erscheint, ausdrücklich **http://192.168.4.1/** im Browser öffnen. Es gibt keine HTTPS-Umleitung. VPN, privates DNS oder das Verhalten des Handys können die automatische Erkennung verhindern.
4. Als **admin** mit dem angezeigten initialen oder deinem bereits gespeicherten WebAdmin-Passwort anmelden. Das WLAN-Passwort und das WebAdmin-Passwort sind normalerweise verschieden.
5. WLAN suchen, Heimnetz auswählen und dessen Passwort speichern. Bei erfolgreicher Verbindung endet der Hotspot. Die neue IP steht am Display; dort im Browser erneut anmelden und unter **Panel** die Aggregator-Adresse und den Displaynamen sowie die weiteren Parameter einrichten.
6. Der Hotspot endet spätestens nach zehn Minuten oder über **Hotspot stoppen**. QR-Code und Portal verschwinden dann. Den QR-Code später erneut über **WLAN-QR-Code anzeigen** öffnen, solange der Hotspot läuft.

Der QR-Code wird lokal erzeugt und enthält ausschliesslich die feste Setup-SSID und das zufällige WLAN-Passwort dieser Hotspot-Sitzung. Keine externen QR-Dienste, Panel-Tokens oder WebAdmin-Passwörter im QR-Code. Eine kurze Beschriftung erklärt den manuellen Weg. Die übrigen Einstellungen bleiben über **Zurück zu Webzugang** erreichbar.

Technik: lokaler DNS-Dienst an der AP-Adresse, DHCP-DNS-Ankündigung und HTTP-Weiterleitung zur festen AP-Adresse für GET-Aufrufe über das Setup-Interface. Keine Weiterleitung aus dem normalen WLAN, keine API-Authentifizierungs-Ausnahme, kein offener Resolver. DNS läuft begrenzt in der vorhandenen Hauptschleife und stoppt nach Ende des Hotspots. Passwort-/CSRF-/Host-Prüfungen bleiben bestehen. Keine zusätzliche Netzwerk-Task; QR-Puffer nutzt den bestehenden LVGL-PSRAM-Allocator. Ein DNS-Startfehler wird ohne Neustart protokolliert; die manuelle Browseradresse bleibt verfügbar.

Umsetzung orientiert sich am [offiziellen ESP-IDF-Captive-Portal-Beispiel](https://github.com/espressif/esp-idf/tree/v5.5/examples/protocols/http_server/captive_portal) und dem [Wi-Fi-QR-Format von ZXing](https://github.com/zxing/zxing/wiki/Barcode-Contents#wi-fi-network-config-android-ios-11).

Lokal erfolgreich geprüft: Hardware-Build, beide bisherigen PlatformIO-Profile, DNS-Parser einschliesslich EDNS und 100000 fehlerhaften Testpaketen, AP-/HTTP-Weiterleitungsregeln, LVGL-Dialog samt automatischem Öffnen/Schliessen und Dekodierung des tatsächlich gerenderten QR-Codes. Bestehende Modell-/Settings-Tests und 18 Python-Tests ebenfalls erfolgreich. Am Gerät noch erforderlich: QR-Scan mit iPhone/Android, DHCP/DNS und automatische Portalanzeige, Anmeldung, WLAN-Wechsel, manuelles Stoppen und Zehn-Minuten-Ablauf. Automatisches Öffnen ist geräteabhängig und kann ohne physischen Test nicht bestätigt werden.

Für R3 wie bei R2 **Neuinstallation** wählen oder USB verwenden. Vorherige Archive bleiben unverändert; gleiche Versionsnummern mit verschiedenen Prüfsummen können vom Aggregator abgelehnt werden.
