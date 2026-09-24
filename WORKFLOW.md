# Entwicklung und Freigabe

**Aktuelle Freigabe:** Aggregator 1.1.0 samt gesamter Dokumentation und synthetischen Bildern ausdrücklich für Übernahme, Commit/Push beider Repos und Deployment freigegeben. Künftige Änderungen benötigen erneut Freigabe. [Projektstand](docs/PROJEKTSTATUS.md).

Dieses Repository ist die öffentliche Distribution mit drei Bereichen: **PRTG Sensoren**, **DockerAggregator** und **Display**. Es enthält geprüfte Quellen, neutrale Vorlagen, Anleitungen und unveränderliche Releases. Es ist kein Arbeitsverzeichnis für Experimente, lokale Konfiguration oder Zugangsdaten.

## Verbindliche Freigabe

1. Änderungen und Release-Kandidaten zuerst ausserhalb dieser öffentlichen Arbeitskopie entwickeln und testen.
2. Vor jeder Übernahme ins öffentliche Repository den Benutzer ausdrücklich fragen. Die Frage nennt Version, Dateien, Testergebnisse und den vorgesehenen Umfang: Übernahme, Commit, Push und gegebenenfalls GitHub-Release/OTA-Katalog.
3. Eine frühere Freigabe gilt nur für den damals genannten Stand. Ein neuer Firmware-Auftrag oder eine erfolgreiche Geräteabnahme ist keine Freigabe für die nächste Veröffentlichung. Bei Änderungen ausserhalb des freigegebenen Umfangs erneut fragen.
4. Erst nach Freigabe die ausgewählten Dateien übernehmen. Bestehende Releases niemals ersetzen. Privatsphäre, Paket-Hashes, Signaturen, Diff und Dateiliste prüfen. Keine private Git-Historie übernehmen.
5. Nur die geprüften Dateien committen und pushen. Öffentliche Downloads, Remote-Commit und OTA-Metadaten anschliessend prüfen. Ein Git-Push ist keine Freigabe für ein Server-Deployment.

Eine ausdrückliche Anweisung, die konkrete Änderung zu veröffentlichen, erfüllt Schritt 2 bereits. Sie ist keine dauerhafte Erlaubnis für spätere Änderungen.

## Lokale Push-Sperre

Einmal pro Arbeitskopie aktivieren:

```sh
git config core.hooksPath .githooks
```

Der `pre-push`-Hook verlangt eine lokale Freigabedatei unter dem von `git rev-parse --git-path publication-approved-commit` ausgegebenen Pfad. Sie enthält exakt die geprüfte Commit-ID. Die verantwortliche Person beziehungsweise der Agent darf sie **erst nach ausdrücklicher Benutzerfreigabe für diesen Stand** schreiben. Jeder neue Commit macht die alte Freigabe unwirksam. Der Hook prüft auch, dass alle gepushten Referenzen auf diesen Commit zeigen; Löschungen sind gesperrt.

Dies ist eine lokale Fehlbedienungssperre, keine serverseitige Zugriffskontrolle. Hooks werden nicht automatisch bei einem Clone aktiviert. `--no-verify` oder das Abschalten des Hooks darf den Freigabeprozess nicht umgehen. Die Sperre ersetzt insbesondere nicht die Nachfrage **vor der Dateiübernahme** und gilt nicht technisch für GitHub-API-Aufrufe: auch diese benötigen die ausdrückliche Freigabe.

## Verzeichnisregeln

- `Display/firmware/`: freigegebener Quellstand, keine generierten Builds oder Toolchains.
- `Display/releases/<Version>/`: unveränderliche USB-/OTA-Pakete mit Dokumentation und SHA256.
- `Display/ota/`: freigegebener Direktkanal. Änderungen an Manifest/Katalog machen Angebote unmittelbar verfügbar.
- `DockerAggregator/`: freigegebene Software und neutrale Konfiguration; keine Betriebsdaten.
- `PRTG Sensoren/`: Anleitung und Beispiele; keine privaten Inventare.

Alte OTA-Dateien bleiben erhalten, solange veröffentlichte Manifeste oder unterstützte Geräte darauf verweisen. Neue Builds werden niemals über eine bestehende Version geschrieben.

## Freigegebene Bereinigung

Auf ausdrücklichen Betreiberwunsch wurden die öffentlichen Releases vor 0.9.1 samt Download-Paketen und OTA-Angeboten entfernt. Aktuell wird nur 0.9.1 angeboten. Private Archive und Git-Historie bleiben erhalten. Dies ist eine konkrete Ausnahme zur Aufbewahrung, keine allgemeine Löschfreigabe für spätere Releases.

Nachfolgend ausdrücklich freigegeben: Release 1.0.0. Der Katalog enthält nun 1.0.0 und 0.9.1; die zuvor entfernten älteren Releases bleiben entfernt.

## Ausdrücklich freigegebener Paket-Ersatz

Der Betreiber hat den Ersatz von 1.0.0 durch Revision 2 bei unveränderter Firmware-Versionsnummer ausdrücklich genehmigt. Ausnahme für dieses Paket: öffentlicher Ordner/USB-Paket und OTA-Angebot werden ersetzt, der ursprüngliche Stand bleibt privat archiviert. Git-Tag v1.0.0-r2 kennzeichnet die korrigierten Quellen; keine Git-Historie und kein bestehender Tag wird umgeschrieben. Diese Ausnahme gilt nicht automatisch für spätere Veröffentlichungen.

Erneute ausdrückliche Freigabe für **1.0.0 R3**: QR-Code und Captive Portal samt Anleitung, Commit/Push und Ersatz des öffentlichen 1.0.0-Pakets/OTA-Angebots. Separater Quelltag v1.0.0-r3; private R1/R2-Archive und bestehende Tags bleiben erhalten. Keine allgemeine Freigabe für spätere Änderungen.
