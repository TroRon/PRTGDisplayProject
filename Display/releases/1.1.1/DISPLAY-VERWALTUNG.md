# Zentrale Display-Verwaltung und Bedienbarkeit – Release 1.1.0

Stand 26.09.2026: zur Veröffentlichung freigegeben. Benötigt Display-Firmware **1.1.0** und Aggregator **1.3.0** mit aktiviertem WebAdmin und beschreibbarem `ADMIN_DIRECTORY`. Aggregator 1.3.0 produktiv installiert und healthy. Physische Display-Abnahme ausstehend.

## Einrichtung

1. Aggregator nach Freigabe aktualisieren; vorhandenes Admin-Verzeichnis beibehalten und sichern.
2. Firmware 1.1.0 per OTA oder USB installieren. WLAN, Panel-URL und Panel-Token bleiben erhalten. Nach OTA den Start bestätigen.
3. Im Aggregator anmelden und **Displays → Neu laden** öffnen. Das Panel meldet sich über seine bestehende HTTPS-Verbindung selbst an. Es muss kein eingehender Port am Panel geöffnet werden.
4. Beim gewünschten Gerät **Vom Aggregator** wählen, Werte einstellen und **An Display senden** bestätigen.
5. Nach etwa einer Minute erneut laden. **Ausstehend** bedeutet: gespeichert, aber noch nicht bestätigt. Erst **Übernommen** bestätigt die passende Revision und die gemeldeten Einstellungen. Offene Einstellungen, OTA und aktuelle Touch-Bedienung können die Übernahme verzögern.

Konfigurierbar: Anzeigename, automatischer Seitenwechsel (10–60 Sekunden), Wechsel bei neuen kritischen Störungen, Nachtmodus mit Stunden und Helligkeit, fester UTC-Versatz, RAM-Verläufe, Favoriten und Favoriten-Startübersicht. WLAN, Passwörter, Panel-Token, Serveradresse und OTA-Installation gehören nicht zu diesem Kanal. Keine automatische Sommerzeit-Umschaltung.

Die Geräte-ID unterscheidet mehrere Panels mit gleichem Namen. Bis zu 32 Geräte sind vorgesehen. Ein Werksreset erzeugt eine neue Identität; alte Einträge werden nicht automatisch gelöscht. Eine Löschfunktion ist in diesem Kandidaten noch nicht enthalten.

## Lokal, zentral und offline

Standard ist lokale Verwaltung. Zentrale Einstellungen werden separat in NVS gespeichert und gelten nach einem Neustart auch ohne Verbindung. Die vorherigen lokalen Anzeigeoptionen bleiben erhalten. **Lokal verwalten** am Display löst die zentrale Verwaltung auch offline; bei nächster Verbindung wird dies dem Aggregator gemeldet. Beim Zurückschalten gelten wieder die vorherigen lokalen Optionen und der lokale Panelname.

Ein Wechsel der Panel-Serveradresse löst die zentrale Konfiguration des bisherigen Servers. Favoriten eines anderen Servers werden nicht übernommen. Ein Werksreset löscht auch die zentrale Konfiguration und den individuellen Geräteschlüssel.

## Speicherung, Sicherheit und Fehler

Der Aggregator speichert Sollzustand und Revision atomar in `ADMIN_DIRECTORY/displays.json`. Diese Datei sichern und mit denselben Benutzer-/Administratorrechten wie die übrigen Admin-Daten schützen. Der individuelle Geräteschlüssel wird dort nur als SHA256 gespeichert. Letzter Kontakt und Istzustand sind flüchtig und werden nach einem Aggregator-Neustart neu gemeldet.

Die bestehende Panel-Bearer-Authentifizierung wird um einen zufälligen Schlüssel pro Gerät ergänzt. Der erste Kontakt registriert das Gerät; dies setzt den Panel-Zugang voraus. Admin-Schreibzugriffe verwenden die vorhandene Anmeldung und Origin-Prüfung. TLS-Zertifikate werden geprüft; keine Weiterleitungen. Keine Zugangsdaten in Statusmeldungen oder Logs.

Ohne Kontakt länger als drei Minuten erscheint das Gerät offline. Speicher-/Validierungsfehler werden als Fehler gemeldet; die UI bleibt mit dem bisherigen Stand aktiv. Bei einem zurückgespielten Aggregator-Backup mit älterer Revision verhindert das Panel eine unbemerkte Rücknahme. In diesem Fall am Gerät **Lokal verwalten** wählen und danach die zentrale Konfiguration erneut einrichten.

## Optimierung der Bedienung

Der Hardware-Banner wird nicht mehr alle 10 Millisekunden neu gesetzt, sondern im Sekundenrhythmus aktualisiert. Unveränderter Titeltext wird nicht erneut gesetzt. Neue Messzeitstempel allein löschen und erstellen nicht mehr sämtliche Systemkarten. Sichtbare Werte, Status, Reihenfolge und Details aktualisieren die Karten weiterhin. Datenalter wird weiterhin regelmässig aktualisiert.

Die zentrale Abfrage läuft höchstens einmal pro Minute im vorhandenen Netzwerk-Worker. Sie startet keine zusätzliche Task und führt HTTP/TLS nicht im LVGL-Thread aus. Empfangsbuffer wird wiederverwendet, grössere Konfigurationsobjekte liegen in PSRAM. NVS wird bei Übernahme oder Freigabe geschrieben, nicht bei jedem Poll. Übernahme erst nach kurzer Bedienpause und ausserhalb Einstellungen/OTA.

## Prüfungen und Grenzen

Automatisiert geprüft: Aggregator-Authentifizierung, Revisionen, Konflikte, Bestätigung, Offlinezustand, Neustart und lokale Freigabe; Protokoll-Typen, Grenzen, Umlaute und erlaubte Felder; native LVGL-/Einstellungsregressionen einschliesslich Karten-Neuzeichnung. Browserprüfung mit ausschliesslich synthetischen Geräten auf Desktop und Mobilformat. Beide PlatformIO-Profile und ESP-IDF-Hardware-Build gehören zur Kandidatenprüfung.

Noch erforderlich: tatsächliche Reaktionszeit auf dem Waveshare-Gerät, Übernahme mit laufender Touch-Bedienung, Neustart/offline, lokale Freigabe und OTA-Rückweg physisch prüfen. Ein erfolgreicher Build oder Browserlauf ist kein Ersatz dafür. Keine Veränderung an Display-/Touch-Pins, Partitionen oder archiviertem Simulator 0.3.
