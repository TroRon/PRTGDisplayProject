const states = { ok: 'OK', warning: 'WARNUNG', critical: 'KRITISCH', unknown: 'UNBEKANNT' };
const reasons = {
  SOURCE_UNAVAILABLE: 'PRTG nicht erreichbar oder Abfrage fehlgeschlagen',
  PRTG_NETWORK_OR_TLS: 'PRTG-Verbindung, TLS oder Zeitüberschreitung prüfen',
  PRTG_AUTH: 'PRTG-Zugriff verweigert; API-Key und Leserechte prüfen',
  PRTG_HTTP: 'PRTG meldet einen HTTP-Fehler',
  PRTG_JSON: 'PRTG-Antwort ist kein gültiges JSON',
  PRTG_SHAPE: 'PRTG-Antwort hat ein unerwartetes Format',
  SENSOR_MISSING: 'Sensor nicht gefunden', STALE: 'Messwerte veraltet',
  TIMESTAMP_MISSING: 'Zeitstempel fehlt', TIMESTAMP_IN_FUTURE: 'Zeitstempel in der Zukunft',
  CHANNEL_DATA_MISSING: 'Kanalwerte fehlen', PRTG_NOT_MONITORING: 'PRTG überwacht nicht aktiv',
  WAITING_FOR_DATA: 'Warte auf Daten', NOT_CONFIGURED: 'Sensor nicht konfiguriert'
};
const state = value => states[value] || 'UNBEKANNT';
const label = value => JSON.stringify(String(value).slice(0, 100));

export function createLogger(write = console.log, clock = () => new Date()) {
  return (level, message) => write(`${clock().toISOString()} [${level}] ${message}`);
}

// Log only selected status fields. Never serialize source records or errors.
export function createPollReporter(log) {
  let previous = new Map();
  return (snapshot, round, durationMs) => {
    const entities = Object.values(snapshot.categories).flatMap(c => c.entities);
    const sensors = entities.flatMap(e => e.sensors);
    const values = sensors.flatMap(s => Object.values(s.metrics));
    const counts = Object.keys(states).map(s => `${entities.filter(e => e.status === s).length} ${state(s)}`).join(', ');
    log(snapshot.overall === 'ok' ? 'INFO' : 'WARN',
      `Runde ${round} abgeschlossen (${durationMs} ms): ${entities.length} Systeme, ${sensors.length} Sensoren, ${values.filter(Number.isFinite).length} Kennzahlen; ${counts}; Gesamt: ${state(snapshot.overall)}; Daten ${snapshot.data_complete ? 'vollständig' : 'unvollständig'}.`);
    const next = new Map();
    for (const entity of entities) {
      const details = entity.sensors.filter(s => s.status !== 'ok' || s.reason).map(s =>
        `${label(s.key)}: ${state(s.status)} (${reasons[s.reason] || 'PRTG-Status oder Abfrage prüfen'})`).join('; ');
      const signature = `${entity.status}|${details}`;
      next.set(entity.id, signature);
      if (previous.get(entity.id) !== signature) {
        log(entity.status === 'ok' ? 'INFO' : 'WARN',
          `${previous.has(entity.id) ? 'Statusänderung' : 'Erster Status'} ${label(entity.label)}: ${state(entity.status)}${details ? '; ' + details : ''}.`);
      }
    }
    previous = next;
  };
}
