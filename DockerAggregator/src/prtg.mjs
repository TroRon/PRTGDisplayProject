const EPOCH = Date.UTC(1899, 11, 30);

export function numberOrNull(value) {
  if (typeof value !== 'number' && typeof value !== 'string') return null;
  if (typeof value === 'string' && !/^-?\d+(?:\.\d+)?$/.test(value.trim())) return null;
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : null;
}

// API account must use UTC. Verify lastcheck_raw against PRTG during commissioning.
export function rawTime(value) {
  const days = numberOrNull(value);
  if (days === null || days <= 0) return null;
  const time = EPOCH + days * 86400000;
  return Number.isFinite(time) && Math.abs(time) < 8.64e15 ? Math.round(time) : null;
}

export function prtgStatus(value) {
  const code = numberOrNull(value);
  if (code === 3) return 'ok';
  if ([4, 10].includes(code)) return 'warning';
  if ([5, 13, 14].includes(code)) return 'critical';
  return 'unknown';
}

export class PrtgClient {
  constructor(config, token, fetchImpl = fetch) {
    this.config = config;
    this.token = token;
    this.fetch = fetchImpl;
  }

  async table(content, params = {}) {
    const url = new URL('/api/table.json', this.config.prtg_url);
    url.search = new URLSearchParams({ content, output: 'json', count: '500', ...params, apitoken: this.token });
    let response;
    try {
      response = await this.fetch(url, {
        redirect: 'error',
        signal: AbortSignal.timeout(this.config.request_timeout_seconds * 1000),
        headers: { accept: 'application/json' }
      });
    } catch { throw new Error('PRTG_NETWORK_OR_TLS'); }
    if ([401, 403].includes(response.status)) throw new Error('PRTG_AUTH');
    if (!response.ok) throw new Error('PRTG_HTTP');
    let body;
    try { body = await response.json(); } catch { throw new Error('PRTG_JSON'); }
    if (!Array.isArray(body?.[content])) throw new Error('PRTG_SHAPE');
    return body[content];
  }

  async discover() {
    const result = [];
    for (let start = 0; start < 50000; start += 500) {
      const rows = await this.table('sensors', { columns: 'objid,device,sensor,status,lastcheck', start: String(start), sortby: 'objid' });
      result.push(...rows);
      if (rows.length < 500) return result;
    }
    throw new Error('PRTG_DISCOVERY_LIMIT');
  }

  async collect(sensor) {
    if (sensor.id === null) return { error: 'NOT_CONFIGURED' };
    try {
      const rows = await this.table('sensors', { filter_objid: String(sensor.id), columns: 'objid,sensor,status,lastcheck', count: '2' });
      const row = rows.find(item => Number(item.objid) === sensor.id);
      if (!row) return { error: 'SENSOR_MISSING' };
      const result = { row, channels: [] };
      if (Object.keys(sensor.metrics).length) {
        try {
          result.channels = await this.table('channels', { id: String(sensor.id), columns: 'name,lastvalue_', count: '500' });
        } catch { result.metrics_error = true; }
      }
      return result;
    } catch (error) { return { error: error.message }; }
  }
}

// Synthetic input goes through the same normalizer as the live adapter.
export function demoCollect(sensor, now = Date.now()) {
  const samples = {
    cpu: 18.4, ram: 35.2, temperature: 87, root_disk: 64, disk: 64,
    zfs_health: 1, zfs_capacity: 19, daily_current: 17, daily_total: 17,
    daily_old: 0, usage: 67.7, engine: 1, running: 38, total: 38
  };
  return {
    row: { objid: sensor.id, status_raw: 3, lastcheck_raw: String((now - EPOCH) / 86400000) },
    channels: Object.entries(sensor.metrics).map(([key, name]) => ({ name, lastvalue_raw: String(samples[key] ?? 0) }))
  };
}
