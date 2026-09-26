# Display einrichten

## Firmware 1.0.3 â€“ Favoriten und MesswertverlÃ¤ufe

- Favoriten fÃ¼r ausgewÃ¤hlte Systeme und automatische Ansicht bei neuen kritischen StÃ¶rungen.
- Optionale MesswertverlÃ¤ufe mit maximal 16 echten Messpunkten je System, ausschliesslich im PSRAM.
- Hinweis auf 2,4-GHz-WLAN bei der Einrichtung.

Neue Komfortfunktionen sind standardmÃ¤ssig ausgeschaltet. Aktivierung unter Einstellungen â†’ Panel â†’ Anzeige. Keine zusÃ¤tzlichen PRTG-Abfragen und keine periodische Verlaufssicherung im Flash.

Update per OTA: Versionen prÃ¼fen â†’ 1.0.3 installieren â†’ nach dem Neustart innert 120 Sekunden bestÃ¤tigen. WLAN und Panel-Konfiguration bleiben erhalten. 1.0.2 bleibt als RÃ¼ckweg verfÃ¼gbar.

Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151; ESP-IDF 5.5.0. Hardware-Build, beide PlatformIO-Profile und lokale Modell-/LVGL-/Einstellungstests erfolgreich. Signaturen, Paket- und QuellprÃ¼fsummen geprÃ¼ft. Physische GerÃ¤teabnahme von 1.0.3 steht noch aus.

[USB-Paket 1.0.3](releases/prtg-display-1.0.3.zip) Â· [Anzeige einrichten](ANZEIGE.md)

## Firmware 1.0.2 â€“ schlanke Anzeige-Erweiterung

- Einheitlich hohe Systemkarten mit vollstÃ¤ndigem Text.
- Datenalter je System aus den PRTG-Messzeitstempeln.
- Optionaler Seitenwechsel mit Touch-/Einstellungs-/OTA-Pause.
- Optionaler softwareseitiger Nachtmodus mit Helligkeit, Stunden und manuellem UTC-Versatz.

Einrichten unter **Einstellungen â†’ Panel â†’ Anzeige**. Beide Automatiken sind zunÃ¤chst ausgeschaltet. Favoriten, automatische StÃ¶rungswechsel und Verlaufsgrafiken wurden bewusst zurÃ¼ckgestellt; ihre Speicher-/Hintergrundlogik ist nicht im Release enthalten.

**Update:** Versionen prÃ¼fen, **1.0.2 installieren**, nach dem Neustart innert **120 Sekunden bestÃ¤tigen**. Direktkanal und Aggregator werden unterstÃ¼tzt. WLAN/Panel-Zugang bleiben erhalten; kein Werksreset nÃ¶tig. Board Waveshare ESP32-S3-Touch-LCD-5B SKU 28151, ESP-IDF 5.5.0, bestehendes OTA-Layout unverÃ¤ndert. FrÃ¼here Pakete bleiben erhalten.

Modell-/LVGL-/EinstellungsprÃ¼fungen, beide PlatformIO-Profile und Hardware-Build lokal erfolgreich. Paket-Hashes und Signatur geprÃ¼ft. Physischer GerÃ¤te-/Dauerlauftest von 1.0.2 steht aus.

## Aggregator 1.2.0 und Display 1.0.1

Systemverwaltung und Ãœbersicht sind nach Infra, Backup, Docker, Netzwerk und Dienste gruppiert. Die Verwaltung zeigt aufklappbare Rubriken mit Anzahl aktiver Systeme. Â«System hinzufÃ¼genÂ» Ã¼bernimmt die Rubrik; Â«Nach oben/untenÂ» verschiebt innerhalb dieser Rubrik. Deaktivierte Systeme bleiben ausgegraut und ausdrÃ¼cklich markiert, damit sie wieder aktiviert werden kÃ¶nnen.

Ã„nderungen gelten erst nach **PrÃ¼fen und speichern**. Die stets erreichbare Speicherleiste markiert offene Ã„nderungen deutlich; Â«Verwerfen / neu ladenÂ» holt den gespeicherten Stand zurÃ¼ck. Die Ãœbersicht zeigt nur gespeicherte aktive Systeme. Kategoriezuordnung und alle bisherigen Konfigurationswerte bleiben erhalten.

