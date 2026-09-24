# Betrieb, Wartung und Fehlerbehebung

Gilt für das gesamte System. [Versionsstand](PROJEKTSTATUS.md) beachten: Aggregator 1.1.0 ist freigegeben; ältere Installationen müssen gezielt aktualisiert werden.

## Oberfläche und Zustände

Das Display gruppiert in Infra, Backup, Docker, Netzwerk und Dienste. Systemdetails per Touch öffnen, Einstellungen über den entsprechenden Knopf. Verbindung enthält WLAN/Token/NTP und Diagnose; Panel enthält Ursprung und Titel; System-Informationen zeigt Hardware, Speicher und Laufzeit; Firmware enthält Versionswahl, Installation und Werksreset; Webzugang enthält den lokalen WebAdmin/Hotspot; Info nennt Projektidee und Grundlagen.

| Anzeige | Bedeutung / Reaktion |
|---|---|
| OK | Konfigurierte Daten aktuell und ohne gemeldete Warnung |
| Warnung / Kritisch | PRTG-Zustand prüfen; Grenzwerte dort pflegen |
| Unbekannt | Quelle, Kanal, Zeitstempel, Konfiguration oder Datenalter prüfen |
| Veraltete Werte | Letzter Messwert kann sichtbar bleiben, ist aber keine aktuelle Messung |
| DEMO | Synthetische Vorführung, keine Aussage zur realen Infrastruktur |
| Bereich nicht sichtbar | Kein aktives konfiguriertes System in diesem Bereich; bei Live-Daten beabsichtigt |

Priorität im Aggregator: kritisch vor unbekannt vor Warnung vor OK. Fehlende Werte sind nicht null Prozent. Abfragepausen am Aggregator und am Display sind getrennt; Panel-Abfragen lesen den Cache und lösen keine weitere PRTG-Runde aus. Demo nach 50 Sekunden gilt ab Display 1.0.0; historische Releases hatten andere Werte.

## Zugangsdaten auseinanderhalten

| Zugang | Zweck | Speicher / Änderung |
|---|---|---|
| PRTG-API-Key | Aggregator liest PRTG | Secret-Datei; Ersatz ab WebAdmin in geschützter settings.json |
| Panel-Token | Display liest Health und Firmware vom Aggregator | Server-Secret und lokale Display-Konfiguration |
| OTA-Administrator-Token | Konfiguration und Firmware-Bereitstellung administrieren | Geschützte Server-Secret-Datei, kein Panel-Zugang |
| Aggregator-Passwort, ab 1.1.0 | Anmeldung im Aggregator-Browser | Salt/scrypt-Hash in admin/password.json; Bereich Zugang |
| Display-WebAdmin-Passwort | Einstellungen/OTA am einzelnen Display | Lokaler NVS-Speicher; unabhängig vom Aggregator |
| Hotspot-Passwort | Verbindung zu SetupPRTGDisplay | Für den Hotspot erzeugt und am Gerät angezeigt |

Tokenwerte nicht in Tickets, Screenshots, Git oder öffentlich lesbare Compose-Ausgaben übernehmen. Der Display-WebAdmin nutzt HTTP im vertrauenswürdigen lokalen Netz; der Aggregator-WebAdmin benötigt HTTPS. Ein vollständiges Flash-Backup enthält möglicherweise Zugangsdaten und bleibt vertraulich.

## Systeme ändern

Namen, Sensoren und Zuordnungen im Aggregator-WebAdmin ändern und gemeinsam speichern. Interne IDs bestehender Systeme bleiben im Formular fest. PRTG-Sensor-IDs sind eindeutig. «Verwerfen / neu laden» holt den gespeicherten Stand zurück. Ein Revisionskonflikt bedeutet, dass ein anderer Browser bereits gespeichert hat; neu laden und eigene Änderungen erneut eintragen.

Ab 1.1.0 können Systeme verschoben und kopiert werden. Kopien erhalten eigene interne IDs und leere PRTG-Sensor-IDs; Kanaldefinitionen bleiben erhalten. Reihenfolge nach Speichern in der Webübersicht, innerhalb der Display-Bereiche nach API-Reihenfolge. [Anleitung mit Bildern](../DockerAggregator/VERSION-1.1.md).

## Firmware aktualisieren oder zurückstufen

**Direktkanal:** am Display öffentliche Manifest-Adresse setzen, Versionen prüfen, gewünschte Version wählen und ausdrücklich installieren. Kein GitHub-Konto nötig.

**Aggregator 1.0.0:** im WebAdmin Firmware ein signiertes `.eagleota`-Paket hochladen. Alternativ technische Upload-Seite `/updates` mit OTA-Administrator-Token.

**Aggregator 1.1.0:** zusätzlich GitHub prüfen, gewünschte signierte Version in den Aggregator übernehmen. Das ist eine Bereitstellung, kein Fern-Flash. Das Display wählt danach Kanal Aggregator und installiert die gewünschte Version.

Nach dem OTA-Neustart innert **120 Sekunden bestätigen**. Bei nicht bestätigtem Start ist ein Rückfall vorgesehen; der gezielte physische Rollback-Test steht offen. Firmware niemals während Stromausfallrisiko installieren. Verfügbare ältere Versionen erlauben Downgrades; danach fehlen gegebenenfalls neuere Funktionen.

Gleiche Versionsnummer mit anderem BIN-Hash wird im Aggregator abgelehnt. Bei R1/R2/R3 von 1.0.0 daher gegebenenfalls Direktkanal/USB statt Archiv-Manipulation. Maximal acht Angebote. Vor einer kontrollierten Archivbereinigung sichern; nicht den gesamten Firmware-Ordner löschen.

## Aggregator aktualisieren

