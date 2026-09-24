import { createFirmwareService } from './firmware.mjs';
import { fileURLToPath } from 'node:url';
import { readConfig, readSecret } from './config.mjs';
import { PrtgClient, demoCollect } from './prtg.mjs';
import { Collector } from './health.mjs';
import { createApi, parseAllowedClients } from './server.mjs';
import { createLogger, createPollReporter } from './logging.mjs';

const log = createLogger();

try {
  const args = process.argv.slice(2);
  if (args.some(a => !['--demo', '--discover'].includes(a)) || args.length > 1) throw new Error('INVALID_ARGUMENTS');
  const demo = args.includes('--demo');
  const config = readConfig(process.env.CONFIG_FILE || fileURLToPath(new URL('../config/aggregator.example.json', import.meta.url)));
  const client = demo ? null : new PrtgClient(config, readSecret(process.env, 'PRTG_API_TOKEN'));
  if (args.includes('--discover')) {
    const rows = await client.discover();
    // Only allowlisted metadata, no messages, tokens or request URLs.
    console.log(JSON.stringify(rows.map(r => ({ id: r.objid, device: r.device, sensor: r.sensor, status_raw: r.status_raw, lastcheck_raw: r.lastcheck_raw })), null, 2));
  } else {
    const host = process.env.HOST || '127.0.0.1';
    const port = Number(process.env.PORT || 8787);
    if (!Number.isInteger(port) || port < 1 || port > 65535) throw new Error('INVALID_PORT');
    const panelToken = readSecret(process.env, 'PANEL_API_TOKEN', !demo || !['127.0.0.1', '::1'].includes(host));
    const collector = new Collector(config, demo ? demoCollect : s => client.collect(s), demo ? 'demo' : 'live');
    const firmware = process.env.OTA_DIRECTORY ? createFirmwareService({ directory: process.env.OTA_DIRECTORY, adminToken: readSecret(process.env, 'OTA_ADMIN_TOKEN'), panelToken, log }) : null;
    const server = createApi(collector, panelToken, parseAllowedClients(process.env.ALLOWED_CLIENT_IPS), firmware);
    server.requestTimeout = firmware ? 120000 : 10000;
    server.headersTimeout = 10000;
    await new Promise((resolve, reject) => { server.once('error', reject); server.listen(port, host, resolve); });
    let stopped = false;
    let timer;
    let round = 0;
    const report = createPollReporter(log);
    log('INFO', `EagleNET Aggregator: ${demo ? 'DEMO (synthetische Daten)' : 'LIVE'}; Port ${port}; Abfragepause ${config.poll_seconds} s; veraltet nach ${config.stale_seconds} s; API-Token-Schutz ${panelToken ? 'aktiv' : 'inaktiv (lokale Demo)'}. Zeitstempel in UTC.`);
    const tick = async () => {
      const started = performance.now();
      round++;
      log('INFO', `Runde ${round}: ${demo ? 'Demo-Daten erzeugen' : 'PRTG-Abfrage gestartet'}.`);
      try {
        await collector.poll();
        report(collector.snapshot(), round, Math.round(performance.now() - started));
      }
      catch { log('ERROR', `Runde ${round}: Verarbeitung fehlgeschlagen; nächste Abfrage nach ${config.poll_seconds} s.`); }
      if (!stopped) timer = setTimeout(tick, config.poll_seconds * 1000);
    };
    void tick();
    for (const signal of ['SIGTERM', 'SIGINT']) process.on(signal, () => {
      log('INFO', `${signal}: Aggregator wird beendet.`);
      stopped = true; clearTimeout(timer); server.close();
      setTimeout(() => process.exit(0), 1500).unref();
    });
  }
} catch {
  // Never log exception details: fetch/config/secret errors can contain credentials.
  log('ERROR', 'START_FAILED: Konfiguration, Secret-Dateien, Port und PRTG-Verbindung prüfen.');
  process.exitCode = 1;
}