Display **1.0.1** Ã¼bernimmt innerhalb seiner Rubriken die konfigurierte Reihenfolge. Die vorherige Firmware 1.0.0 R3 sortiert zusÃ¤tzlich nach Status; fÃ¼r die korrekte Reihenfolge ist deshalb das Update erforderlich. Farben und Hinweise zeigen Fehler weiterhin. Deaktivierte Systeme verschwinden nach dem Speichern und dem nÃ¤chsten gÃ¼ltigen API-Snapshot. Bei Verbindungsausfall kÃ¶nnen letzte EintrÃ¤ge als unbekannt sichtbar bleiben.

**Installation:** Aggregator aktualisieren; am Display unter Firmware die Versionen prÃ¼fen, **1.0.1 installieren** und nach dem Neustart innert **120 Sekunden bestÃ¤tigen**. Direktkanal und Aggregator sind unterstÃ¼tzt. WLAN, Panel-Zugang und Einstellungen bleiben erhalten. Keine Partition-/Pinout-Ã„nderung, kein Werksreset nÃ¶tig.

GeprÃ¼ft: 31 Aggregator-Tests, Desktop-/Mobilbrowser mit Gruppierung, Reihenfolge, Aktivierung, Speichern/Verwerfen und sichtbarer Speicherleiste; native LVGL- und Modelltests, beide PlatformIO-Profile, ESP-IDF-5.5.0-Hardware-Build und PaketprÃ¼fsummen. Physische Abnahme von Firmware 1.0.1 steht aus. FrÃ¼here Firmware-Releases bleiben unverÃ¤ndert.

[Komplette Inbetriebnahme](../docs/HANDBUCH.md) Â· [Bedienung / Updates / Fehlerhilfe](../docs/BETRIEB.md) Â· [Projektstand](../docs/PROJEKTSTATUS.md). Aggregator- und Display-Versionen sind unabhÃ¤ngig; Aggregator 1.1.0 erfordert keinen neuen Firmware-Build.

Diese Firmware unterstÃ¼tzt ausschliesslich **Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151**, 1024 Ã— 600, 16 MB Flash und 8 MB Octal-PSRAM. Aktuell ist **1.0.0 Revision 3** mit WLAN-QR-Code, Captive Portal, WLAN-Suche und WebAdmin. Lokal gebaut und getestet; die physische Abnahme der neuen Funktionen steht noch aus. 0.8.1 ist am realen GerÃ¤t bestÃ¤tigt.

**[WLAN, SetupPRTGDisplay und WebAdmin einrichten](SETUP.md).** Beim Update zunÃ¤chst innerhalb von 120 Sekunden Â«Diese Version behaltenÂ» bestÃ¤tigen, danach im Register Webzugang das automatisch erzeugte zwÃ¶lfstellige Zahlenpasswort ablesen. Bereits eigene PasswÃ¶rter bleiben erhalten; Benutzer: `admin`.

## 1. USB-Paket herunterladen

Verwende [prtg-display-1.0.0.zip](releases/prtg-display-1.0.0.zip) und entpacke es vollstÃ¤ndig. Alle folgenden Flash-Befehle laufen im entpackten Ordner. Einzelne Dateien nicht aus verschiedenen Releases mischen.

Enthalten sind vier getrennte Images, **kein Full-Flash-Abbild**. Das Paket enthÃ¤lt keine WLAN-Zugangsdaten und keine Aggregator-Adresse. Es darf keine Sicherung eines bereits eingerichteten GerÃ¤ts ersetzen.

## 2. Python und Werkzeuge installieren

Python 3 installieren. Auf Windows beim Installer Â«Add Python to PATHÂ» aktivieren. PowerShell beziehungsweise Terminal Ã¶ffnen:

```powershell
# Windows
py -m venv .venv
.\.venv\Scripts\python.exe -m pip install esptool==4.9.0 pyserial==3.5
.\.venv\Scripts\python.exe flash.py --check
.\.venv\Scripts\python.exe flash.py
```

```bash
# macOS / Linux
python3 -m venv .venv
.venv/bin/python -m pip install esptool==4.9.0 pyserial==3.5
.venv/bin/python flash.py --check
.venv/bin/python flash.py
```

## 3. GefÃ¼hrt flashen

1. Panel mit einem **USB-Datenkabel** anschliessen. Seriellen Monitor und andere Programme schliessen, die den Port verwenden.
2. Der Assistent prÃ¼ft die Paket-PrÃ¼fsummen und zeigt erkannte AnschlÃ¼sse. Den Anschluss des Panels auswÃ¤hlen, etwa `COM4` unter Windows; unter macOS den tatsÃ¤chlich angezeigten `/dev/cu.usbmodemâ€¦`-Port verwenden.
3. Board und Port kontrollieren. Erst die Eingabe `FLASH` startet das Schreiben.
4. Das Werkzeug schreibt vier Images und verifiziert sie anschliessend. Kabel wÃ¤hrenddessen eingesteckt lassen.
5. Auf die Erfolgsmeldung und die sichtbare OberflÃ¤che warten.

