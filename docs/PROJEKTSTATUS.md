# Projektstand und Versionsübersicht

## Firmware 1.0.2 – schlanke Anzeige-Erweiterung

- Einheitlich hohe Systemkarten mit vollständigem Text.
- Datenalter je System aus den PRTG-Messzeitstempeln.
- Optionaler Seitenwechsel mit Touch-/Einstellungs-/OTA-Pause.
- Optionaler softwareseitiger Nachtmodus mit Helligkeit, Stunden und manuellem UTC-Versatz.

Einrichten unter **Einstellungen → Panel → Anzeige**. Beide Automatiken sind zunächst ausgeschaltet. Favoriten, automatische Störungswechsel und Verlaufsgrafiken wurden bewusst zurückgestellt; ihre Speicher-/Hintergrundlogik ist nicht im Release enthalten.

**Update:** Versionen prüfen, **1.0.2 installieren**, nach dem Neustart innert **120 Sekunden bestätigen**. Direktkanal und Aggregator werden unterstützt. WLAN/Panel-Zugang bleiben erhalten; kein Werksreset nötig. Board Waveshare ESP32-S3-Touch-LCD-5B SKU 28151, ESP-IDF 5.5.0, bestehendes OTA-Layout unverändert. Frühere Pakete bleiben erhalten.

Modell-/LVGL-/Einstellungsprüfungen, beide PlatformIO-Profile und Hardware-Build lokal erfolgreich. Paket-Hashes und Signatur geprüft. Physischer Geräte-/Dauerlauftest von 1.0.2 steht aus.

## Aggregator 1.2.0 und Display 1.0.1

Systemverwaltung und Übersicht sind nach Infra, Backup, Docker, Netzwerk und Dienste gruppiert. Die Verwaltung zeigt aufklappbare Rubriken mit Anzahl aktiver Systeme. «System hinzufügen» übernimmt die Rubrik; «Nach oben/unten» verschiebt innerhalb dieser Rubrik. Deaktivierte Systeme bleiben ausgegraut und ausdrücklich markiert, damit sie wieder aktiviert werden können.

Änderungen gelten erst nach **Prüfen und speichern**. Die stets erreichbare Speicherleiste markiert offene Änderungen deutlich; «Verwerfen / neu laden» holt den gespeicherten Stand zurück. Die Übersicht zeigt nur gespeicherte aktive Systeme. Kategoriezuordnung und alle bisherigen Konfigurationswerte bleiben erhalten.

Display **1.0.1** übernimmt innerhalb seiner Rubriken die konfigurierte Reihenfolge. Die vorherige Firmware 1.0.0 R3 sortiert zusätzlich nach Status; für die korrekte Reihenfolge ist deshalb das Update erforderlich. Farben und Hinweise zeigen Fehler weiterhin. Deaktivierte Systeme verschwinden nach dem Speichern und dem nächsten gültigen API-Snapshot. Bei Verbindungsausfall können letzte Einträge als unbekannt sichtbar bleiben.

**Installation:** Aggregator aktualisieren; am Display unter Firmware die Versionen prüfen, **1.0.1 installieren** und nach dem Neustart innert **120 Sekunden bestätigen**. Direktkanal und Aggregator sind unterstützt. WLAN, Panel-Zugang und Einstellungen bleiben erhalten. Keine Partition-/Pinout-Änderung, kein Werksreset nötig.

Geprüft: 31 Aggregator-Tests, Desktop-/Mobilbrowser mit Gruppierung, Reihenfolge, Aktivierung, Speichern/Verwerfen und sichtbarer Speicherleiste; native LVGL- und Modelltests, beide PlatformIO-Profile, ESP-IDF-5.5.0-Hardware-Build und Paketprüfsummen. Physische Abnahme von Firmware 1.0.1 steht aus. Frühere Firmware-Releases bleiben unverändert.

Stand: 24. September 2026. Softwareversionen von Aggregator und Display sind unabhängig. Diese Übersicht beschreibt nachgewiesene Ergebnisse; ein lokaler Test ist keine Geräteabnahme.

