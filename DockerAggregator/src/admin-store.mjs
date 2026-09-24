import {readFile, open, rename, unlink, mkdir} from 'node:fs/promises';
import {join} from 'node:path';
import {randomUUID} from 'node:crypto';
import {validateConfig} from './config.mjs';

export async function createAdminStore({directory, config, prtgToken = '', apply = () => {}}) {
  await mkdir(directory, {recursive:true, mode:0o770});
  const file = join(directory, 'settings.json');
  let state = {schema:1, revision:0, config:validateConfig(structuredClone(config)), prtgToken:null};
  try {
    const raw = await readFile(file);
    if(raw.length > 131072) throw Error('ADMIN_STATE_SIZE');
    const saved = JSON.parse(raw);
    if(saved.schema !== 1 || !Number.isSafeInteger(saved.revision) || saved.revision < 1 ||
       !(saved.prtgToken === null || validToken(saved.prtgToken))) throw Error('ADMIN_STATE_INVALID');
    validateConfig(saved.config); state = saved;
  } catch(e) { if(e.code !== 'ENOENT') throw e; } // Never silently ignore damaged saved settings.
  let busy = false;
  const effective = () => ({config:structuredClone(state.config), prtgToken:state.prtgToken ?? prtgToken});
  return {
    effective,
    view: () => ({revision:state.revision, config:structuredClone(state.config), hasPrtgToken:!!effective().prtgToken}),
    async save(input) {
      if(busy) throw Error('SAVE_BUSY');
      busy = true;
      let temporary;
      try {
        if(!input || Object.keys(input).some(k=>!['revision','config','prtgToken'].includes(k))) throw Error('CONFIG_REQUEST');
        if(input.revision !== state.revision) throw Error('CONFIG_CONFLICT');
        const nextConfig = validateConfig(structuredClone(input.config));
        const replacement = input.prtgToken;
        if(replacement !== undefined && replacement !== '' && !validToken(replacement)) throw Error('PRTG_TOKEN_INVALID');
        // Changing servers must not forward the old server's secret to a different host.
        if(new URL(nextConfig.prtg_url).origin !== new URL(state.config.prtg_url).origin && !replacement) throw Error('PRTG_TOKEN_REQUIRED');
        const next = {schema:1, revision:state.revision + 1, config:nextConfig, prtgToken:replacement || state.prtgToken};
        const data = JSON.stringify(next, null, 2) + '\n';
        if(Buffer.byteLength(data)>131072) throw Error('CONFIG_TOO_LARGE');
        temporary = join(directory, `.settings-${randomUUID()}.tmp`);
        // Group bits retain the inherited administrator ACL mask. The directory is not public.
        const handle = await open(temporary, 'wx', 0o660);
        try { await handle.writeFile(data); await handle.sync(); } finally { await handle.close(); }
        await rename(temporary, file); temporary = null;
        state = next;
        apply(effective()); // Synchronous in-memory swap; no network/disk work here.
        return this.view();
      } finally { if(temporary) await unlink(temporary).catch(()=>{}); busy = false; }
    }
  };
}
function validToken(value) { return typeof value === 'string' && value.length > 0 && value.length <= 2048 && !/[\s\x00-\x1f\x7f]/.test(value); }