Falls keine Verbindung entsteht: anderes Datenkabel/USB-Port testen; bei Bedarf BOOT gedrÃ¼ckt halten, RESET kurz drÃ¼cken, BOOT loslassen und den Port erneut auswÃ¤hlen. Bei Ãœbertragungsfehlern `flash.py --baud 115200` verwenden. Kein `erase_flash` als Routine-Schritt ausfÃ¼hren.

Die genauen Offsets und der manuelle Befehl stehen in [FLASH.md](releases/1.0.0/FLASH.md). Bei einem bestehenden GerÃ¤t bleiben NVS-Einstellungen erhalten, sofern es das bisherige Projektlayout verwendet. Die vier Images setzen die OTA-Auswahl zurÃ¼ck und starten die neue Factory-Anwendung. Von Versionen vor 0.6 ist diese USB-Installation fÃ¼r das neue OTA-Partitionslayout erforderlich.

## 4. Verbindung und Anzeigename eintragen

Auf dem Display die Einstellungen Ã¶ffnen:

| Register | Eingabe |
|---|---|
| Verbindung | Eigenes 2,4-GHz-WLAN, Passwort, Panel-Token, erreichbarer NTP-Server |
| Panel | HTTPS-Ursprung des eigenen Aggregators, z. B. `https://monitor.example.org`; kein API-Pfad. Optional eigener Port. Anzeigename, z. B. Â«ServerraumÂ». Speichern. |
| Systeminformationen | IP, Zeitsynchronisation, HTTP-Status, Datenquelle und Speicher prÃ¼fen |
| Firmware | GewÃ¼nschten Update-Kanal auswÃ¤hlen |
| Webzugang | WebAdmin-Passwort setzen, Browseradresse ablesen oder offline den Einrichtungshotspot starten |

Der Panelname ist unabhÃ¤ngig von den **Systemnamen**: Systemnamen und Sensor-Zuordnungen kommen aus der Aggregator-Konfiguration. Der PRTG-API-Key wird ausschliesslich am Aggregator hinterlegt. Das Panel benÃ¶tigt den dort separat erzeugten **Panel-Token**.

![Panel-Einstellungen mit synthetischen Beispieldaten](panel-settings.png)

Einstellungen werden lokal in NVS gespeichert und Ã¼berstehen Neustarts und gewÃ¶hnliche OTA-Updates. NVS ist in diesem Build nicht verschlÃ¼sselt; ein physischer Flash-Dump ist deshalb vertraulich. Niemals solche Dumps verÃ¶ffentlichen.

### Optional: Zugangsdaten Ã¼ber USB eingeben

Der Flash-Assistent bietet diesen Schritt direkt an. Alternativ nach dem Start der OberflÃ¤che:

Ab **0.9.0** bietet der Flash-Assistent zusÃ¤tzlich **Â«4 â€“ nur WebAdminÂ»**. Seit 0.7.2 bestehen bereits diese MÃ¶glichkeiten: spÃ¤ter einrichten, vollstÃ¤ndig einrichten (WLAN/Token, Panel und optional OTA), **nur OTA-Kanal** oder **nur Panel** einrichten. Bei OTA wÃ¤hlst du unverÃ¤ndert, Aggregator oder direkt Ã¶ffentlich. Im Direktkanal genÃ¼gt Enter fÃ¼r die vorbelegte Adresse dieses Repos; eine eigene HTTPS-Manifest-URL ist ebenfalls mÃ¶glich. Speichern startet weder Download noch Installation.

```powershell
# Windows: COM4 durch den eigenen Port ersetzen
.\.venv\Scripts\python.exe provision.py --port COM4
```

```bash
# macOS/Linux: Port durch den tatsÃ¤chlich angezeigten Anschluss ersetzen
.venv/bin/python provision.py --port /dev/cu.usbmodemXXXX
```

WLAN-Name und NTP werden abgefragt, Passwort und Panel-Token verdeckt eingegeben. Das Werkzeug legt keine Konfigurationsdatei an. Es meldet Erfolg erst nach SpeicherbestÃ¤tigung. Ab 0.7.2 fragt die vollstÃ¤ndige USB-Einrichtung auch Aggregator-Adresse und Displayname ab. Alternativ bleibt das Register Panel verfÃ¼gbar. USB ist eine lokale unverschlÃ¼sselte Konfigurationsverbindung; nur am eigenen Rechner verwenden.

