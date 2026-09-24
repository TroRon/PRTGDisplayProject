# Anzeige-Einstellungen ab Firmware 1.0.2

## Enthalten

- **Einheitliche Kartenhöhen:** Alle Systemkarten der geöffneten Ansicht passen sich an die höchste Karte an. Texte bleiben vollständig; Hinweis-Karten behalten ihre eigene Höhe.
- **Datenalter je System:** Älteste verfügbare PRTG-Messzeit aller Sensoren. Fehlt eine Messzeit, steht «Messzeit unbekannt». HTTP-Empfangszeit wird nicht als Messzeit ausgegeben. Demo ist ausdrücklich gekennzeichnet.
- **Optionaler Seitenwechsel:** Unter **Einstellungen → Panel → Anzeige** aktivieren. Intervall 10–60 Sekunden. Übersicht und aktive Rubriken wechseln; Berührung pausiert mindestens 60 Sekunden. Während Einstellungen und laufender oder noch zu bestätigender OTA-Installation bleibt der Wechsel pausiert.
- **Optionaler Nachtmodus:** Im selben Fenster Start-/Endstunde, Helligkeit 10–100 Prozent und UTC-Versatz einstellen. Beispiel: +60 Minuten Winter / +120 Minuten Sommer. Sommerzeit wird manuell umgestellt. Gleiche Start-/Endstunde gilt ganztägig. Ohne synchronisierte Uhr bleibt das Display hell. Berühren hellt für 60 Sekunden auf.

Die Einstellungsseite ist scrollbar; **Anzeige speichern** übernimmt die Werte dauerhaft. Schliessen verwirft ungespeicherte Eingaben. Beide Automatikfunktionen sind zunächst ausgeschaltet. Der Nachtmodus verwendet die vorhandene softwareseitige Abdunklung; keine Änderung des Backlight-Pinouts und keine Abschaltung der Hintergrundbeleuchtung.

## Bewusst nicht enthalten

Favoriten, automatische Störungswechsel und Verlaufsgrafiken sind für eine spätere Runde zurückgestellt. Sie sind nicht auswählbar und ihre Speicher-/Hintergrundlogik ist nicht im Release enthalten. Vor einer Wiederaufnahme steht Stabilität auf echter Hardware im Vordergrund.

## Update und technische Grenzen

Am Display **Firmware → Versionen prüfen → 1.0.2 → Installieren**, danach innert **120 Sekunden bestätigen**. Aggregator- und Direktkanal werden unterstützt. WLAN, Panel-Zugang und bestehende Einstellungen bleiben erhalten. Kein Werksreset und keine Änderung an Partitionen oder Board-Pinout.

Neue Optionen liegen im NVS-Namensraum `eagle-view`, Schlüssel `basic`; der bestehende vollständige Werksreset löscht auch diese Einstellungen. Das bisherige Snapshot-Limit unter 16 KiB bleibt durch einen Compile-Test abgesichert. Keine neue Netzwerkabfrage, kein Messwertarchiv und keine periodischen Flash-Schreibvorgänge.

Lokal geprüft: Modelltests, echtes LVGL bei 320×240 und 1024×600, Hardware-Einstellungsprüfung mit simuliertem NVS inklusive Speichern/Laden, Schreibfehler, Seitenwechsel/Touch-Pause und Nacht-Aufhellung sowie beide PlatformIO-Profile und ESP-IDF-Hardware-Build. Physische Dauerlauf-/Touch-/Nachtmodus-/OTA-Abnahme der neuen Version steht aus.
