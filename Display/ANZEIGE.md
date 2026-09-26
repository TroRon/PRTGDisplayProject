# Anzeige-Einstellungen: Release 1.0.3

**Stand 25.09.2026:** 1.0.2 lief laut Betreiber sauber über Nacht. Die nachfolgenden Erweiterungen sind implementiert und zur Veröffentlichung freigegeben und benötigt eine eigene Geräteabnahme. Pinout, Partitionen und frühere Release-Dateien bleiben unverändert.

## Neue Funktionen in 1.0.3

Unter **Einstellungen → Panel → Anzeige** nach unten scrollen, gewünschte Optionen auswählen und **Anzeige speichern** antippen. Neue Funktionen sind zunächst ausgeschaltet. Nachtmodus und Seitenwechsel aus 1.0.2 werden beim ersten Laden übernommen.

- **Favoriten:** Systeme anhand ihrer stabilen Aggregator-ID auswählen, optional als Startübersicht verwenden. **Favoriten / Alle** unten links schaltet die Übersicht um. Rubriken zeigen weiterhin alle zugehörigen aktiven Systeme; Gesamtstatus und Hinweise werden nicht gefiltert. Umbenennen erhält die Auswahl, entfernte/deaktivierte Systeme erscheinen nicht. Bei einer anderen Aggregator-Adresse wird die alte Auswahl nicht angewendet. Demo bleibt vollständig sichtbar und liefert keine auswählbaren Favoriten.
- **Neue kritische Störungen:** Optional zur Rubrik eines neu kritisch gewordenen Systems wechseln. Bereits beim Start bzw. nach Verbindungsausfall kritische Systeme bilden zunächst die Ausgangslage; unveränderte Alarme lösen keine Wiederholungen aus. Touch pausiert mindestens 60 Sekunden, Einstellungen/Dialoge sowie laufendes oder noch zu bestätigendes OTA pausieren ebenfalls. Erholung entfernt einen wartenden Wechsel. Höchstens ein automatischer Störungswechsel pro Minute; mehrere gleichzeitige Störungen bleiben über Hinweise sichtbar.
- **Kleine Messwertverläufe:** Optional bis zu 16 echte Messzeitpunkte je System sammeln. Der erste numerische Messwert aus der Sensor-/Kennzahlenreihenfolge wird verwendet; seine Sensor-ID/Kennzahl steht beim Diagramm. Keine freie Kennzahlenauswahl. Kennzahlenwechsel beginnt einen neuen Verlauf. **Verlauf** auf einer Systemkarte öffnet eine Momentaufnahme; zum Aktualisieren schliessen und erneut öffnen. Y-Werte sind Rohwerte aus der API, Minimum/Maximum/letzter Wert werden genannt. X zeigt Beobachtungsreihenfolge, kein gleichmässiges Zeitraster. Ausfälle, ungültige Messungen und Abstände über drei Minuten unterbrechen die Linie. Demo, unbekannte Zustände, zukünftige/alte Werte und doppelte Messzeitpunkte werden nicht als neue Messungen gespeichert. Keine zusätzlichen PRTG-Abfragen.
- **WLAN-Hinweis:** Display-Einrichtung, Suchfenster und WebAdmin erklären die Unterstützung von 2,4-GHz-WLAN. Der ESP32-S3 unterstützt kein 5-GHz-WLAN.

## Speicher und Persistenz

Verläufe benötigen genau einen begrenzten PSRAM-Puffer (8.228 Byte) erst nach Aktivierung. Ohne verfügbaren Speicher bleibt die Live-Anzeige bedienbar und die Aktivierung meldet einen Fehler. Deaktivieren gibt den Puffer frei; Neustart, andere Quelle oder entfernte Systeme verwerfen die entsprechenden Messpunkte. **Keine Verlaufssicherung im Flash** und keine periodischen NVS-Schreibvorgänge. Langzeitdaten bleiben in PRTG.

Nur explizit gespeicherte Anzeigeoptionen/Favoriten werden im NVS-Namensraum `eagle-view` unter `options` gespeichert. Der alte Schlüssel `basic` wird als Migrationsquelle erhalten. Ein Downgrade auf 1.0.2 verwendet somit die zuletzt dort gespeicherten Grundeinstellungen, nicht neuere Änderungen aus 1.0.3. Werksreset löscht beide Schlüssel. Netzwerkzugang und OTA-Konfiguration bleiben unabhängig davon erhalten.

Snapshot: 16.288 Byte, weiterhin Compile-Grenze unter 16 KiB. Bestehende Netzwerk-Snapshots liegen weiterhin in PSRAM; kein weiterer Task und keine Änderung an RGB-/Touch-/Framebuffer-Konfiguration. Zusätzliche Options-/ID-Metadaten und LVGL-Objekte benötigen ebenfalls RAM. Ein realer Heap-/Dauerlauftest ist deshalb weiterhin nötig.

## Prüfungen und Geräteabnahme

Lokale Regression: Datenmodell (inklusive Zeit-/Identitätsgrenzen), LVGL 320×240 und 1024×600, Favoriten/Umbenennung/Quellenwechsel, Verlaufslimit/Duplikate/Demo/Lücken/Kennzahlenwechsel, Allokations-/Schreibfehler, NVS-Migration, Touch-/OTA-/Erholungspause sowie bestehende Setup-/Reset-/OTA-Policies. Beide PlatformIO-Profile und ESP-IDF-Hardware-Build gehören zur Kandidatenprüfung. Lokale Tests simulieren NVS/Hardware und ersetzen keine Geräteabnahme.

Nach Freigabe bzw. lokalem Test: OTA installieren und innert 120 Sekunden bestätigen. Danach Favoriten auswählen, reale Messpunkte sammeln, Touch-/Nacht-/Störungswechsel prüfen und über mehrere Stunden freie Heap-Blöcke/Systeminformationen beobachten. Bei Neustart sind Verläufe erwartungsgemäss leer. 1.0.2 bleibt Rückweg.

---

# Historischer Release 1.0.2

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