Nur den OTA-Kanal nachtrÃ¤glich Ã¤ndern, ohne WLAN und Token erneut einzugeben (Firmware 0.7.1 oder neuer):

```powershell
.\.venv\Scripts\python.exe provision.py --port COM4 --ota-only
```

Unter macOS/Linux entsprechend `.venv/bin/python provision.py --port DEIN_PORT --ota-only` verwenden. Die URL und Kanalwahl bleiben nach einem Neustart erhalten. Bereits offene Einstellungen am Display danach schliessen und erneut Ã¶ffnen. Ã„ltere Firmware weist den neuen USB-Befehl zurÃ¼ck; das Tool meldet dafÃ¼r keinen Speichererfolg.

## 5. Live-Betrieb kontrollieren

### Aggregator-Adresse und Displayname bequem Ã¼ber USB

Ab Firmware **0.7.2** kannst du bei bereits funktionierendem WLAN im Flash-Assistenten **Â«3 â€“ nur PanelÂ»** wÃ¤hlen. Ohne erneutes Flashen:

```powershell
# Windows, den eigenen Port verwenden
.\.venv\Scripts\python.exe provision.py --port COM4 --panel-only
```

```bash
# macOS/Linux, den eigenen Port verwenden
.venv/bin/python provision.py --port /dev/cu.usbmodemXXXX --panel-only
```

Das Script fragt die **Aggregator-Adresse** (z. B. `https://monitor.example.org`, ohne `/api/v1/health`) und den **Displaynamen** ab. Es bestÃ¤tigt erst nach erfolgreichem Speichern. WLAN, Token und OTA bleiben dabei unverÃ¤ndert. Die vollstÃ¤ndige Einrichtung fragt dieselben Angaben zusammen mit WLAN und OTA ab; Eingaben werden vor der Ãœbertragung geprÃ¼ft. Jeder Bereich wird separat gespeichert und bestÃ¤tigt. Bei einem spÃ¤teren Fehler bleiben bereits bestÃ¤tigte Bereiche gespeichert.

Danach etwa 20 Sekunden warten. Bei erfolgreicher Datenabfrage endet die Demo automatisch. Offene Einstellungen am Display schliessen und erneut Ã¶ffnen, damit sie die neuen Werte anzeigen. Ã„ltere Firmware unterstÃ¼tzt diesen Befehl nicht und muss zuerst aktualisiert werden.

Erwartet werden eine IP-Adresse, synchronisierte Zeit, erfolgreicher API-Abruf und aktuelle PRTG-Werte. HTTP 200 allein beweist noch keine erfolgreiche PRTG-Abfrage: Der Aggregator kann erreichbar sein, wÃ¤hrend seine Datenquelle ausgefallen ist.

Bei vollstÃ¤ndigem Quellenausfall erscheint nach 50 Sekunden eine ausdrÃ¼cklich markierte **DEMO** mit synthetischen Werten. Das Panel prÃ¼ft die Verbindung im Hintergrund weiter. Sobald echte Daten zurÃ¼ckkehren, endet die Demo. Einzelne echte Alarme werden nicht durch Demo-Werte verdeckt. Demo-Werte sind kein Nachweis einer gesunden Infrastruktur.

| Symptom | PrÃ¼fen |
|---|---|
| Keine IP | WLAN-Name, Passwort, 2,4 GHz und DHCP |
| Zeit fehlt / TLS scheitert | NTP, DNS, Firewall, gÃ¼ltiges HTTPS-Zertifikat samt Kette |
| HTTP 401/403 | Panel-Token, Proxy und optionale Aggregator-IP-Freigabe |
| API erreichbar, PRTG unbekannt | PRTG-Adresse, API-Key, Leserechte, Sensor-IDs, Kanalnamen und UTC-Zeitbasis |
| Einzelne Werte fehlen | Sensorstatus und exakte Kanalnamen in PRTG |

Es gibt keinen Schalter zum Abschalten der TLS-PrÃ¼fung. FÃ¼r eigene private Zertifizierungsstellen ist derzeit ein angepasster Firmware-Build erforderlich.

## 6. OTA-Updates

### Auf 1.0.0 aktualisieren

