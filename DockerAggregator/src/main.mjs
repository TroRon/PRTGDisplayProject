import {createDisplayStore} from './displays.mjs';
import { createFirmwareService } from './firmware.mjs';
import { fileURLToPath } from 'node:url';
import { readConfig, readSecret } from './config.mjs';
import { PrtgClient } from './prtg.mjs';
import { MonitoringRuntime } from './runtime.mjs';
import { createAdminStore } from './admin-store.mjs';
import { createAdminAuth } from './admin-auth.mjs';
import { createGithubFirmware } from './github-firmware.mjs';
import { createAdmin } from './admin.mjs';
import { createApi, parseAllowedClients } from './server.mjs';
import { createLogger } from './logging.mjs';

const log = createLogger();

try {
  const args = process.argv.slice(2);
  if (args.some(a => !['--demo', '--discover'].includes(a)) || args.length > 1) throw new Error('INVALID_ARGUMENTS');
  const demo = args.includes('--demo');
  const config = readConfig(process.env.CONFIG_FILE || fileURLToPath(new URL('../../config/aggregator.example.json', import.meta.url)));
  const prtgToken = readSecret(process.env, 'PRTG_API_TOKEN', !demo && !process.env.ADMIN_DIRECTORY);
  let client = demo ? null : new PrtgClient(config, prtgToken);
  if (args.includes('--discover')) {
    if(process.env.ADMIN_DIRECTORY){
      const saved=await createAdminStore({directory:process.env.ADMIN_DIRECTORY,config,prtgToken});
      const effective=saved.effective();if(!effective.prtgToken)throw Error('MISSING_PRTG_TOKEN');
      client=new PrtgClient(effective.config,effective.prtgToken);
    }
    const rows = await client.discover();
    // Only allowlisted metadata, no messages, tokens or request URLs.
    console.log(JSON.stringify(rows.map(r => ({ id: r.objid, device: r.device, sensor: r.sensor, status_raw: r.status_raw, lastcheck_raw: r.lastcheck_raw })), null, 2));
  } else {
    const host = process.env.HOST || '127.0.0.1';
    const port = Number(process.env.PORT || 8787);
    if (!Number.isInteger(port) || port < 1 || port > 65535) throw new Error('INVALID_PORT');
    const panelToken = readSecret(process.env, 'PANEL_API_TOKEN', !demo || !['127.0.0.1', '::1'].includes(host));
    const runtime = new MonitoringRuntime({config,prtgToken,demo,log});
    const adminToken = (process.env.OTA_DIRECTORY || process.env.ADMIN_DIRECTORY) ? readSecret(process.env, 'OTA_ADMIN_TOKEN') : '';
    const firmware = process.env.OTA_DIRECTORY ? createFirmwareService({directory:process.env.OTA_DIRECTORY,adminToken,panelToken,log}) : null;
    let admin = null, displays = null;
    if(process.env.ADMIN_DIRECTORY){
      const store = await createAdminStore({directory:process.env.ADMIN_DIRECTORY,config,prtgToken,apply:next=>runtime.update(next)});
      runtime.update(store.effective());
      displays=await createDisplayStore(process.env.ADMIN_DIRECTORY);
      const auth=await createAdminAuth({directory:process.env.ADMIN_DIRECTORY,token:adminToken});
      const github=createGithubFirmware({url:process.env.OTA_GITHUB_CATALOG});
      admin=createAdmin({auth,github,displays,origin:process.env.ADMIN_ORIGIN,token:adminToken,panelToken,store,runtime,firmware,log});
    }
    const server = createApi(runtime, panelToken, parseAllowedClients(process.env.ALLOWED_CLIENT_IPS), firmware, admin, displays);
    server.requestTimeout = firmware ? 120000 : 10000;
    server.headersTimeout = 10000;
    await new Promise((resolve, reject) => { server.once('error', reject); server.listen(port, host, resolve); });
    log('INFO', `EagleNET Aggregator: ${demo ? 'DEMO (synthetische Daten)' : 'LIVE'}; Port ${port}; WebAdmin ${admin ? 'aktiv' : 'aus'}; Firmware ${firmware ? 'aktiv' : 'aus'}. Zeitstempel in UTC.`);
    runtime.start();
    for (const signal of ['SIGTERM', 'SIGINT']) process.on(signal, () => {
      log('INFO', `${signal}: Aggregator wird beendet.`);
      runtime.stop(); server.close();
      setTimeout(() => process.exit(0), 1500).unref();
    });
  }
} catch {
  // Never log exception details: fetch/config/secret errors can contain credentials.
  log('ERROR', 'START_FAILED: Konfiguration, Secret-Dateien, Port und PRTG-Verbindung prüfen.');
  process.exitCode = 1;
}
