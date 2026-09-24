# Aggregator 1.0.0 – WebAdmin

Freigegebener Aggregator 1.0.0 mit lokal geprüfter Weboberfläche und HTTP-API. WebAdmin unter **/admin**: Systeme/PRTG konfigurieren, signierte Firmware hochladen und am Display über den Kanal Aggregator auswählen. [Bedienung und Installation](WEBADMIN.md).

`ADMIN_ORIGIN` in `.env` setzen und `prepare-storage.sh` für den neuen Verwaltungsordner ausführen. Bestehende schreibgeschützte Konfigurations- und Secret-Mounts bleiben erhalten. Web-Einstellungen werden separat in `admin/settings.json` gespeichert und haben nach dem ersten Speichern Vorrang. PRTG-Token lässt sich im WebAdmin verdeckt ersetzen. Administrator- und Panel-Token bleiben Installations-Secrets.

# DockerAggregator

Der Aggregator fragt deine konfigurierten PRTG-Sensoren ab, vereinheitlicht Zustände und liefert eine kleine HTTPS-fähige API für das Display. Er enthält weder eine feste Serverliste noch einen vorgegebenen PRTG-Endpunkt. HTTPS übernimmt dein Reverse Proxy.

## 1. Dateien und Speicher vorbereiten

Dieses Verzeichnis auf deinen Docker-Host kopieren oder das Repo klonen. Im Verzeichnis `DockerAggregator`:

```sh
cp .env.example .env
sudo bash prepare-storage.sh ./data DEIN_ADMIN_BENUTZER
```

`DEIN_ADMIN_BENUTZER` ersetzen. Das Script benötigt `openssl`, `acl`/`setfacl`, `realpath` und `runuser`. Es fragt den PRTG-Key verdeckt ab, erzeugt zwei unabhängige Tokens für Panel und OTA-Administration und überschreibt bestehende Secrets nicht. Container-UID 1000 und der angegebene Administrator erhalten die erforderlichen Rechte; der Administrator auch über vererbbare ACLs auf neue Dateien. Tokenwerte werden nicht ausgegeben.

In `.env` sind Datenverzeichnis, Port, Bind-Adresse und Compose-Projektname anpassbar. Wird ein anderes Datenverzeichnis gewählt, dieses auch an das Setup-Script übergeben. Die Vorlage bindet den Backend-Port nur an `127.0.0.1`. Für Dockge am besten einen absoluten `DATA_DIRECTORY` verwenden; Build-Dateien im Stack-Verzeichnis ablegen.

## 2. Eigene PRTG-Konfiguration eintragen

`data/config/aggregator.json` bearbeiten:

- `prtg_url`: deine HTTPS-PRTG-Adresse ohne API-Pfad oder Zugangsdaten.
- `poll_seconds`: Pause nach einer Abfragerunde.
- `stale_seconds`: maximales Sensoralter; mindestens zwei Abfragepausen.
- `request_timeout_seconds`: Timeout einer PRTG-Abfrage.
- `entities`: deine Systeme mit eindeutiger `id`, frei wählbarem `label`, Kategorie und Sensoren.

Die Beispielkonfiguration enthält genau ein neutrales System und eine nicht gesetzte Sensor-ID. `null` muss durch die echte ID ersetzt werden. Weitere Systeme durch zusätzliche Einträge ergänzen. Kategorien: `compute` (Anzeige Infra), `backup`, `docker`, `network`, `services`. Nicht belegte Kategorien bleiben deaktiviert.

Namen im Display kommen aus **`entities[].label`** und lassen sich im WebAdmin ändern. Der Panel-Titel selbst wird separat am Gerät eingestellt. Kanalzuordnung und Beispiele stehen unter [PRTG Sensoren](../PRTG%20Sensoren/README.md).

## 3. Starten

```sh
docker compose config --quiet
docker compose up -d --build
docker compose ps
docker compose logs --tail=40 aggregator
```

Dockge kann dieselbe `compose.yaml` und lokale `.env` verwenden. Fehlende Secret-Dateien werden absichtlich nicht automatisch als Verzeichnisse angelegt. Das Container-Dateisystem ist schreibgeschützt; nur `data/firmware` ist für Uploads beschreibbar.

`/healthz` prüft den Prozess, nicht den Zustand der Infrastruktur. `/api/v1/health` benötigt `Authorization: Bearer <Panel-Token>`. Der PRTG-Key wird nur serverseitig verwendet. Fehler, Abfragedauer und Wiederherstellung werden protokolliert, keine Tokens oder Rohantworten.

## 4. HTTPS bereitstellen

Deinen Reverse Proxy auf den Backend-Port dieses Stacks richten. Das Panel benötigt eine gültige HTTPS-Verbindung und synchronisierte Zeit. Beispiel für einen Caddy-Siteblock auf demselben Host:

```caddy
monitor.example.org {
    reverse_proxy 127.0.0.1:8787
}
```