Im Firmware-Register Â«Versionen prÃ¼fenÂ» beziehungsweise Â«Update prÃ¼fenÂ» antippen und 1.0.0 installieren. Nach Neustart innerhalb von 120 Sekunden Â«Diese Version behaltenÂ» bestÃ¤tigen.

Ab 0.8.0: **Â«Versionen prÃ¼fenÂ» â†’ Zielversion auswÃ¤hlen â†’ Â«InstallierenÂ» â†’ ausdrÃ¼cklich bestÃ¤tigen.** Angezeigt werden installierte und gewÃ¤hlte Version, PaketgrÃ¶sse und Fortschritt. Die Liste kennzeichnet Update, Downgrade oder Neuinstallation. Eine vorgemerkte Installation kann vor dem zweiten Klick abgebrochen werden; wÃ¤hrend des Schreibens nicht ausschalten. Nach jedem OTA-Neustart gilt die Boot-BestÃ¤tigung erneut.

![Versionsauswahl und Downgrade-BestÃ¤tigung](ota-versions.png)

Der Ã¶ffentliche Katalog enthÃ¤lt derzeit **1.0.0 und 0.9.1**. FrÃ¼here Release-Pakete und OTA-Angebote wurden auf Wunsch des Projektbetreibers entfernt. Die Versionswahl unterstÃ¼tzt weiterhin Updates und Neuinstallation; ein Downgrade von 1.0.0 auf 0.9.1 ist verfÃ¼gbar. WLAN, Panel- und OTA-Konfiguration bleiben gespeichert. Auch Ã¤ltere installierte Firmware kann das aktuelle Manifest zum Update verwenden.

Die Git-Historie wurde nicht umgeschrieben; historische DateistÃ¤nde kÃ¶nnen dort weiterhin vorhanden sein.

Die vorhandene Manifest-URL bleibt gleich. Ab 0.8.0 wird im selben Verzeichnis zuerst `catalog.json` gelesen; nur bei HTTP 404 erfolgt ein RÃ¼ckgriff auf das Einzelangebot. Der Katalog ist auf acht Versionen und 16 KiB begrenzt. Die Signaturen der einzelnen Angebote schÃ¼tzen Version und Image-Hash; der Katalog selbst ist keine signierte VollstÃ¤ndigkeitsgarantie. BinÃ¤rdateien mÃ¼ssen unter ihrem SHA256-Namen im selben Verzeichnis liegen.

FÃ¼r eigene VerÃ¶ffentlichungen erzeugt `node tools/build-catalog.mjs` aus den geprÃ¼ften Release-Paketen die neuesten acht Angebote. Manifest, Katalog und zugehÃ¶rige BINs gemeinsam verÃ¶ffentlichen. Der echte OTA-/Rollback-Test fÃ¼r 0.8.0 bleibt bis zur GerÃ¤teabnahme offen.

**Aggregator:** Signiertes `.eagleota`-Paket auf `https://DEIN-AGGREGATOR/updates` mit dem separaten OTA-Administrator-Token hochladen. Dieses Token gehÃ¶rt nicht auf das Panel. Am Panel den Kanal Aggregator wÃ¤hlen.

**Ã–ffentlich:** Im Register Firmware den direkten HTTPS-Kanal wÃ¤hlen. Vorbelegt ist:

```text
https://raw.githubusercontent.com/TroRon/PRTGDisplayProject/main/Display/ota/manifest.json
```

Dieser Pfad liefert das neueste verÃ¶ffentlichte Angebot. Keine Anmeldung erforderlich. Die URL muss auf ein signiertes Manifest zeigen, nicht auf eine GitHub-Webseite oder ein USB-ZIP. Bereits gespeicherte OTA-Einstellungen werden bei einem Update nicht Ã¼berschrieben; vorhandene leere/andere URLs gegebenenfalls anpassen.

Â«Update prÃ¼fenÂ» antippen, Angebot kontrollieren und Â«InstallierenÂ» bestÃ¤tigen. WÃ¤hrend des Downloads pausiert die Live-Abfrage. Nach dem Neustart Display und Touch prÃ¼fen und die neue Version **innerhalb von 120 Sekunden bestÃ¤tigen**. Ohne BestÃ¤tigung ist ein automatischer RÃ¼ckfall vorgesehen; die reale Abnahme dieses Mechanismus steht noch aus. Ab 0.8.0 kÃ¶nnen gleiche Versionen neu installiert und Ã¤ltere signierte Versionen gezielt ausgewÃ¤hlt werden. Ã„ltere Firmware bietet weiterhin ausschliesslich neuere Versionen an.

