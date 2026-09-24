# Projektstand und Versionsübersicht

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