Domain, Port, DNS und Zertifikatsbereitstellung an deine Umgebung anpassen. Ein Proxy in einem anderen Container erreicht mit `127.0.0.1` nur sich selbst; dort ist ein gemeinsames Docker-Netz oder eine andere gezielte Anbindung nötig. Ein bestehender interner Proxy kann genauso verwendet werden. Das Projekt öffnet keine Firewall automatisch. Backend und Upload-Seite sollten nur aus den vorgesehenen Netzen erreichbar sein.

Optional unterstützt `ALLOWED_CLIENT_IPS` eine kommaseparierte Liste konkreter TCP-Absender-IP-Adressen. Das ist keine CIDR-Liste. Die tatsächlich sichtbare Proxy-Adresse konfigurieren; Forwarded-Header werden nicht als Zugriffsberechtigung vertraut. Im Containerbetrieb kann der sichtbare TCP-Absender von der ursprünglichen Client-IP abweichen.

Bei einer privaten CA: Node über `NODE_EXTRA_CA_CERTS` und einen schreibgeschützten Zertifikats-Mount erweitern. Das Display muss derselben CA vertrauen; hierfür ist derzeit ein angepasster Firmware-Build nötig. Zertifikatsprüfung nicht abschalten.

## 5. Panel und OTA verbinden

- Panel: WLAN, Panel-Token und NTP unter Verbindung eingeben.
- Unter Panel: beispielsweise `https://monitor.example.org` als Aggregator-Adresse und deinen eigenen Anzeigenamen speichern. Keine API-Pfade oder Credentials in die Adresse schreiben.
- Unter Firmware: **Aggregator (intern)** wählen.
- Browser: `https://monitor.example.org/updates` öffnen, separates OTA-Administrator-Token eingeben und eine signierte `.eagleota`-Datei hochladen.

Ein Upload stellt ein Angebot bereit und installiert nichts automatisch. Der lesende Panel-Token darf keine Firmware hochladen. Neue Dateien im Firmware-Ordner müssen für deinen Administrator zugänglich bleiben; Standard-ACLs aus dem Setup-Script dafür beibehalten.

### Versionsliste ab 0.8.0

Bis zu acht signierte `.eagleota`-Pakete können bereitstehen, auch ältere Versionen. `/api/v1/firmware/catalog.json` liefert deren signierte Metadaten mit Panel-Token; `manifest.json` bleibt das neueste Angebot für alte Firmware. Ein älterer Upload ersetzt das neueste Angebot nicht. Identische Wiederholungen sind erlaubt, verschiedene Inhalte unter derselben Versionsnummer werden abgelehnt.

Pakete liegen in `data/firmware/releases/<sha256>.eagleota`. Ein bestehendes `current.eagleota` wird bei der nächsten neuen Bereitstellung mit ins Archiv übernommen. Ein voller Katalog lehnt weitere Versionen ab. Zum Auslagern einer alten Version den Stack kontrolliert stoppen, das entsprechende ältere Paket aus `releases` in eine eigene Sicherung verschieben und wieder starten. `current.eagleota` und dessen neueste Version beibehalten. Bei ACL-fähigen Dateisystemen die Administratorrechte erhalten. Keine pauschale Löschung des Firmware-Verzeichnisses durchführen.

Für einen bisherigen Aggregator ist ein bewusster Neubau/Neustart mit dieser Version erforderlich. Die Veröffentlichung im Repo führt kein Deployment aus. Bis dahin sieht das neue Display nur dessen Einzelangebot; der öffentliche Direktkanal bietet unabhängig davon die vollständige Liste.

## Lokal ohne Infrastruktur testen

Node.js ab Version 20 genügt; keine npm-Abhängigkeiten:

```sh
node src/main.mjs --demo
node --test
```

Die Demo hört standardmässig auf Loopback. Sie liefert synthetische Daten und braucht keine echten Tokens. Die Live-Firmware lehnt API-Demo-Payloads bewusst ab; die Vorführ-Demo auf dem Display wird separat lokal erzeugt.

## Fehler einordnen

| Beobachtung | Prüfen |
|---|---|
| Container startet nicht | Dateipfade, Secret-Rechte, valide Konfiguration, Portkonflikt |
| PRTG 401/403 | API-Key, lesender Benutzer und Rechte auf die gewählten Sensoren |
| HTTP 200, aber unbekannte Werte | Sensor-ID, Status, Kanalnamen und Zeitstempel/UTC |
| Panel erreicht API nicht | DNS, HTTPS-Zertifikat, NTP, Proxy, Token und Netzfreigaben |
| Upload abgelehnt | OTA-Administrator-Token, Signatur, Board, höhere Version und Schreibrechte |

Kein produktiver Docker-Start ist durch die lokale Repository-Prüfung bestätigt. Compose auf deinem Zielhost vor dem Start validieren.