GeprÃ¼ft werden Signatur, Board, Layout, Version, GrÃ¶sse und SHA256. Ã–ffentliche PrÃ¼fschlÃ¼ssel sind enthalten; private SignierschlÃ¼ssel werden nicht verteilt. Das ist kein eFuse-Secure-Boot und kein Schutz gegen absichtliches Neuflashen per USB.

## Quellcode selbst bauen

Im aktivierten **ESP-IDF-5.5.0-Terminal** aus `Display/firmware/hardware`:

```bash
idf.py set-target esp32s3
idf.py build
```

`sdkconfig.defaults`, `partitions.csv` und `dependencies.lock` dokumentieren die Vorgaben. AbhÃ¤ngigkeiten werden durch den IDF Component Manager geladen. Kein Arduino-/Simulator-Build auf dieses GerÃ¤t flashen. Display-/Touch-Treiber in `vendor/waveshare` stammen vom konkreten Waveshare-Board. Den Pinout nicht aus Ã¤hnlichen Modellen Ã¼bernehmen.

Die ausgelieferten Images wurden nativ mit ESP-IDF 5.5.0 gebaut. FÃ¼r Ã¶ffentliche eigene Builds Compiler-PfadprÃ¤fixe neutralisieren (CMake-Option `PUBLIC_SOURCE_ROOT`) und Dateien vor VerÃ¶ffentlichung prÃ¼fen. Eigene OTA-Forks benÃ¶tigen einen eigenen privaten SignierschlÃ¼ssel ausserhalb des Repos und denselben Ã¶ffentlichen SchlÃ¼ssel in Firmware und Aggregator; zuerst per USB installieren. Der mitgelieferte Ã¶ffentliche SchlÃ¼ssel autorisiert keine selbst signierten Pakete.

Weitere technische Angaben: [BUILD-INFO.json](releases/1.0.0/BUILD-INFO.json), [Lizenzen](LICENSES/).

## Aktive Rubriken ab 0.8.1

Rubriken mit `enabled: false` werden nach der ersten Antwort ausgeblendet. Aktive Rubriken mit Fehlern oder unbekannten Daten bleiben sichtbar. Die Ã¼brigen Kacheln verteilen sich gleichmÃ¤ssig. Hinweise bleiben erreichbar. Im Demo-Modus sind alle fiktiven Beispielrubriken aktiv. DafÃ¼r sind keine neuen Einstellungen und kein Aggregator-Update nÃ¶tig. Die aktuelle Version 1.0.0 kann Ã¼ber OTA installiert werden; nach Neustart innerhalb von 120 Sekunden bestÃ¤tigen. Der Betreiber hat das Ausblenden inaktiver Rubriken unter 0.8.1 auf dem realen Display bestÃ¤tigt; dies ist keine Abnahme aller neuen 1.0.0-R3-Funktionen.

## Werksreset und Info ab 1.0.0

Unter **Firmware â†’ Werksreset** erscheint vor dem LÃ¶schen eine ausdrÃ¼ckliche RÃ¼ckfrage. Nach BestÃ¤tigung werden alle lokalen Kundeneinstellungen vollstÃ¤ndig gelÃ¶scht und geprÃ¼ft; Firmware und externe Systeme bleiben erhalten. Ein Update allein lÃ¶st keinen Reset aus. Danach neu einrichten und das neue Zahlenpasswort unter Webzugang ablesen. Details und Testgrenzen: [Werksreset](SETUP.md#werksreset-und-weitergabe-ab-100). Das Register **Info** ersetzt Copyright / Idee und bleibt ganz rechts.

## 1.0.0 Revision 3 installieren

Die Firmware-Versionsnummer bleibt 1.0.0. Bei bereits installierter 1.0.0 im Direktkanal **Versionen prÃ¼fen â†’ 1.0.0 â†’ Neuinstallation** wÃ¤hlen oder das korrigierte USB-Paket verwenden; nach OTA innerhalb von 120 Sekunden bestÃ¤tigen. Bei Erstinstallation oder nach Reset startet SetupPRTGDisplay automatisch und Ã¶ffnet die Zugangsdaten am Display. WebAdmin erlaubt fÃ¼nfstellige ZahlenpasswÃ¶rter; WPA2/WLAN weiterhin mindestens acht Zeichen. [Details und EinschrÃ¤nkungen beim Aggregator](SETUP.md#korrekturstand-r2-unter-derselben-versionsnummer).