Freigegebenen Quellstand und passende Dokumentation verwenden. Daten/Secrets sichern; Compose, Mounts, Imageversion und Build-Kontext vergleichen. `docker compose config --quiet` vor dem gezielten Neubau/Start ausführen. Nicht routinemässig `down`, Volume-Prune oder Datenlöschung verwenden.

Danach Prozesszustand, HTTPS-Anmeldung, PRTG-Daten, Dateirechte und Firmware-Katalog prüfen. Neue Standard-ACLs für Dateien/Unterordner beachten; UID 1000 allein garantiert dem Linux-Administrator keinen Zugriff. Rollback benötigt den vorherigen Code/Compose-Stand und einen konsistenten Datenstand. Eine ältere Version ignoriert möglicherweise neue Verwaltungsdateien; zuerst in einer Testumgebung prüfen.

## Backup und Wiederherstellung

Sichern: Compose und lokale .env, ursprüngliche Konfiguration, sämtliche Secret-Dateien, `admin/`, `firmware/` und die verwendete Softwareversion. Ab 1.1.0 auch password.json. `settings.json` kann einen PRTG-Key enthalten. Backup geschützt ablegen und ACLs erhalten; nicht über den Webserver anbieten. Für einen konsistenten Dateistand Schreibzugriffe unterbinden, nötigenfalls Container kontrolliert stoppen.

Wiederherstellung: passenden Softwarestand bereitstellen, geschützte Daten und Secret-Mounts wiederherstellen, Besitzer/ACLs und Linux-Administratorzugriff prüfen, Compose validieren, starten. Anschliessend Login, gespeicherte Systemliste, PRTG und Firmware-Downloads prüfen. Ein vorhandener beschädigter Verwaltungsstand führt zu einem Startfehler; nicht blind durch einen leeren Stand ersetzen.

Passwort vergessen ab 1.1.0: Container stoppen, ausschliesslich admin/password.json geschützt ausserhalb admin/ sichern und aus admin/ entfernen, starten. Mit dem vorhandenen OTA-Administrator-Token neu einrichten. Einstellungen/PRTG/Panel/Firmware nicht löschen. Passwortwechsel ersetzt das technische OTA-Token nicht. Bei kompromittiertem OTA-Token dieses separat austauschen und Container neu erstellen/starten.

## Display zurücksetzen und weitergeben

Unter Firmware **Werksreset** wählen und Löschung ausdrücklich bestätigen. Ein offenes OTA zuerst bestätigen; während OTA/Boot-Bestätigung ist Reset gesperrt. Der Reset löscht die lokalen NVS-Einstellungen inklusive WLAN, Panel-/OTA-Konfiguration und WebAdmin-Daten. Installierte Firmware bleibt bestehen. Danach startet die Neueinrichtung mit SetupPRTGDisplay.

PRTG, Aggregator, externe Flash-Backups und andere Geräte werden dadurch nicht gelöscht. Ein vom Kunden verwendetes Panel-Token auf dem Server bei Bedarf ebenfalls ersetzen. Normale Flash-/OTA-Updates sind kein Werksreset.

## Fehler systematisch eingrenzen

| Beobachtung | Nächste Prüfung |
|---|---|
| USB-Port fehlt | Datenkabel, anderer Port, seriellen Monitor schliessen, Board-Bootmodus gemäss Flash-Anleitung |
| WLAN fehlt | 2,4 GHz, SSID/Passwort, unterstütztes Verfahren, Reichweite; Display-Verbindungsdiagnose |
| Setup-Seite öffnet nicht automatisch | Im Setup-WLAN bleiben, http://192.168.4.1 manuell; Hotspot-Zeitlimit beachten |
| WLAN vorhanden, API nicht erreichbar | DNS, NTP, HTTPS-Zertifikat, Aggregator-Ursprung ohne API-Pfad, Firewall |
| API 401/403 | Panel-Token bzw. Administrator-Zugang, TCP-Allowlist, tatsächliche Proxy-Adresse |
| Admin ORIGIN_DENIED | Browseradresse/Host und ADMIN_ORIGIN müssen genau zusammenpassen |
| API HTTP 200, aber keine aktuellen Messwerte | PRTG-Verbindung, Leserechte, Sensor-IDs, exakte Kanäle und UTC-Zeitbasis |
| Demo trotz erreichbarem WLAN | API-Erfolg ist nicht gleich PRTG-Vollständigkeit; echten Quellenstatus ansehen |
| Änderungen verschwinden | Speichern erfolgreich? Revisionskonflikt? Persistente admin-Mounts? settings.json hat Vorrang |
| Passwort nicht akzeptiert | Aggregator und Display nicht verwechseln; ab 1.1.0 Browser-Token nur vor Passwort-Einrichtung |
| Login gesperrt | Eine Minute warten; Proxy-Nutzer teilen sich den Absender für Fehlversuchslimits |
| GitHub-Abruf fehlgeschlagen, ab 1.1.0 | Ausgehendes HTTPS, freigegebene Raw-Katalogadresse, Signatur, Grössenlimit; vorhandene Angebote bleiben nutzbar |
| Firmware VERSION_CONFLICT | Gleiche Version, anderer Build: passenden Direktkanal oder USB verwenden |
| Kein neueres Update | Gleiche Versionsnummer: gewünschte Neuinstallation statt höherer Version wählen |
| Container healthy, Panel unbekannt | Prozess-Health separat von Monitoring-Zustand betrachten |

Für Fehlerberichte: Version und Paketrevision, Board, Zeitpunkt, genaue Fehlermeldung, reproduzierbare Schritte und bereinigte Logs. Keine Tokens, URLs mit Zugangsdaten, privaten Inventare oder vollständigen Flash-Dumps hochladen.
