# Pulse-Alarme – Aggregator 1.4.0 / Display 1.2.0

Release 1.4.0 / 1.2.0, 28.09.2026. Veröffentlichung und Deployment freigegeben. Der neue Adapter wurde lesend mit Pulse 5.1.35 geprüft; keine Änderungen an Pulse vorgenommen. Frühere Firmware-Releases bleiben unverändert.

## Einrichtung

1. Aggregator auf 1.4.0 aktualisieren, Admin-Verzeichnis und Secrets erhalten.
2. Unter **Systeme / PRTG → Pulse-Alarme** aktivieren, Serveradresse und Anzeigename setzen. Standard ist deaktiviert. HTTPS bevorzugen. Eine interne HTTP-Verbindung erfordert das ausdrückliche Kontrollkästchen und überträgt den Token unverschlüsselt.
3. Eigenes lesendes Pulse-API-Token eingeben und **Prüfen und speichern** wählen. Leer lassen behält den bestehenden Token; bei geänderter Serveradresse muss ein neuer Token eingegeben werden. Tokens werden nie an den Browser zurückgegeben. Sie liegen geschützt in `ADMIN_DIRECTORY/settings.json`; alternativ ist `PULSE_API_TOKEN_FILE` als serverseitige Secret-Datei unterstützt. Vorhandene Admin-Dateien werden kompatibel geladen.
4. **Gespeicherte Pulse-Verbindung prüfen** ausführen. Danach zeigt die Übersicht die neue Quelle unter Dienste.
5. Display 1.2.0 installieren und OTA-Start bestätigen. Für die Alarmtexte und deutschen Zählerbezeichnungen ist diese Firmware vorgesehen. Ältere Firmware kann den Status und numerische Werte lesen, zeigt aber noch nicht die Meldungstexte. Kein Werksreset nötig.

Für die private Entwicklung gibt `scripts/local/Initialize-Pulse.ps1` unter PowerShell 7 den Token verdeckt ein. Ablage `secrets/pulse_api_token.txt`, von Git ausgeschlossen, keine automatische Übertragung. `-Replace` ersetzt bewusst den vorhandenen Token. Das produktive Secret wird erst bei freigegebenem Deployment übertragen.

## Was auf dem Display erscheint

Eine Übersicht mit kritischen, warnenden und unbekannten Alarmen sowie Gesamtzahl; dazu bis zu sechs Detailkarten, zuerst kritisch, dann unbekannt, dann Warnung. Je Alarm: Systemname, Text, Beginn in UTC und Kennzeichnung «Quittiert». Quittierte Alarme gelten weiterhin als aktiv; das Panel quittiert, löscht oder verändert keine Pulse-Alarme.

Der Anzeigename der Übersicht ist frei wählbar. Weitere Alarme werden gezählt; die vollständige Liste bleibt in Pulse. Das Display unterstützt insgesamt 24 Systeme: bei aktivem Pulse höchstens 23 vorhandene Systeme, die übrigen freien Plätze werden für Details genutzt. Die Übersicht bleibt immer erhalten. IDs mit `pulse-` sind reserviert. Abschalten entfernt Pulse aus der Anzeige, behält aber Konfiguration und Zugang.

Keine aktiven Alarme bedeutet ausschliesslich: Pulse meldet momentan keine aktiven Alarme. Das bestätigt weder vollständige Überwachungsabdeckung noch Erreichbarkeit sämtlicher Anwendungen. PRTG-Systemkarten behalten ihre eigenen Zustände. Doppelte Meldungen werden nicht automatisch unterdrückt.

## Abfrage und Fehlerverhalten

Ausschliesslich GET `/api/state/summary` und GET `/api/alerts/active`, mit `X-API-Token`, ohne Redirects und mit normaler TLS-Zertifikatsprüfung. Beide Endpunkte sind auf der geprüften Installation verfügbar. Es wird nicht der grosse vollständige Infrastruktur-/Backup-Datensatz abgefragt.

Pulse nutzt dieselbe Abfragepause und Veraltungsgrenze wie PRTG. Die Pause beginnt nach Abschluss der Runde. Anfragen laufen parallel zur PRTG-Abfrage; Timeout pro Pulse-Anfrage maximal 30 Sekunden, Antwort höchstens 512 KiB, höchstens 512 Alarme. Die API-Zusammenfassung muss einen aktuellen `lastUpdate` liefern. Empfangszeit und Quellenzeit werden geprüft; mehr als 60 Sekunden Zukunft oder veraltete Werte gelten als unbekannt. Der Beginn eines lange aktiven Alarms ist kein Veraltungsindikator.

Alarmliste und Zusammenfassung müssen dieselbe Anzahl nennen. Bei einem Alarmwechsel zwischen den beiden Abfragen kann deshalb vorübergehend «Unbekannt» erscheinen; nächste Runde prüft erneut. Unbekannte Schweregrade werden nicht als gesund behandelt. Authentifizierungs-, HTTP-, JSON-, Format-, Grössen- und Zeitfehler erzeugen eine unbekannte Pulse-Übersicht. Bekannte kritische PRTG-Zustände bleiben sichtbar. Keine Weiteranzeige alter Pulse-Alarmkarten als aktuelle Daten.

Bei einem Ausfall nur von Pulse bleiben PRTG und die Live-Anzeige aktiv. Pulse-Fehler führen bewusst nicht in die Demo; die Quelle wird als unbekannt angezeigt. Im lokalen Aggregator-Demo-Modus entstehen ausschliesslich gekennzeichnete synthetische Alarme, ohne Netzwerkzugriff auf Pulse.

## Speicherung und Betrieb

Nur der Aggregator besitzt das Pulse-Token. Das Display verwendet weiterhin seinen bestehenden Panel-Zugang. Secret-Mounts schreibgeschützt, Admin-Verzeichnis mit Zugriff für Container und Administrator sichern. Logs enthalten Anzahl/Fehlercode, keine Token, Alarmtexte oder Request-URLs. Ein Quellenwechsel verlangt ein neues Token, damit der bisherige Zugang nicht an einen anderen Server weitergegeben wird.

Gespeicherte Konfigurationen starten nach Änderungen zunächst unbekannt. Späte Ergebnisse einer alten Konfiguration können die neue nicht überschreiben. Die externe Pulse-Adresse und echte Tokens gehören nicht in öffentliche Vorlagen, Bilder oder Firmware.

## Prüfung

Aggregator-Regressionen und zusätzliche Prüfungen für Token-Schutz, Migration, Quellenwechsel, API-Fehler, Veraltung, Grenzen, Sortierung und verspätete Ergebnisse. Desktop-/Mobilbrowser mit synthetischen Daten: Aktivieren, Token verdeckt eingeben, speichern, kein Token-Rückfluss, Alarmübersicht. Lesender Adapter-Test mit realer Pulse-Installation erfolgreich. Modell-/LVGL-/Einstellungstests, beide PlatformIO-Profile und ESP-IDF-Hardware-Build; physische Alarmdarstellung/OTA-Abnahme noch ausstehend.