| Bestandteil | Stand | Prüfung / Verfügbarkeit |
|---|---|---|
| PRTG-Vorlagen | Proxmox, PBS, Docker-Host | Allgemeine Quellen im Repo; eigene Umgebung separat abnehmen |
| Aggregator 1.0.0 | WebAdmin, Konfiguration, Systemnamen, signierter Firmware-Upload/Katalog | Öffentlich verfügbar; Referenzinstallation mit HTTPS, PRTG, Persistenz und Dateirechten geprüft |
| Aggregator 1.1.0 | Systeme sortieren/kopieren, Passwort-Anmeldung, GitHub-Katalog und Import | Freigegeben; 31 private Tests, 16 Tests der neutralen Distribution, Desktop/Mobilbrowser. Referenzinstallation produktiv aktualisiert |
| Display 1.0.0 R3 | Setup-QR-Code/Captive Portal, WLAN-Suche, WebAdmin, Reset, Info, Demo nach 50 Sekunden | Öffentliches USB-/OTA-Paket; lokal gebaut/geprüft. Physische Abnahme der neuen Setup-/Reset-Funktionen offen |
| Bestätigter Gerätetest | Firmware 0.8.1 | Betreiber bestätigt OTA-Installation und Ausblenden inaktiver Rubriken |
| OTA-Direktkanal | 1.0.0 R3 und 0.9.1 | Signierte Angebote; Downgrade/Neuinstallation auswählbar |
| Gezielter Rollback / Aggregator-OTA am Gerät | Noch offen | Server-Downloadprüfung ist kein durchgeführter Geräte-Update |
| Gehäuse | Privater Prototyp | Keine freigegebene druckfertige öffentliche Konstruktion |

## Was das Paket heute leistet

PRTG erfasst Zustände. Der Aggregator übersetzt konfigurierte Sensoren in eine geschützte Health-API. Das Display zeigt Bereiche, Systeme und Messwerte, behält bei Fehlern einen nachvollziehbaren unbekannten Zustand und bietet eine gekennzeichnete Vorführ-Demo. Alle Infrastrukturadressen und Zugangsdaten werden bei der Installation gesetzt.

**1.1.0 ist eine Aggregator-Erweiterung.** Es wird dafür keine neue Display-Firmware benötigt. Quellcode, Dokumentation und synthetische Bilder gehören zum freigegebenen Aggregator-Stand. [Neue Bedienung im Detail](../DockerAggregator/VERSION-1.1.md).

## Grenzen und offene Arbeiten

- Unterstütztes Zielboard ausschliesslich Waveshare ESP32-S3-Touch-LCD-5B, SKU 28151.
- Display: maximal 24 Systeme und 32 KiB API-Antwort. Aggregator kann bis 64 Systeme konfigurieren; für dieses Display innerhalb dessen engerer Grenzen bleiben.
- Fünf feste Bereichsschlüssel; Namen der Systeme frei, keine beliebigen neuen Bereichstypen über das Formular.
- Keine separate Pulse-Integration und keine automatische Geräte-/Sensor-Erkennung im WebAdmin. Netzwerk/Dienste können mit passenden PRTG-Sensoren manuell zugeordnet werden.
- Keine automatische Installation auf allen Displays, kein zentraler Gerätebestand und keine erfundene Anzeige einer am Gerät installierten Firmwareversion im Aggregator.
- Pro Aggregator-Datenverzeichnis genau eine schreibende Instanz; keine Mehrbenutzerverwaltung oder Hochverfügbarkeit.
- Nachtmodus, Präsenzsteuerung und automatische Helligkeit sind keine zugesicherten aktuellen Funktionen.
- Physische Smartphone-/Hotspot-/Reset-/Rollback-Abnahme und unabhängige Neuinstallation nach Anleitung bleiben erforderlich.
- Projektlizenz für eigenen Code ist noch offen. Drittanbieter-Lizenzen gelten unverändert; öffentliche Sichtbarkeit ist keine pauschale Nutzungsfreigabe.

## Dokumentationskarte

[Gesamtes System in Betrieb nehmen](HANDBUCH.md) · [Bedienung, Wartung und Fehlerhilfe](BETRIEB.md) · [Architektur und Entwicklung](ENTWICKLUNG.md) · [Änderungshistorie](../CHANGELOG.md) · [Veröffentlichungsregeln](../WORKFLOW.md).
