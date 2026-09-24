import test from 'node:test';
import assert from 'node:assert/strict';
import { createLogger, createPollReporter } from '../src/logging.mjs';

test('poll logs show initial state, failures, changed reasons and recovery without repeated detail or source secrets', () => {
  const lines = [];
  const report = createPollReporter(createLogger(s => lines.push(s), () => new Date('2026-09-22T17:00:00Z')));
  const snapshot = { overall: 'ok', data_complete: true, categories: { compute: { entities: [
    { id: 'server-a', label: 'SERVER-A\nforged line', status: 'ok', sensors: [{ key: 'health', status: 'ok', reason: null, metrics: { cpu: 0 }, raw: 'secret-token' }] }
  ] } }, error: 'https://example/?apitoken=secret-token' };
  report(snapshot, 1, 20);
  assert.equal(lines.length, 2);
  assert.match(lines[0], /1 Kennzahlen.*Daten vollständig/);
  report(snapshot, 2, 10);
  assert.equal(lines.length, 3);
  const e = snapshot.categories.compute.entities[0];
  snapshot.overall = e.status = e.sensors[0].status = 'unknown';
  snapshot.data_complete = false;
  e.sensors[0].reason = 'SOURCE_UNAVAILABLE';
  e.sensors[0].metrics.cpu = null;
  report(snapshot, 3, 10000);
  assert.match(lines.at(-1), /Statusänderung.*PRTG nicht erreichbar/);
  assert.match(lines.at(-2), /0 Kennzahlen.*unvollständig/);
  e.sensors[0].reason = 'secret-token';
  report(snapshot, 4, 10);
  assert.match(lines.at(-1), /PRTG-Status oder Abfrage prüfen/);
  snapshot.overall = e.status = e.sensors[0].status = 'ok';
  e.sensors[0].reason = null;
  snapshot.data_complete = true;
  report(snapshot, 5, 10);
  assert.match(lines.at(-1), /Statusänderung.*OK/);
  assert.ok(lines.every(line => line.startsWith('2026-09-22T17:00:00.000Z [')));
  assert.ok(lines.every(line => !line.includes('\n') && !line.includes('secret-token')));
});
