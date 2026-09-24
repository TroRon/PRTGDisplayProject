# PRTG Sensoren

Hier beginnt die Datenerfassung. Du kannst vorhandene PRTG-Sensoren verwenden; die Beispiele sind keine Pflicht. Sie sind allgemeine Vorlagen und müssen auf deinem PRTG-/Probe-/Host-Stand getestet werden.

## Vorhandene Sensoren nutzen

1. Auf dem betreffenden Gerät in PRTG den Sensor öffnen.
2. Sensor-ID und gewünschte Kanalnamen notieren. Die ID identifiziert den Sensor; der Name eines Hosts allein genügt nicht.
3. Grenzwerte und Zustandsmeldungen in PRTG prüfen. Der Aggregator übernimmt den PRTG-Sensorstatus.
4. Einen lesenden API-Benutzer anlegen. Dessen Gruppe an den benötigten Geräten/Gruppen unter Zugriffsrechte auf **Lesezugriff** setzen; Vererbung kontrollieren.
5. API-Key für diesen Benutzer lokal erstellen. Den Benutzer für die aktuelle Aggregator-Zeitinterpretation auf **UTC** konfigurieren; später Abfragezeit und Sensorzeit vergleichen.

[Paessler: API-Keys und Zugriffsrechte](https://www.paessler.com/manuals/prtg/api_keys)

## Beispiele

| Datei | Zweck | Benutzereinstellungen |
|---|---|---|
| `Proxmox.ps1` | Node/Cluster, CPU/RAM/Root-Disk und ausgewählte Gäste | Host, API-Port, Node, Token-ID, Secret-Datei und Gast-Tag |
| `PBS.ps1` | Datastore, Backup-Alter und GC-Status | API-Adresse, Datastore, Namespace, Token-ID, Secret-Datei, maximales Backup-Alter |
| `docker-host.py` | Docker Engine, RAM, Dateisysteme und Containeranzahlen | Host, Ausführungsrechte und optionaler Root-Dateisystempfad |

### Proxmox / PBS auf der Windows-Probe

Die PowerShell-Dateien gehören in den für **EXE/Script Advanced** konfigurierten Script-Ordner deiner Probe. Sie geben PRTG-JSON zurück. Die PRTG-Dienstidentität braucht Leserechte auf die jeweilige Secret-Datei; sie enthält ausschliesslich den API-Token-Secretwert, nicht das komplette HTTP-Headerfeld. Token-IDs und Zieladressen sind Parameter.

Beispiele mit Platzhaltern, keine echten Zugangsdaten:

```powershell
.\Proxmox.ps1 -HostName 'hypervisor.example.org' -Node 'node-a' -TokenID 'monitor@pve!display' -SecretFile 'C:\ProgramData\PRTGDisplay\proxmox.secret' -GuestTag 'monitor'
.\PBS.ps1 -BaseUri 'https://backup.example.org:8007/api2/json' -Datastore 'my-store' -Namespace '' -TokenID 'monitor@pbs!display' -SecretFile 'C:\ProgramData\PRTGDisplay\pbs.secret' -MaxBackupAgeHours 30
```

API-Benutzern nur benötigte Leserechte erteilen. Zertifikatsprüfung bleibt aktiv; bei privaten CAs muss die Probe deren CA vertrauen. Gastfilter und Backup-Alter an deine eigene Organisation anpassen. Die Vorlagen sind nicht gegen deine Hosts validiert.

[Paessler: EXE/Script Advanced](https://manuals.paessler.com/exe_script_advanced_sensor.htm)

### Docker auf dem Linux-Zielhost

`docker-host.py` benötigt Python 3, Docker CLI, `/proc` und Leserechte auf die Docker-Engine. Die Ausgabe ist XML für einen **SSH Script Advanced**-Sensor. Installationsverzeichnis und ausführbare Berechtigungen gemäss deiner Probe festlegen; keine Befehle ungeprüft als root ausführen.

```sh
python3 docker-host.py --root /
```

Der Zugriff auf den Docker-Socket ist weitreichend. Für den Monitoring-Benutzer einen eng begrenzten, root-eigenen Wrapper bzw. eine passend eingeschränkte Ausführung vorsehen; nicht pauschal beliebige sudo-Befehle freigeben. Der Wrapper und das Script dürfen vom Monitoring-Benutzer nicht veränderbar sein. SSH-Hostkeys prüfen. Die konkrete Rechtevergabe hängt von deinem Host ab und wird nicht automatisch verändert.

Fehlen Docker- oder Speicherdaten, liefert das Script einen Sensorfehler statt künstlicher Nullwerte. Container werden gezählt; einzelne Container werden nicht separat durch den Aggregator bewertet.

[Paessler: SSH Script Advanced](https://www.paessler.com/manuals/prtg/ssh_script_advanced_sensor)

## Von Kanälen zur Anzeige

Ein Aggregator-Eintrag ordnet einen frei gewählten Systemnamen und Sensor-ID den Kanalnamen zu:

```json
{
  "id": "server-a",
  "label": "Mein Server",
  "category": "compute",
  "sensors": [{
    "key": "health",
    "id": 1234,
    "metrics": {"cpu": "CPU Usage", "ram": "RAM Usage", "root_disk": "Root Disk Usage"}
  }]
}
```

`1234` ist ein Beispiel und muss ersetzt werden. `label` ist der Name auf dem Display. Die rechte Seite der `metrics`-Zuordnung muss den PRTG-Kanalnamen entsprechen; links stehen die vom Display verstandenen Kennzahlen.

Die verfügbaren Schlüssel umfassen beispielsweise `cpu`, `ram`, `root_disk`, `temperature`, `zfs_health`, `zfs_capacity`, `engine`, `disk`, `running`, `total`, `daily_current`, `daily_total`, `daily_old` und `usage`. Andere Schlüssel werden als allgemeine Kennzahlen dargestellt. Werte müssen zu den vorgesehenen Einheiten passen; dieses Projekt rechnet keine beliebigen Kanalformate automatisch um.

## Grenzwerte sind deine Entscheidung

PRTG muss einen ungesunden Zustand erkennen können: beispielsweise `Docker Engine = 0`, zu alte Backups oder eine volle Disk. Grenzwerte und Lookups unter den PRTG-Kanaleinstellungen passend zur eigenen Umgebung setzen. Ein erfolgreich ausgeführtes Script bedeutet nicht automatisch, dass jede gemessene Zahl gesund ist.

Danach mit [DockerAggregator](../DockerAggregator/README.md) fortfahren.
